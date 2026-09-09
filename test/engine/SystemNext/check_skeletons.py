#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Check documented outlines separately from executable infrastructure; compile and run isolated tests."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess


def without_comments(source):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)


def verify(root):
    module = root / 'rts/SystemNext'
    manifest = json.loads((module / 'skeleton-manifest.json').read_text())
    assert manifest['version'] == 3
    assert not (module / 'Globals').exists(), 'Globals must not survive as an ownership category'
    actual = {str(p.relative_to(module)) for p in module.rglob('*.cpp')}
    outlined_files = {entry['file'] for entry in manifest['outlines']}
    assert actual == outlined_files | set(manifest['architecture_sources']), 'Missing or unclassified runtime source'
    incomplete = set(manifest['incomplete_sources'])
    todo_files = {
        name for name in actual
        if 'TODO(SystemNext):' in (module / name).read_text()
    }
    assert todo_files == incomplete, 'Missing or unclassified implementation TODO'
    assert outlined_files <= incomplete, 'Documented outline is not classified incomplete'
    headers = {str(p.relative_to(module)) for p in module.rglob('*.h')}
    assert headers == set(manifest['headers']), 'Missing or unclassified runtime header'

    for name in sorted(actual | headers):
        path = module / name
        source = path.read_text()
        code = without_comments(source)
        # LegacyAdapters are the bridge to the existing engine and are exempt
        # from the isolation checks: they include engine headers and reference
        # engine globals by design.
        if name.startswith('LegacyAdapters/'):
            continue
        for link in re.findall(r'\[[^\]]+\]\(([^)]+)\)', source):
            assert (path.parent / link).is_file(), name + ': broken source link ' + link
        for include in re.findall(r'#include "([^"]+)"', code):
            target = (path.parent / include).resolve()
            assert target.is_relative_to(module.resolve()) and target.is_file(), name + ': engine dependency in isolated infrastructure'
        assert not re.search(r'\bextern\b|\bglobalRendering\b|\bactiveController\b', code), name + ': global/legacy execution dependency'
        assert '#if 0' not in source and 'FIXME' not in source, name + ': obsolete conflict scaffolding'

    loop_header = (module / 'ApplicationLoop.h').read_text()
    assert 'ApplicationContext' not in loop_header and 'ApplicationHost' not in loop_header
    assert all(owner + '&' in loop_header for owner in ['Platform', 'ApplicationLifecycle', 'SnapshotManager', 'Diagnostics'])
    loop_source = (module / 'ApplicationLoop.cpp').read_text()
    for label in ['[ input', '[ session', '[ display', '[ render', '[ present', '[ application lifecycle', '[ diagnostics']:
        assert label in loop_source, 'Application loop missing readable concern label: ' + label
    assert 'BeginLogical(LogicalIterationId' not in (module / 'Snapshots/SnapshotManager.h').read_text(), 'caller allocates logical IDs'

    mode_code = '\n'.join(path.read_text() for path in (module / 'Modes').rglob('*') if path.suffix in {'.h', '.cpp'})
    assert not re.search(r'\b(?:Platform|ApplicationLifecycle|Diagnostics)\s*[*&]', without_comments(mode_code)), 'mode can access an application executor'

    groups = {}
    for entry in manifest['outlines']:
        groups.setdefault(entry['file'], []).append(entry)
    for name, entries in groups.items():
        source = (module / name).read_text()
        expected = '#include "' + Path(name).stem + '.h"\nnamespace runtime {\n'
        mode = entries[0].get('mode')
        if mode:
            cls = entries[0]['class']
            expected += cls + '::' + cls + '(): Mode(ModeKind::' + mode + ') {}\n'
        for entry in entries:
            signature = entry['signature']
            start = source.index(signature + '\n{') + len(signature) + 2
            end = source.index('\n}', start)
            body = source[start:end]
            expected += signature + '{' + ('return {};' if mode else '') + '}\n'
            labels = ['Expected responsibility and why:', 'Expected legacy sources',
                      'Expected work, in conceptual order:', 'Scheduling and lifetime:']
            labels += ['Snapshot contract:', 'Expected outputs and authority:'] if mode else ['Context contract:']
            for label in labels:
                assert label in body, name + ': missing stage explanation ' + label
            assert body.count('/*') >= 3 and len(body.split()) >= 140, name + ': insufficient stage documentation'
            if mode:
                assert 'return {};' in body and 'Implementation status:' in body
                contract = (module / 'Modes' / mode / (mode + 'Snapshots.h')).read_text()
                reads = re.search(r'using ' + entry['function'] + r'Reads = SnapshotReads<(.*?)\n\t>;', contract, re.S)
                assert reads, name + ': missing consumption declaration'
                for required, stage, slot in re.findall(r'(Required|Optional)<Stage::(\w+), Slot::(\w+)>', reads.group(1)):
                    assert stage + '.' + slot in body, name + ': read missing from explanation'
            assert 'TODO(SystemNext):' in body, name + ': incomplete concern has no local TODO'
        expected += '}\n'
        assert re.sub(r'\s+', '', without_comments(source)) == re.sub(r'\s+', '', expected), name + ': hidden/duplicate behavior in outline'

    for name in ['rts/CMakeLists.txt', 'test/CMakeLists.txt', 'test/headercheck/CMakeLists.txt']:
        assert 'SystemNext' not in (root / name).read_text(), name + ': production skeleton registration'
    assert json.loads((module / 'hook-manifest.json').read_text())['hooks'] == []
    for folder in ['rts', 'test']:
        for path in (root / folder).rglob('*'):
            if path.suffix in {'.cpp', '.h', '.hpp', '.cmake', '.txt'} and not {'SystemNext', 'RenderingNext'} & set(path.parts):
                assert 'VKFUN-HOOK(' not in path.read_text(errors='replace'), 'Remaining hook marker: ' + str(path)
    return sorted(actual), manifest


