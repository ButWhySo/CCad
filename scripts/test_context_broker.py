"""No-network contract for signal-driven, cached memory context retrieval."""

# Runtime path insertion below intentionally resolves modules from src/ccad_agent.
# pyright: reportMissingImports=false

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from context_broker import ContextBroker, extract_context_signals
from memory_manager import MemoryManager
from memory_store import MemoryStore


class ContextBrokerTests(unittest.TestCase):
    def test_signals_keep_design_identifiers_and_bound_sensitive_input(self):
        signals = extract_context_signals(
            "Move U3 on F.Cu and keep GND clear", active_editor="pcb",
            selected_objects=["T17", "V2"], workflow="placement_pass")
        self.assertIn("u3", signals["identifiers"])
        self.assertIn("f.cu", signals["identifiers"])
        self.assertIn("gnd", signals["identifiers"])
        self.assertIn("t17", signals["identifiers"])
        self.assertLessEqual(len(signals["query"]), 2048)
        secret = extract_context_signals("api_key=not-a-real-secret")
        self.assertNotIn("not-a-real-secret", secret["query"])

    def test_broker_retrieves_relevant_memory_once_and_invalidates_by_revision(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    task_id="task", thread_id="thread",
                                    project_id="project")
            manager.configure({"stm": True, "ltm": True, "episodic": True})
            relevant = manager.add("Keep the GND return path short near U3",
                                   tier="ltm", title="Routing preference")
            manager.add("Use blue labels in presentation slides", tier="episodic",
                         title="Unrelated preference")
            broker = ContextBroker()
            first = broker.prepare(manager, thread_id="thread",
                                   project_revision="rev-a",
                                   user_request="Improve GND routing near U3")
            second = broker.prepare(manager, thread_id="thread",
                                    project_revision="rev-a",
                                    user_request="Improve GND routing near U3")
            self.assertEqual(first["version"], 1)
            self.assertFalse(first["cache_hit"])
            self.assertTrue(second["cache_hit"])
            self.assertEqual([entry["id"] for entry in first["memories"]],
                             [relevant["id"]])
            changed = broker.prepare(manager, thread_id="thread",
                                     project_revision="rev-b",
                                     user_request="Improve GND routing near U3")
            self.assertEqual(changed["version"], 2)
            self.assertFalse(changed["cache_hit"])

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
