#!/usr/bin/env python3
"""Runtime offline JSONL validation. Exit 0=pass, 1=fail, 2=incomplete.

This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
Uses only the Python standard library; no engine process or graphics required.
"""

import argparse
import json
from pathlib import Path


class ReportError(Exception):
    def __init__(self, outcome, message):
        super().__init__(message)
        self.outcome = outcome


def incomplete(message):
    raise ReportError("incomplete", message)


def failed(message):
    raise ReportError("fail", message)


def integer(value):
    return isinstance(value, int) and not isinstance(value, bool)


def require(record, key, predicate, line):
    if key not in record or not predicate(record[key]):
        incomplete(f"line {line}: missing or invalid {key}")
    return record[key]


def read_run(path):
    header = None
    footer = None
    stacks = {}
    used_scopes = set()
    frames = []
    messages = []
    sessions = []
    publications = []
    frame_keys = set()
    last_tick = {}
    last_revision = {}
    active_epoch = None
    last_epoch = 0
    guards = {}
    last_message = {}
    record_count = 0
    context = []
    known_types = {
        "run_start", "run_end", "session", "phase_begin", "phase_end",
        "controller", "load_guard", "message", "frame", "publication",
        "summary", "incomplete",
    }
    try:
        with Path(path).open(encoding="utf-8-sig") as stream:
            for line_no, line in enumerate(stream, 1):
                if not line.strip():
                    incomplete(f"line {line_no}: empty record")
                try:
                    record = json.loads(line)
                except (ValueError, RecursionError) as error:
                    incomplete(f"line {line_no}: invalid JSON ({error})")
                if not isinstance(record, dict):
                    incomplete(f"line {line_no}: record is not an object")
                if type(record.get("schema")) is not int or record["schema"] != 1:
                    incomplete(f"line {line_no}: unsupported schema")
                seq = require(record, "seq", integer, line_no)
                if seq != record_count:
                    incomplete(f"line {line_no}: expected sequence {record_count}, got {seq}")
                record_count += 1
                kind = require(record, "type", lambda x: isinstance(x, str), line_no)
                if kind not in known_types:
                    incomplete(f"line {line_no}: unknown record type {kind}")
                if footer is not None:
                    failed(f"line {line_no}: record after run_end")
                if header is None and kind != "run_start":
                    incomplete(f"line {line_no}: missing run_start")
                context.append({"seq": seq, "type": kind})
                context = context[-5:]

                if kind == "run_start":
                    if header is not None:
                        failed(f"line {line_no}: duplicate run_start")
                    require(record, "run_id", lambda x: isinstance(x, str) and bool(x), line_no)
                    require(record, "engine_revision", lambda x: isinstance(x, str) and bool(x), line_no)
                    require(record, "diagnostics", lambda x: x in ("summary", "detailed"), line_no)
                    header = record
                elif kind == "incomplete":
                    incomplete(f"line {line_no}: {record.get('reason', 'capture invalidated')}")
                elif kind == "run_end":
                    for key, expected in (("complete", True), ("observation_valid", True), ("io_failed", False)):
                        if record.get(key) is not expected:
                            incomplete(f"line {line_no}: run_end {key} is not {expected}")
                    if not integer(record.get("dropped")) or record["dropped"] != 0:
                        incomplete(f"line {line_no}: dropped records or missing drop count")
                    if any(stacks.values()):
                        failed(f"line {line_no}: completed run has unfinished phase scopes")
                    if any(guards.values()) or active_epoch is not None:
                        failed(f"line {line_no}: completed run has an active guard or session")
                    footer = record
                elif kind in ("phase_begin", "phase_end"):
                    thread = require(record, "thread", lambda x: isinstance(x, str), line_no)
                    scope = require(record, "scope", lambda x: integer(x) and x >= 0, line_no)
                    phase = require(record, "phase", lambda x: isinstance(x, str), line_no)
                    stack = stacks.setdefault(thread, [])
                    if kind == "phase_begin":
                        if phase in ("draw", "swap") and not guards.get(thread):
                            failed(f"line {line_no}: {phase} outside load/draw guard")
                        if (thread, scope) in used_scopes:
                            failed(f"line {line_no}: reused phase scope {scope}")
                        expected_parent = stack[-1][0] if stack else None
                        if "parent" not in record or record["parent"] != expected_parent:
                            failed(f"line {line_no}: invalid parent for scope {scope}; context={context}")
                        used_scopes.add((thread, scope))
                        stack.append((scope, phase))
                    elif not stack or stack.pop() != (scope, phase):
                        failed(f"line {line_no}: mismatched phase end; context={context}")
                elif kind == "session":
                    action = require(record, "action", lambda x: x in ("bootstrap", "end", "reset"), line_no)
                    epoch = require(record, "epoch", lambda x: integer(x) and x > 0, line_no)
                    if action in ("bootstrap", "reset"):
                        if epoch <= last_epoch:
                            failed(f"line {line_no}: epoch did not advance")
                        if active_epoch is not None and action == "bootstrap":
                            failed(f"line {line_no}: bootstrap while another session is active")
                        active_epoch = epoch
                        last_epoch = epoch
                    elif epoch != active_epoch:
                        failed(f"line {line_no}: session end for inactive epoch")
                    else:
                        active_epoch = None
                    sessions.append(record)
                elif kind in ("frame", "publication", "message"):
                    epoch = require(record, "epoch", lambda x: integer(x) and x > 0, line_no)
                    tick = require(record, "tick", integer, line_no)
                    if epoch != active_epoch:
                        failed(f"line {line_no}: data for an inactive epoch")
                    if kind == "frame":
                        checksum = require(record, "checksum", lambda x: x is None or (integer(x) and 0 <= x <= 0xffffffff), line_no)
                        key = (epoch, tick)
                        if key in frame_keys or tick <= last_tick.get(epoch, tick - 1):
                            failed(f"line {line_no}: duplicate or decreasing completed tick {key}")
                        if epoch in last_tick and tick != last_tick[epoch] + 1:
                            incomplete(f"line {line_no}: missing completed ticks in epoch {epoch}")
                        frame_keys.add(key)
                        last_tick[epoch] = tick
                        frames.append((epoch, tick, checksum))
                    elif kind == "publication":
                        revision = require(record, "revision", lambda x: integer(x) and x > 0, line_no)
                        first = require(record, "first_event", lambda x: integer(x) and x >= 0, line_no)
                        end = require(record, "end_event", lambda x: integer(x) and x >= first, line_no)
                        if record.get("valid") is not True:
                            incomplete(f"line {line_no}: publication invalid or unvalidated")
                        if revision <= last_revision.get(epoch, 0):
                            failed(f"line {line_no}: publication revision did not advance")
                        last_revision[epoch] = revision
                        publications.append((epoch, tick, revision, first, end))
                    else:
                        ordinal = require(record, "ordinal", lambda x: integer(x) and x >= 0, line_no)
                        code = require(record, "code", lambda x: integer(x) and 0 <= x <= 255, line_no)
                        length = require(record, "length", lambda x: integer(x) and x > 0, line_no)
                        digest = require(record, "digest", lambda x: isinstance(x, str) and bool(x), line_no)
                        previous = last_message.get(epoch, -1)
                        if ordinal != previous + 1:
                            incomplete(f"line {line_no}: message sequence gap in epoch {epoch}")
                        messages.append((epoch, ordinal, code, length, digest))
                        last_message[epoch] = ordinal
                elif kind == "load_guard":
                    thread = require(record, "thread", lambda x: isinstance(x, str), line_no)
                    action = require(record, "action", lambda x: x in ("acquire", "release"), line_no)
                    mode = require(record, "mode", lambda x: x in ("noop", "context"), line_no)
                    if action == "acquire":
                        if guards.get(thread):
                            failed(f"line {line_no}: duplicate outer load/draw guard")
                        guards[thread] = mode
                    elif guards.get(thread) != mode:
                        failed(f"line {line_no}: mismatched load/draw guard release")
                    else:
                        guards[thread] = None
                elif kind == "controller":
                    require(record, "iteration", lambda x: integer(x) and x >= 0, line_no)
                    for field in ("before", "after"):
                        require(record, field, lambda x: x is None or (integer(x) and x >= 0), line_no)
                elif kind == "summary":
                    if record.get("observation_valid") is False or record.get("dropped", 0) != 0:
                        incomplete(f"line {line_no}: incomplete summary")
    except (OSError, UnicodeError) as error:
        incomplete(f"cannot read {path}: {error}")
    if header is None or footer is None:
        incomplete(f"{path}: missing run boundaries (capture may be truncated)")
    return {"header": header, "sessions": sessions, "frames": frames,
            "messages": messages, "publications": publications, "records": record_count}