def compile_sources(root, files, output):
    module = root / 'rts/SystemNext'
    tests = root / 'test/engine/SystemNext'
    compiler = os.environ.get('CXX', 'g++')
    failures = [
        'UNDECLARED_SOURCE', 'UNDECLARED_DEPTH', 'CURRENT_DISPLAY', 'MUTABLE_SNAPSHOT',
        'INPUT_TRANSITION', 'FORWARD_CURRENT', 'REQUIRED_VISUAL', 'DUPLICATE_READ',
        'MISSING_PAYLOAD', 'UNKNOWN_SOURCE', 'CURRENT_CYCLE',
    ]
    for variant in ['legacy', 'headless']:
        directory = output / variant
        directory.mkdir(parents=True, exist_ok=True)
        flags = ['-std=c++23', '-Wall', '-Wextra', '-Werror', '-fPIC']
        if variant == 'headless':
            flags.append('-DHEADLESS')
        print('Checking isolated headers and contracts: ' + variant, flush=True)
        for header in sorted(module.rglob('*.h')):
            if 'LegacyAdapters/' in str(header.relative_to(module)):
                continue
            subprocess.run([compiler, *flags, '-x', 'c++', '-fsyntax-only', '-include', str(header), '/dev/null'], check=True)
        subprocess.run([compiler, *flags, '-I', str(module), '-fsyntax-only', str(tests / 'testContextContracts.cpp')], check=True)
        # A positive control must compile before negative diagnostics can count.
        command = [compiler, *flags, '-I', str(module), '-fsyntax-only', str(tests / 'compileFailSnapshots.cpp')]
        subprocess.run(command, check=True)
        for case in failures:
            result = subprocess.run(command + ['-D' + case], capture_output=True, text=True)
            (directory / (case + '.log')).write_text(result.stderr)
            assert result.returncode != 0 and 'error:' in result.stderr, 'Invalid contract compiled: ' + case
        print('PASS 11 compile-failure contracts: ' + variant, flush=True)

        objects = []
        for index, name in enumerate(files):
            if name.startswith('LegacyAdapters/'):
                continue
            obj = directory / ('concern-%d.o' % index)
            subprocess.run([compiler, *flags, '-c', str(module / name), '-o', str(obj)], check=True)
            objects.append(str(obj))
        for test in ['testSnapshotManager', 'testLoopArchitecture']:
            executable = directory / test
            subprocess.run([compiler, *flags, '-I', str(module), str(tests / (test + '.cpp')), *objects, '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
        subprocess.run([compiler, '-shared', '-Wl,--no-undefined', *objects, '-o', str(directory / 'systemnext-contracts.so')], check=True)
        print('PASS standalone compilation, linkage and runtime tests: ' + variant, flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--compile-dir', type=Path)
    args = parser.parse_args()
    files, manifest = verify(args.root)
    if args.compile_dir:
        compile_sources(args.root, files, args.compile_dir)
    print('PASS: %d documented concerns, %d translation units; source boundaries and documentation verified%s' % (
        len(manifest['outlines']), len(files), '; isolated legacy/headless checks passed' if args.compile_dir else ''))
