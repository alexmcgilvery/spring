#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Validate concern/context contracts; optionally compile/link without running them."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess


def verify(root):
    module = root / 'rts/SystemNext'
    manifest = json.loads((module / 'skeleton-manifest.json').read_text())
    files = {}
    for entry in manifest['functions']:
        files.setdefault(entry['file'], []).append(entry)
    actual = {str(p.relative_to(module)) for p in module.rglob('*.cpp')}
    assert actual == set(files) | set(manifest['architecture_sources']), 'Missing or unlisted runtime source'
    for name in actual:
        path = module / name
        for link in re.findall(r'\[[^\]]+\]\(([^)]+)\)', path.read_text()):
            assert (path.parent / link).is_file(), name + ': broken architecture source link ' + link
    headers = {str(p.relative_to(module)) for p in module.rglob('*.h')}
    assert headers == {str(Path(p).with_suffix('.h')) for p in files} | set(manifest['contract_headers']), 'Unexpected runtime headers'
    for name in manifest['contract_headers']:
        path = module / name
        code = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        assert not re.search(r'\bextern\b|\bvoid\s*\*', code), name + ': global or untyped dependency'
        assert not re.search(r'\)\s*(?:const\s*)?\{', code), name + ': executable contract body'
        for include in re.findall(r'#include \"([^\"]+)\"', code):
            target = (path.parent / include).resolve()
            assert target.is_relative_to(module.resolve()) and target.is_file(), name + ': non-contract dependency'
    for name, entries in files.items():
        if name == 'ApplicationLoop.cpp':
            continue
        path = module / name
        source = path.read_text()
        clean = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
        expected = '#include "' + path.stem + '.h"\nnamespace runtime {\n'
        mode = entries[0]['class'] in manifest['mode_capabilities']
        if mode:
            kind, session, display = manifest['mode_capabilities'][entries[0]['class']]
            cls = entries[0]['class']
            expected += cls + '::' + cls + '(): IMode(ModeKind::' + kind + ', ' + session + ', DisplayPhase::' + display + ') {}\n'
        for e in entries:
            if mode:
                expected += 'void %s::%s(const %s& supplied) { [[maybe_unused]] const auto& context = std::get<%s>(supplied); }\n' % (e['class'], e['function'], e['context'], e['concrete_context'])
            else:
                expected += 'void %s::%s(const %s&) {}\n' % (e['class'], e['function'], e['context'])
        expected += '}\n'
        assert re.sub(r'\s+', '', clean) == re.sub(r'\s+', '', expected), name + ': unexpected concern implementation'
        header = re.sub(r'/\*.*?\*/', '', path.with_suffix('.h').read_text(), flags=re.S)
        include = '../IMode.h' if mode else Path(entries[0]['context_header']).name
        expected_header = '#pragma once\n#include "' + include + '"\nnamespace runtime { class ' + entries[0]['class']
        expected_header += ' final : public IMode { public:\n' if mode else ' { public:\n'
        if mode:
            expected_header += entries[0]['class'] + '();\n'
        expected_header += ''.join('void %s(const %s& context)%s;\n' % (e['function'], e['context'], ' override' if mode else '') for e in entries) + '}; }'
        assert re.sub(r'\s+', '', header) == re.sub(r'\s+', '', expected_header), name + ': unexpected concern declaration'
        for e in entries:
            body = source.split('void %s::%s(const %s&%s)\n{' % (e['class'], e['function'], e['context'], ' supplied' if mode else ''), 1)[1].split('\n}', 1)[0]
            for label in ['Context contract:', 'Expected legacy sources', 'Expected responsibility:', 'Expected work, in conceptual order:', 'Expected dependencies:', 'Expected relationships:']:
                assert label in body, name + ': missing ' + label
            assert body.count('/*') >= 3, name + ': needs internal concern documentation'
            assert len(body.split()) >= 150, name + ': insufficient concern detail'
        assert 'FIXME' not in source and '#if' not in source, name + ': obsolete conflict machinery'
        for link in re.findall(r'\[[^\]]+\]\(([^)]+)\)', source):
            assert (path.parent / link).is_file(), name + ': broken source link ' + link
    for name in ['rts/CMakeLists.txt', 'test/CMakeLists.txt', 'test/headercheck/CMakeLists.txt']:
        assert 'SystemNext' not in (root / name).read_text(), name + ': production skeleton registration'
    assert json.loads((module / 'hook-manifest.json').read_text())['hooks'] == []
    markers = []
    for folder in ['rts', 'test']:
        for path in (root / folder).rglob('*'):
            if path.suffix in {'.cpp', '.h', '.hpp', '.cmake', '.txt'} and 'SystemNext' not in path.parts and 'RenderingNext' not in path.parts:
                if 'VKFUN-HOOK(' in path.read_text(errors='replace'):
                    markers.append(str(path.relative_to(root)))
    assert not markers, 'Remaining hook markers: ' + str(markers)
    return sorted(actual), len(manifest['functions'])


def compile_sources(root, files, output):
    module = root / 'rts/SystemNext'
    compiler = os.environ.get('CXX', 'g++')
    for variant in ['legacy', 'headless']:
        directory = output / variant
        directory.mkdir(parents=True, exist_ok=True)
        flags = ['-std=c++23', '-Wall', '-Wextra', '-Werror', '-fPIC']
        if variant == 'headless':
            flags.append('-DHEADLESS')
        objects = []
        for header in module.rglob('*.h'):
            subprocess.run([compiler, *flags, '-x', 'c++', '-fsyntax-only', '-include', str(header), '/dev/null'], check=True)
        subprocess.run([compiler, *flags, '-I', str(module), '-fsyntax-only', str(root / 'test/engine/SystemNext/testContextContracts.cpp')], check=True)
        for index, name in enumerate(files):
            source = module / name
            obj = directory / ('concern-%d.o' % index)
            subprocess.run([compiler, *flags, '-c', str(source), '-o', str(obj)], check=True)
            objects.append(str(obj))
        subprocess.run([compiler, *flags, '-I', str(module), str(root / 'test/engine/SystemNext/testLoopArchitecture.cpp'), *objects, '-o', str(directory / 'testLoopArchitecture')], check=True)
        subprocess.run([str(directory / 'testLoopArchitecture')], check=True)
        subprocess.run([compiler, '-shared', '-Wl,--no-undefined', *objects, '-o', str(directory / 'skeletons.so')], check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--compile-dir', type=Path)
    args = parser.parse_args()
    files, count = verify(args.root)
    if args.compile_dir:
        compile_sources(args.root, files, args.compile_dir)
    print('PASS: %d concern contracts, %d translation units; context contracts, outlines, links and isolation verified%s' % (count, len(files), '; compiled/linked legacy and headless independently' if args.compile_dir else ''))
