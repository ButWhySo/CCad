"""No-network contract for signal-driven, cached memory context retrieval."""

# Runtime path insertion below intentionally resolves modules from src/ccad_agent.
# pyright: reportMissingImports=false

import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from context_broker import (ContextBroker, extract_context_signals,
                            memory_exposure_manifest,
                            memory_retrieval_trace_metadata,
                            project_retrieval_signals)
from context_package import build_context_package
from memory_manager import MemoryManager
from memory_store import MemoryStore


class ContextBrokerTests(unittest.TestCase):
    def test_memory_retrieval_trace_metadata_is_safe_scalar_and_complete(self):
        metadata = memory_retrieval_trace_metadata({
            "cache_hit": True,
            "memory_chars": 128,
            "memory_token_budget": 1000,
            "memories": [{"id": "private-memory-id", "content": "private text"}],
            "memory_retrieval_status": {
                "status": "partial", "reason": "untrusted-private-reason",
                "channels": {"lexical": "ready", "semantic": "unavailable"}},
            "manifest": {"tiers": {"ltm": {"enabled": True},
                                    "episodic": {"enabled": False}},
                         "available_tier_count": 1,
                         "semantic_retrieval_ready": False},
            "memory_retrieval": [{
                "entry_id": "private-memory-id",
                "inclusion_channels": ["automatic_retrieval", "memory_summary"]}],
        })

        self.assertTrue(all(type(value) is str for value in metadata.values()))
        self.assertEqual(metadata["retrieval_status"], "partial")
        self.assertEqual(metadata["retrieval_lexical_status"], "ready")
        self.assertEqual(metadata["retrieval_semantic_status"], "unavailable")
        self.assertEqual(metadata["retrieved_record_count"], "1")
        self.assertEqual(metadata["candidate_automatic_retrieval_count"], "1")
        self.assertEqual(metadata["candidate_memory_summary_count"], "1")
        self.assertEqual(metadata["candidate_explicit_deep_retrieval_count"], "0")
        self.assertNotIn("private", json.dumps(metadata))
        manifest = json.loads(metadata["memory_exposure"])
        self.assertEqual(manifest[0]["inclusion_channels"],
                         ["automatic_retrieval", "memory_summary"])
        self.assertNotEqual(manifest[0]["memory_key"], "private-memory-id")

    def test_memory_summary_carries_stable_fact_content_without_task_or_secret_data(self):
        summary = ContextBroker._summary([
            {"id": "preference", "tier": "ltm", "scope": "project",
             "kind": "preference", "importance": 5,
             "title": "Ground return", "content":
             "Keep the GND return path short near U3."},
            {"id": "task", "tier": "stm", "scope": "task",
             "kind": "constraint", "importance": 5,
             "title": "Temporary target", "content": "Place the test pad at 4 mm."},
            {"id": "secret", "tier": "episodic", "scope": "user",
             "kind": "preference", "importance": 5,
             "title": "api_key=private-value", "content": "Keep layout simple."},
            {"id": "ephemeral", "tier": "episodic", "scope": "user",
             "kind": "fact", "importance": 1,
             "title": "Temporary note", "content": "Unverified temporary detail."},
            {"id": "important", "tier": "episodic", "scope": "user",
             "kind": "fact", "importance": 5,
             "title": "Stable constraint", "content": "Keep power return copper continuous."},
        ])

        self.assertIn("Keep the GND return path short near U3", summary)
        self.assertNotIn("Place the test pad", summary)
        self.assertNotIn("private-value", summary)
        self.assertNotIn("Unverified temporary detail", summary)
        self.assertIn("Keep power return copper continuous", summary)
        self.assertLessEqual(len(summary), ContextBroker.MAX_MEMORY_SUMMARY_CHARS)
        package = build_context_package({}, [], [], char_limit=4096,
                                       memory_summary=summary)
        self.assertIn("Keep the GND return path short near U3", package["content"])
        expanded = ContextBroker._summary([
            {"tier": "ltm", "kind": "preference", "importance": 3,
             "title": f"Rule {index}", "content": "x " * 200}
            for index in range(8)
        ])
        self.assertLessEqual(len(expanded), ContextBroker.MAX_MEMORY_SUMMARY_CHARS)

    def test_memory_context_channels_distinguish_automatic_summary_and_deep_search(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    task_id="task", thread_id="thread",
                                    project_id="project")
            manager.configure({"ltm": True, "episodic": True})
            stable = manager.add(
                "Keep the GND return path continuous around U3",
                tier="ltm", kind="preference", importance=5,
                title="GND return rule")
            routine = manager.add(
                "Place the test point beside connector J4",
                tier="episodic", kind="fact", importance=1,
                title="Test point placement")
            broker = ContextBroker()
            context = broker.prepare(
                manager, thread_id="thread", project_revision="rev-a",
                user_request="Review GND return near U3 and test point at J4")
            initial = {item["entry_id"]: item for item in context["memory_retrieval"]}
            self.assertEqual(initial[stable["id"]]["inclusion_channels"],
                             ["automatic_retrieval", "memory_summary"])
            self.assertEqual(initial[routine["id"]]["inclusion_channels"],
                             ["automatic_retrieval"])
            package = build_context_package(
                {}, context["memories"], [], char_limit=4096,
                memory_retrieval=context["memory_retrieval"],
                memory_summary=context["memory_summary"],
                memory_summary_entry_ids=context["memory_summary_entry_ids"])
            packaged = {item["memory_key"]: item for item in
                        package["metadata"]["memory_retrieval"]}
            self.assertEqual(package["metadata"]["memory_exposure_channel_counts"], {
                "automatic_retrieval": 2, "memory_summary": 1,
                "explicit_deep_retrieval": 0})
            self.assertNotIn(stable["id"], json.dumps(package["metadata"]))
            self.assertEqual(len(packaged), 2)

            refreshed = broker.refresh_memory(
                manager, context, "test point J4", reason="agent_requested_deeper_memory")
            deep = {item["entry_id"]: item for item in refreshed["memory_retrieval"]}
            self.assertEqual(deep[stable["id"]]["inclusion_channels"],
                             ["automatic_retrieval", "memory_summary",
                              "explicit_deep_retrieval"])
            self.assertEqual(deep[routine["id"]]["inclusion_channels"],
                             ["automatic_retrieval", "explicit_deep_retrieval"])
            safe_manifest = memory_exposure_manifest(refreshed["memory_retrieval"])
            self.assertEqual(len(safe_manifest), 2)
            self.assertTrue(all(set(item) == {"memory_key", "inclusion_channels"}
                                for item in safe_manifest))
            self.assertNotIn(stable["id"], json.dumps(safe_manifest))

    def test_signals_keep_design_identifiers_and_bound_sensitive_input(self):
        signals = extract_context_signals(
            "Move U3 on F.Cu and keep GND clear", active_editor="pcb",
            selected_objects=["T17", "V2"], workflow="placement_pass",
            active_layer="F.Cu", active_net="GND")
        self.assertIn("u3", signals["identifiers"])
        self.assertIn("f.cu", signals["identifiers"])
        self.assertIn("gnd", signals["identifiers"])
        self.assertIn("t17", signals["identifiers"])
        self.assertIn("F.Cu", signals["query"])
        self.assertIn("GND", signals["query"])
        self.assertLessEqual(len(signals["query"]), 2048)
        secret = extract_context_signals("api_key=not-a-real-secret")
        self.assertNotIn("not-a-real-secret", secret["query"])

    def test_live_editor_and_canvas_object_id_are_extracted_without_wrapper_prefix(self):
        editor, selected = project_retrieval_signals(json.dumps({
            "active_view": "schematic",
            "selection": {"canvas": "canvas:schematic", "items": [{
                "id": "canvas_object:U1", "object_id": "U1",
                "type": "Component"}]}}))
        self.assertEqual(editor, "schematic")
        self.assertEqual(selected, ["U1"])
        pcb_signals = extract_context_signals("inspect", active_editor="pcb")
        schematic_signals = extract_context_signals("inspect", active_editor="schematic")
        self.assertNotEqual(pcb_signals["digest"], schematic_signals["digest"])

    def test_context_broker_passes_active_editor_into_project_retrieval(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    task_id="task", thread_id="thread",
                                    project_id="project")
            manager.configure({"ltm": True})
            snapshot = {"id": "project-context-editor", "board": {
                "layers": [], "footprints": [{"reference": "R1", "value": "1k",
                    "position": {"x_nm": 0, "y_nm": 0}}], "tracks": [],
                "pads": [], "vias": [], "zones": []},
                "components": [{"id": "sch-r1", "reference": "R1",
                    "position": {"x_nm": 1_000_000, "y_nm": 1_000_000}}],
                "nets": [], "wires": [], "labels": []}
            context = ContextBroker().prepare(
                manager, thread_id="thread", project_revision="rev-1",
                user_request="inspect objects near 0,0", active_editor="schematic",
                project_snapshot=snapshot)
            self.assertEqual(context["signals"]["editor"], "schematic")
            self.assertEqual(context["project_retrieval"]["search_domain"], "schematic")

    def test_broker_retrieves_relevant_memory_once_and_invalidates_by_revision(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    task_id="task", thread_id="thread",
                                    project_id="project")
            manager.configure({"stm": True, "ltm": True, "episodic": True})
            relevant = manager.add("Keep the GND return path short near U3",
                                   tier="ltm", title="Routing preference")
            project_memory = manager.add(
                "Keep connector clearance above two millimeters on this board",
                tier="ltm", scope="project", title="Board clearance rule")
            manager.add("Use blue labels in presentation slides", tier="episodic",
                         title="Unrelated preference")
            broker = ContextBroker()
            first = broker.prepare(manager, thread_id="thread",
                                   project_revision="rev-a",
                                   user_request="Improve GND routing near U3 and connector clearance")
            second = broker.prepare(manager, thread_id="thread",
                                    project_revision="rev-a",
                                    user_request="Improve GND routing near U3 and connector clearance")
            self.assertEqual(first["version"], 1)
            self.assertFalse(first["cache_hit"])
            self.assertTrue(second["cache_hit"])
            self.assertEqual({entry["id"] for entry in first["memories"]},
                             {relevant["id"], project_memory["id"]})
            self.assertTrue(first["manifest"]["project_scope_available"])
            self.assertEqual(first["manifest"]["project_memory_count"], 1)
            changed = broker.prepare(manager, thread_id="thread",
                                     project_revision="rev-b",
                                     user_request="Improve GND routing near U3 and connector clearance")
            self.assertEqual(changed["version"], 2)
            self.assertFalse(changed["cache_hit"])

    def test_active_layer_and_net_invalidate_cache_and_seed_memory_retrieval(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    task_id="task", thread_id="thread",
                                    project_id="project")
            manager.configure({"ltm": True})
            layer_memory = manager.add(
                "Keep F.Cu signal routing clear of the board edge",
                tier="ltm", title="Front copper routing")
            net_memory = manager.add(
                "Preserve the GND return path on the selected copper net",
                tier="ltm", title="Ground return constraint")
            broker = ContextBroker()
            args = {"thread_id": "thread", "project_revision": "same-revision",
                    "user_request": "Inspect the current board setup"}
            first = broker.prepare(manager, **args, active_layer="F.Cu",
                                   active_net="GND")
            same_state = broker.prepare(manager, **args, active_layer="F.Cu",
                                        active_net="GND")
            changed_layer = broker.prepare(manager, **args, active_layer="B.Cu",
                                           active_net="GND")
            changed_net = broker.prepare(manager, **args, active_layer="B.Cu",
                                         active_net="VCC")
            self.assertTrue(same_state["cache_hit"])
            self.assertFalse(changed_layer["cache_hit"])
            self.assertFalse(changed_net["cache_hit"])
            self.assertIn(layer_memory["id"],
                          {item["id"] for item in first["memories"]})
            self.assertIn(net_memory["id"],
                          {item["id"] for item in first["memories"]})
            self.assertIn("F.Cu", first["signals"]["query"])
            self.assertIn("GND", first["signals"]["query"])
            self.assertNotEqual(first["signal_digest"],
                                changed_layer["signal_digest"])

    def test_project_memory_isolated_from_other_project_and_thread(self):
        with tempfile.TemporaryDirectory() as temp:
            store = MemoryStore(Path(temp) / "memory.json")
            manager = MemoryManager(store, thread_id="thread-a", project_id="project-a")
            manager.configure({"ltm": True})
            project_rule = manager.add("Keep USB shield connected at connector J1",
                                       tier="ltm", scope="project")
            manager.add("Keep USB shield connected at connector J1",
                        tier="ltm", scope="conversation")
            manager.set_identities(task_id="task", thread_id="thread-b",
                                   project_id="project-b", user_id="local-user")
            self.assertNotIn(project_rule["id"], {
                entry["id"] for entry in manager.retrieve("USB shield connector J1")})
            manager.set_identities(task_id="task", thread_id="thread-c",
                                   project_id="project-a", user_id="local-user")
            self.assertIn(project_rule["id"], {
                entry["id"] for entry in manager.retrieve("USB shield connector J1")})
            manager.disable("ltm")
            self.assertEqual(manager.retrieve("USB shield connector J1"), [])
            self.assertEqual(manager.state("ltm")["project_entries"], 1)

    def test_project_switch_rejects_refresh_from_stale_turn_context(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread-a", project_id="project-a")
            manager.configure({"ltm": True})
            manager.add("Keep USB shield attached near connector J2",
                        tier="ltm", scope="project")
            broker = ContextBroker()
            context = broker.prepare(manager, thread_id="thread-a",
                                     project_revision="r1",
                                     user_request="Review USB shield routing",
                                     project_id="project-a")
            manager.set_identities(task_id="task", thread_id="thread-a",
                                   project_id="project-b")
            with self.assertRaisesRegex(ValueError, "memory_context_project_changed"):
                broker.refresh_memory(manager, context, "USB shield",
                                      reason="project_switched_during_turn")

    def test_targeted_refresh_merges_deduplicates_and_budgets_memories(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread", project_id="project")
            manager.configure({"ltm": True, "episodic": True})
            first = manager.add("Preserve testpoint access during routing review",
                                tier="ltm", title="Testpoint constraint")
            second = manager.add("Keep connector clearance at least one millimeter",
                                 tier="episodic", title="Clearance preference")
            broker = ContextBroker(memory_token_budget=1000)
            context = broker.prepare(manager, thread_id="thread", project_revision="r1",
                                     user_request="Review routing and preserve testpoint access")
            extended = broker.refresh_memory(
                manager, context, "connector clearance preference", reason="new_net")
            ids = [entry["id"] for entry in extended["memories"]]
            self.assertIn(first["id"], ids)
            self.assertIn(second["id"], ids)
            self.assertEqual(len(ids), len(set(ids)))
            self.assertEqual(extended["version"], context["version"] + 1)
            self.assertEqual(extended["change_reason"], "new_net")
            self.assertLessEqual(extended["memory_chars"], 4000)
            self.assertEqual(extended["manifest"]["semantic_retrieval_ready"], False)
            self.assertGreaterEqual(extended["manifest"]["available_tier_count"], 1)

    def test_cache_is_thread_isolated_and_memory_writes_change_generation(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread-a")
            manager.configure({"ltm": True})
            broker = ContextBroker()
            initial = broker.prepare(manager, thread_id="thread-a", project_revision="r1",
                                     user_request="Keep clearance")
            self.assertFalse(broker.prepare(manager, thread_id="thread-b",
                                            project_revision="r1",
                                            user_request="Keep clearance")["cache_hit"])
            manager.add("Keep clearance around mounting holes", tier="ltm")
            updated = broker.prepare(manager, thread_id="thread-a", project_revision="r1",
                                     user_request="Keep clearance")
            self.assertFalse(updated["cache_hit"])
            self.assertGreater(updated["version"], initial["version"])
            self.assertEqual(len(updated["memories"]), 1)

    def test_disabled_tier_never_reuses_previously_cached_contents(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread")
            manager.configure({"ltm": True})
            manager.add("Keep the USB connector clear during board placement",
                        tier="ltm", title="Placement preference")
            broker = ContextBroker()
            loaded = broker.prepare(manager, thread_id="thread", project_revision="r1",
                                    user_request="USB connector placement")
            self.assertEqual(len(loaded["memories"]), 1)
            manager.disable("ltm")
            unloaded = broker.prepare(manager, thread_id="thread", project_revision="r1",
                                      user_request="USB connector placement")
            self.assertFalse(unloaded["cache_hit"])
            self.assertEqual(unloaded["memories"], [])
            self.assertEqual(unloaded["manifest"]["tiers"]["ltm"]["loaded_count"], 0)

    def test_automatic_memory_deduplicates_recent_context_and_thread_recap(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread", project_id="project")
            manager.configure({"ltm": True})
            recent_duplicate = manager.add(
                "Keep GND return path short near U3", title="GND routing preference",
                tier="ltm")
            recap_duplicate = manager.add(
                "Preserve connector clearance near J4 during routing",
                title="J4 clearance", tier="ltm")
            distinct = manager.add(
                "Keep reference designators readable after silkscreen edits", tier="ltm")
            broker = ContextBroker()
            context = broker.prepare(
                manager, thread_id="thread", project_revision="r1",
                user_request="Review GND return path, connector clearance, and silkscreen",
                recent_context=["Keep GND return path short near U3 while routing nearby nets"],
                thread_recap={"turns": [{
                    "user_request_summary": "Preserve connector clearance near J4 during routing on the crowded edge",
                    "assistant_summary": "Connector clearance near J4 was preserved during routing with other constraints",
                }]})
            selected = {entry["id"] for entry in context["memories"]}
            self.assertNotIn(recent_duplicate["id"], selected)
            self.assertNotIn(recap_duplicate["id"], selected)
            self.assertIn(distinct["id"], selected)
            refreshed = broker.refresh_memory(
                manager, context, "GND return path connector clearance silkscreen",
                reason="user_requested_more_memory")
            refreshed_ids = {entry["id"] for entry in refreshed["memories"]}
            self.assertNotIn(recent_duplicate["id"], refreshed_ids)
            self.assertNotIn(recap_duplicate["id"], refreshed_ids)
            changed_recap = broker.prepare(
                manager, thread_id="thread", project_revision="r1",
                user_request="Review GND return path, connector clearance, and silkscreen",
                recent_context=["Keep GND return path short near U3 while routing nearby nets"],
                thread_recap={"turns": [{
                    "user_request_summary": "A separate history task with different information",
                    "assistant_summary": "Completed unrelated work on an earlier task",
                }]})
            self.assertFalse(changed_recap["cache_hit"])
            self.assertIn(recap_duplicate["id"],
                          {entry["id"] for entry in changed_recap["memories"]})

    def test_manifest_explicitly_does_not_claim_missing_project_or_semantic_indices(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread")
            context = ContextBroker().prepare(
                manager, thread_id="thread", project_revision="r1",
                user_request="Inspect current board")
            manifest = context["manifest"]
            self.assertFalse(manifest["project_scope_available"])
            self.assertIsNone(manifest["project_memory_count"])
            self.assertFalse(manifest["semantic_retrieval_ready"])


if __name__ == "__main__":
    unittest.main()