def compare_runs(reference, candidate):
    # Identity is mandatory. Matching engine revisions would defeat comparison of
    # baseline versus candidate; compare inputs/configuration, not build identity.
    for run in (reference, candidate):
        if run["header"]["diagnostics"] != "detailed":
            incomplete("comparison requires detailed capture in both runs")
        if not run["frames"] or any(frame[2] is None for frame in run["frames"]):
            incomplete("comparison requires completed frames with SYNCCHECK values")
    for field in ("input_id", "game", "map", "config_id"):
        left, right = reference["header"].get(field), candidate["header"].get(field)
        if not isinstance(left, str) or not left or left != right:
            incomplete(f"comparison requires matching nonempty {field}")
    for name in ("frames", "messages"):
        left, right = reference[name], candidate[name]
        if name == "messages" and (not left or not right):
            incomplete("comparison requires accepted message records")
        if len(left) != len(right):
            incomplete(f"{name}: different coverage ({len(left)} versus {len(right)})")
        for index, (a, b) in enumerate(zip(left, right)):
            if a != b:
                failed(f"{name}[{index}]: {a} != {b}; "
                       f"reference context={left[max(0, index - 2):index + 3]}; "
                       f"candidate context={right[max(0, index - 2):index + 3]}")
    left = [(r["action"], r["epoch"]) for r in reference["sessions"]]
    right = [(r["action"], r["epoch"]) for r in candidate["sessions"]]
    if left != right:
        failed(f"session lifecycle mismatch: {left} != {right}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--reference", type=Path)
    args = parser.parse_args(argv)
    try:
        candidate = read_run(args.log)
        if args.reference:
            compare_runs(read_run(args.reference), candidate)
        scope = "accepted-input/checksum comparison" if args.reference else "structural capture validation only"
        print(json.dumps({"outcome": "pass", "scope": scope, "records": candidate["records"]}))
        return 0
    except ReportError as error:
        print(json.dumps({"outcome": error.outcome, "reason": str(error)}))
        return 1 if error.outcome == "fail" else 2


if __name__ == "__main__":
    raise SystemExit(main())
