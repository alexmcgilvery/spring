#!/usr/bin/env python3
"""Validate // CONCERN: <name> annotations in the legacy call-flow source set.

Usage:
    tools/validate_concern_annotations.py [root]

Scans the configured source directories for `// CONCERN:` lines, checks that
every concern is one of the five allowed names, and prints a report grouped by
file and concern. Exits non-zero if any unknown concern is found.
"""

import os
import re
import sys
from collections import defaultdict

ALLOWED_CONCERNS = {"input", "session", "display", "render", "present"}

DEFAULT_ROOT = os.path.join(os.path.dirname(__file__), "..")

SCAN_DIRS = [
    "rts/System/SpringApp.cpp",
    "rts/System/Input/InputHandler.cpp",
    "rts/Game/GameController.h",
    "rts/Game/Game.cpp",
    "rts/Game/PreGame.cpp",
    "rts/Game/LoadScreen.cpp",
    "rts/Game/UI/MouseHandler.cpp",
    "rts/Game/CameraHandler.cpp",
    "rts/Menu/SelectMenu.cpp",
    "rts/Menu/LuaMenuController.cpp",
    "rts/Net/NetCommands.cpp",
    "rts/Rendering/WorldDrawer.cpp",
    "rts/Rendering/HUDDrawer.cpp",
    "rts/Rendering/GlobalRendering.cpp",
    "rts/Rendering/ShadowHandler.cpp",
    "rts/Rml/Backends/RmlUi_Backend.cpp",
    "rts/SystemNext/LegacyAdapters/PlatformAdapter.cpp",
]

CONCERN_RE = re.compile(r"^\s*//\s*CONCERN:\s*([a-zA-Z]+)")


def main() -> int:
    root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else DEFAULT_ROOT)
    unknown = []
    by_file = defaultdict(list)
    total = 0

    for rel in SCAN_DIRS:
        path = os.path.join(root, rel)
        if not os.path.isfile(path):
            print(f"warning: missing file {rel}", file=sys.stderr)
            continue
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for lineno, line in enumerate(fh, 1):
                m = CONCERN_RE.match(line)
                if not m:
                    continue
                concern = m.group(1).lower()
                total += 1
                by_file[rel].append((lineno, concern, line.strip()))
                if concern not in ALLOWED_CONCERNS:
                    unknown.append((rel, lineno, concern))

    for rel in sorted(by_file):
        print(f"== {rel}")
        for lineno, concern, text in by_file[rel]:
            flag = "" if concern in ALLOWED_CONCERNS else "  <-- UNKNOWN"
            print(f"  {lineno:6d}: {concern:8s}{flag}")

    print()
    print(f"total annotations: {total}")
    print(f"files with annotations: {len(by_file)}")
    if unknown:
        print(f"unknown concerns: {len(unknown)}")
        for rel, lineno, concern in unknown:
            print(f"  {rel}:{lineno}: {concern}")
        return 1
    print("unknown concerns: 0")
    return 0


if __name__ == "__main__":
    sys.exit(main())
