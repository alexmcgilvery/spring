#!/usr/bin/env python3
"""Portable regression tests for runtime offline tools.

This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""

import copy
import json
from pathlib import Path
import tempfile
import unittest

import analyze_runtime_log as analyze
import check_runtime_contracts as check_contracts
import check_legacy_surface as check_surface


class LegacySurfaceTests(unittest.TestCase):
    def test_additive_hook_preserves_body(self):
        self.assertEqual(check_surface.removed_lines('void Update() {\nWork();\n}\n', 'void Update() {\nObserve();\nWork();\n}\n'), [])

    def test_replacement_dispatch_is_rejected(self):
        self.assertTrue(check_surface.removed_lines('Work();\n', 'adapter.Work();\n'))

    def test_interface_removal_is_rejected(self):
        self.assertTrue(check_surface.removed_lines('bool Update();\nbool Draw();\n', 'bool Draw();\n'))


class DependencyTests(unittest.TestCase):
    def test_core_cannot_include_legacy_or_adapter_headers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            core = root / "rts/SystemNext/Simulation/Publication"
            core.mkdir(parents=True)
            header = core / "Frame.h"
            header.write_text('#pragma once\n#include <cstdint>\n')
            self.assertEqual(check_contracts.check_boundaries(root), [])
            header.write_text('#include "Sim/Units/Unit.h"\n')
            self.assertTrue(check_contracts.check_boundaries(root))
            adapter = root / "rts/SystemNext/Simulation/LegacyReader.h"
            adapter.parent.mkdir(parents=True, exist_ok=True)
            adapter.write_text('#pragma once\n')
            header.write_text('#include "SystemNext/Simulation/LegacyReader.h"\n')
            self.assertTrue(check_contracts.check_boundaries(root))
            header.write_text('class CUnit;\n')
            self.assertTrue(check_contracts.check_boundaries(root))

    def test_root_loop_and_nested_concerns_are_checked(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for relative in ("ApplicationLoop.cpp", "LoopServices.h", "Simulation/Publication/Frame.h"):
                source = root / "rts/SystemNext" / relative
                source.parent.mkdir(parents=True, exist_ok=True)
                source.write_text('#include "Game/Game.h"\n')
            self.assertEqual(len(check_contracts.check_boundaries(root)), 3)

    def test_source_contracts_do_not_depend_on_external_design_documents(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "rts/SystemNext/Session/LegacySession.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("// Follow " + "RFC-0" + " for ordering.\n")
            self.assertTrue(check_contracts.check_boundaries(root))
            source.write_text("// Preserve commands between authoritative ticks.\n")
            self.assertEqual(check_contracts.check_boundaries(root), [])


def capture():
    records = [
        {"type": "run_start", "run_id": "test", "engine_revision": "baseline", "diagnostics": "detailed",
         "input_id": "demo-sha256", "game": "pinned-game", "map": "pinned-map", "config_id": "config-sha256"},
        {"type": "session", "action": "bootstrap", "epoch": 1},
        {"type": "phase_begin", "thread": "main", "scope": 1, "parent": None, "phase": "update"},
        {"type": "message", "epoch": 1, "tick": 0, "ordinal": 0, "code": 2, "length": 1, "digest": "packet"},
        {"type": "frame", "epoch": 1, "tick": 1, "checksum": 123},
        {"type": "publication", "epoch": 1, "tick": 1, "revision": 1, "first_event": 0, "end_event": 1, "valid": True},
        {"type": "phase_end", "thread": "main", "scope": 1, "phase": "update"},
        {"type": "session", "action": "end", "epoch": 1},
        {"type": "run_end", "complete": True, "observation_valid": True, "io_failed": False, "dropped": 0},
    ]
    return [{"schema": 1, "seq": seq, **record} for seq, record in enumerate(records)]


class AnalyzerTests(unittest.TestCase):
    def read(self, records, bom=False):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.jsonl"
            path.write_text("\n".join(json.dumps(r) for r in records) + "\n", encoding="utf-8-sig" if bom else "utf-8")
            return analyze.read_run(path)

    def rejects(self, records, outcome="incomplete"):
        with self.assertRaises(analyze.ReportError) as caught:
            self.read(records)
        self.assertEqual(caught.exception.outcome, outcome)

    def test_complete_bom_capture(self):
        self.assertEqual(self.read(capture(), bom=True)["frames"], [(1, 1, 123)])

    def test_missing_end_cannot_pass(self):
        self.rejects(capture()[:-1])

    def test_sequence_gap_and_duplicate(self):
        for replacement in (1, 4):
            records = capture()
            records[2]["seq"] = replacement
            self.rejects(records)

    def test_unknown_schema_or_record(self):
        for key, value in (("schema", 2), ("type", "future_record"), ("schema", True)):
            records = capture()
            records[2][key] = value
            self.rejects(records)

    def test_failure_end_counters(self):
        for key, value in (("complete", False), ("observation_valid", False), ("io_failed", True), ("dropped", 1)):
            records = capture()
            records[-1][key] = value
            self.rejects(records)

    def test_unfinished_or_misnested_phase(self):
        records = capture()
        records[6]["scope"] = 2
        self.rejects(records, "fail")
        records = capture()
        records[2]["parent"] = 99
        self.rejects(records, "fail")

    def test_explicit_overflow(self):
        records = capture()
        records[5] = {"schema": 1, "seq": 5, "type": "incomplete", "reason": "journal_limit"}
        self.rejects(records)

    def test_invalid_publication(self):
        records = capture()
        records[5]["valid"] = False
        self.rejects(records)

    def test_stale_epoch(self):
        records = capture()
        records[5]["epoch"] = 2
        self.rejects(records, "fail")

    def test_data_after_session_end_fails(self):
        records = capture()
        records[5] = {"schema": 1, "seq": 5, "type": "session", "epoch": 1, "action": "end"}
        records[6] = {"schema": 1, "seq": 6, "type": "frame", "epoch": 1, "tick": 2, "checksum": 456}
        self.rejects(records, "fail")

    def test_unprotected_swap_fails(self):
        records = capture()
        records[2]["phase"] = "swap"
        records[6]["phase"] = "swap"
        self.rejects(records, "fail")

    def test_completed_tick_gap_is_incomplete(self):
        records = capture()
        records[5] = {"schema": 1, "seq": 5, "type": "frame", "epoch": 1, "tick": 3, "checksum": 456}
        self.rejects(records)

    def test_message_gap(self):
        records = capture()
        records[3]["ordinal"] = 1
        self.rejects(records)

    def test_malformed_or_missing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.jsonl"
            path.write_text('{"schema":1,')
            with self.assertRaises(analyze.ReportError):
                analyze.read_run(path)
            with self.assertRaises(analyze.ReportError):
                analyze.read_run(path.with_name("absent"))

    def test_compare_different_builds_and_wall_times(self):
        left = capture()
        right = capture()
        right[0]["engine_revision"] = "candidate"
        right[2]["time_ns"] = 99999
        analyze.compare_runs(self.read(left), self.read(right))

    def test_checksum_mismatch_fails(self):
        left = self.read(capture())
        right = copy.deepcopy(left)
        right["frames"][0] = (1, 1, 124)
        with self.assertRaises(analyze.ReportError) as caught:
            analyze.compare_runs(left, right)
        self.assertEqual(caught.exception.outcome, "fail")

    def test_comparison_requires_matching_inputs(self):
        left = self.read(capture())
        for key in ("input_id", "game", "map", "config_id"):
            right = copy.deepcopy(left)
            right["header"][key] = "other"
            with self.assertRaises(analyze.ReportError) as caught:
                analyze.compare_runs(left, right)
            self.assertEqual(caught.exception.outcome, "incomplete")

    def test_comparison_cannot_pass_without_checksum_or_coverage(self):
        left = self.read(capture())
        for frames in ([], [(1, 1, None)]):
            right = copy.deepcopy(left)
            right["frames"] = frames
            with self.assertRaises(analyze.ReportError):
                analyze.compare_runs(left, right)


class ContractTests(unittest.TestCase):
    def event_manifest(self):
        return {"schema": 1, "events": [{"name": "UnitCreated", "properties": ["MANAGED_BIT"], "category": "publication_notification"}]}

    def test_new_event_requires_explicit_classification(self):
        source = "SETUP_EVENT(UnitCreated, MANAGED_BIT)\nSETUP_EVENT(NewEvent, MANAGED_BIT)\n"
        self.assertIn("unclassified event: NewEvent", check_contracts.check_events(source, self.event_manifest()))

    def test_control_event_cannot_be_published(self):
        manifest = self.event_manifest()
        manifest["events"][0]["properties"].append("CONTROL_BIT")
        errors = check_contracts.check_events("SETUP_EVENT(UnitCreated, MANAGED_BIT | CONTROL_BIT)", manifest)
        self.assertTrue(any("control event" in e for e in errors))

    def test_changed_properties_fail(self):
        errors = check_contracts.check_events("SETUP_EVENT(UnitCreated, MANAGED_BIT | UNSYNCED_BIT)", self.event_manifest())
        self.assertIn("changed event properties: UnitCreated", errors)

    def test_stale_and_duplicate_classification(self):
        manifest = self.event_manifest()
        manifest["events"].append(copy.deepcopy(manifest["events"][0]))
        errors = check_contracts.check_events("SETUP_EVENT(NewEvent, MANAGED_BIT)", manifest)
        self.assertIn("duplicate classification: UnitCreated", errors)
        self.assertIn("stale classification: UnitCreated", errors)

    def test_hook_presence_and_fragment(self):
        manifest = {"schema": 1, "hooks": [{"id": "test-hook", "file": "rts/Game.cpp", "symbol": "Update",
            "kind": "runtime_call", "reason": "observe", "ordering": "after tick",
            "fragment": "Observe();", "tests": ["testFixture"], "upstreamable": False}]}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "rts").mkdir()
            source = root / "rts/Game.cpp"
            source.write_text("// VKFUN-HOOK(test-hook)\nObserve();\n")
            self.assertEqual(check_contracts.check_hooks(root, manifest), [])
            source.write_text("// VKFUN-HOOK(test-hook)\nOther();\n")
            self.assertTrue(any("fragment missing" in e for e in check_contracts.check_hooks(root, manifest)))
            source.write_text("Observe();\n")
            self.assertTrue(any("found 0" in e for e in check_contracts.check_hooks(root, manifest)))
            source.write_text("// VKFUN-HOOK(test-hook)\nObserve();\n// VKFUN-HOOK(test-hook)\nObserve();\n")
            self.assertTrue(any("found 2" in e for e in check_contracts.check_hooks(root, manifest)))
            self.assertTrue(any("unregistered hook" in e for e in check_contracts.check_hooks(root, {"schema": 1, "hooks": []})))


if __name__ == "__main__":
    unittest.main()
