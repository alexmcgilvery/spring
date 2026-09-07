#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Reject removal or replacement of baseline legacy source lines."""
import argparse
import difflib
import json
from pathlib import Path
import subprocess


def removed_lines(before, after):
    # Hook additions may insert between any two original lines. Even comment and
    # whitespace rewrites are rejected: keeping originals makes review mechanical.
    return [line for group in difflib.SequenceMatcher(None, before.splitlines(), after.splitlines(), autojunk=False).get_opcodes()
            if group[0] in ('delete', 'replace') for line in before.splitlines()[group[1]:group[2]]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[3])
    args = parser.parse_args()
    manifest = json.loads((args.root / 'rts/SystemNext/legacy-surface.json').read_text())
    failures = []
    for name in manifest['files']:
        original = subprocess.check_output(['git', 'show', manifest['baseline'] + ':' + name], cwd=args.root).decode()
        current = (args.root / name).read_text()
        if removed_lines(original, current):
            failures.append(name)
    if failures:
        print('FAIL: legacy lines removed or replaced: ' + ', '.join(failures))
        return 1
    print('PASS: %d legacy files retain all baseline lines; hook additions checked separately' % len(manifest['files']))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
