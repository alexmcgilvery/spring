#!/usr/bin/env python3
"""Check runtime hook presence and explicit upstream event classification.

This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""

import argparse
import json
from pathlib import Path
import re


EVENT = re.compile(r"^\s*SETUP_(?:UNMANAGED_)?EVENT\((\w+),\s*([^\n]+?)\)", re.MULTILINE)
MARKER = re.compile(r"^\s*(?://|#)\s*VKFUN-HOOK\(([A-Za-z0-9_-]+)\)", re.MULTILINE)
CATEGORIES = {
    "publication_notification", "physical_removal", "synchronous_control",
    "lifecycle", "platform_input", "external_service", "legacy_presentation",
}
KINDS = {"runtime_call", "read_interface", "build_test"}
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.MULTILINE)
FORBIDDEN = re.compile(r'\b(?:lua_State|GLuint|GLenum|Vk\w+|CR_DECLARE\w*|CUnit|CFeature|CProjectile|float3|CMatrix44f)\b')


def check_boundaries(root):
    """Core source edges must stay in the module and outside legacy adapters."""
    module = (root / "rts/SystemNext").resolve()
    errors = []
    for path in module.rglob("*"):
        if path.suffix not in (".h", ".hpp", ".cpp"):
            continue
        source = path.read_text(encoding="utf-8")
        if re.search(r"\bRFC[- ]?\d+\b", source):
            errors.append(f"{path.name}: document reference instead of a self-contained source contract")
        if (path.name.startswith("Legacy") or "Compatibility" in path.parts or
                (path.suffix == ".cpp" and (path.parent.name in {"SelectMenu", "LuaMenu", "PreGame", "Loading", "Game"} or path == module / "Simulation/Simulation.cpp"))):
            continue
        for include in INCLUDE.findall(source):
            target = (path.parent / include).resolve()
            if not target.exists():
                target = (root / "rts" / include).resolve()
            try:
                relative = target.relative_to(module)
            except ValueError:
                errors.append(f"{path.name}: forbidden core include {include}")
                continue
            if relative.parts[0] == "Compatibility" or target.name.startswith("Legacy"):
                errors.append(f"{path.name}: core includes legacy adapter {include}")
            elif not target.is_file():
                errors.append(f"{path.name}: missing core include {include}")
        if path.suffix in (".h", ".hpp") and FORBIDDEN.search(source):
            errors.append(f"{path.name}: legacy type in public contract")
    return errors


def check_events(source, manifest):
    errors = []
    expected = {}
    for name, props in EVENT.findall(source):
        if name in expected:
            errors.append(f"duplicate source event: {name}")
        expected[name] = sorted(x.strip() for x in props.split("|"))
    if not expected:
        return ["no source events found"]
    if manifest.get("schema") != 1 or not isinstance(manifest.get("events"), list):
        return ["invalid event manifest schema"]
    seen = set()
    for entry in manifest["events"]:
        if not isinstance(entry, dict) or not isinstance(entry.get("name"), str):
            errors.append("invalid event entry")
            continue
        name = entry["name"]
        if name in seen:
            errors.append(f"duplicate classification: {name}")
        seen.add(name)
        props = entry.get("properties")
        if not isinstance(props, list) or not all(isinstance(p, str) for p in props):
            errors.append(f"invalid properties: {name}")
            continue
        if name not in expected:
            errors.append(f"stale classification: {name}")
        elif sorted(props) != expected[name]:
            errors.append(f"changed event properties: {name}")
        category = entry.get("category")
        if not isinstance(category, str) or category not in CATEGORIES:
            errors.append(f"invalid event category: {name}")
        if "CONTROL_BIT" in props and category != "synchronous_control":
            errors.append(f"control event cannot be published: {name}")
        if category == "physical_removal" and name != "RenderUnitDestroyed":
            errors.append(f"unreviewed physical-removal event: {name}")
    for name in sorted(expected.keys() - seen):
        errors.append(f"unclassified event: {name}")
    return errors


def source_files(root):
    # Only engine-owned source trees. Vendored modules, build artifacts and the
    # checker itself are not upstream hook sites.
    for directory in ("rts", "test"):
        start = root / directory
        if not start.exists():
            continue
        for path in start.rglob("*"):
            relative = path.relative_to(root)
            if relative.parts[:2] == ("rts", "lib") or (relative.parts[:2] == ("rts", "SystemNext") and path.suffix not in (".cpp", ".h", ".hpp")):
                continue
            if path.is_file() and (path.suffix in (".cpp", ".h", ".hpp", ".cmake", ".py", ".sh") or path.name == "CMakeLists.txt"):
                yield path
    if (root / "CMakeLists.txt").exists():
        yield root / "CMakeLists.txt"


def check_hooks(root, manifest):
    if manifest.get("schema") != 1 or not isinstance(manifest.get("hooks"), list):
        return ["invalid hook manifest schema"]
    errors = []
    sites = {}
    texts = {}
    for path in source_files(root):
        text = path.read_text(encoding="utf-8")
        name = path.relative_to(root).as_posix()
        texts[name] = text
        for match in MARKER.finditer(text):
            sites.setdefault(match[1], []).append((name, match.end()))
    seen = set()
    for entry in manifest["hooks"]:
        if not isinstance(entry, dict):
            errors.append("invalid hook entry")
            continue
        hook_id = entry.get("id")
        if not isinstance(hook_id, str) or not re.fullmatch(r"[A-Za-z0-9_-]+", hook_id):
            errors.append("invalid hook ID")
            continue
        if hook_id in seen:
            errors.append(f"duplicate manifest hook: {hook_id}")
        seen.add(hook_id)
        for field in ("file", "symbol", "reason", "ordering", "fragment"):
            if not isinstance(entry.get(field), str) or not entry[field].strip():
                errors.append(f"{hook_id}: missing {field}")
        kind = entry.get("kind")
        if not isinstance(kind, str) or kind not in KINDS:
            errors.append(f"{hook_id}: invalid kind")
        if not isinstance(entry.get("upstreamable"), bool):
            errors.append(f"{hook_id}: missing upstreamable boolean")
        if not isinstance(entry.get("tests"), list) or not entry["tests"] or not all(isinstance(t, str) and t for t in entry["tests"]):
            errors.append(f"{hook_id}: missing covering tests")
        matches = sites.get(hook_id, [])
        if len(matches) != 1:
            errors.append(f"{hook_id}: expected one marker, found {len(matches)}")
            continue
        filename, offset = matches[0]
        if filename != entry.get("file"):
            errors.append(f"{hook_id}: marker in wrong file ({filename})")
        # A marker left behind after deleting its call must not pass. The
        # referenced fragment must be the immediately following code, ignoring
        # whitespace only (not arbitrary code elsewhere in the file).
        fragment = entry.get("fragment")
        if isinstance(fragment, str) and fragment.strip():
            actual = re.sub(r"\s+", "", texts[filename][offset:])
            expected = re.sub(r"\s+", "", fragment)
            if not actual.startswith(expected):
                errors.append(f"{hook_id}: code fragment missing after marker")
    for hook_id in sorted(sites.keys() - seen):
        errors.append(f"unregistered hook: {hook_id}")
    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[3])
    args = parser.parse_args(argv)
    try:
        runtime = args.root / "rts/SystemNext"
        events = json.loads((runtime / "Simulation/Observation/event-classification.json").read_text())
        hooks = json.loads((runtime / "hook-manifest.json").read_text())
        errors = check_events((args.root / "rts/System/Events.def").read_text(), events)
        errors.extend(check_hooks(args.root, hooks))
        errors.extend(check_boundaries(args.root))
    except (OSError, ValueError, AttributeError) as error:
        print(f"FAIL: cannot check contracts: {error}")
        return 1
    for error in errors:
        print(f"FAIL: {error}")
    if not errors:
        print(f"PASS: {len(events['events'])} classified events, {len(hooks['hooks'])} registered hooks")
    return bool(errors)


if __name__ == "__main__":
    raise SystemExit(main())
