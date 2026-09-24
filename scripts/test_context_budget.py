"""Contracts for bounded context provenance and live request accounting."""

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


MODULE = Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "context_package.py"
SPEC = importlib.util.spec_from_file_location("ccad_context_package", MODULE)
assert SPEC is not None and SPEC.loader is not None
CONTEXT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CONTEXT)
AGENT_DIR = MODULE.parent


def _load_agent_module(name):
    spec = importlib.util.spec_from_file_location(name, AGENT_DIR / f"{name}.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


MEMORY_STORE = _load_agent_module("memory_store")
MEMORY_MANAGER = _load_agent_module("memory_manager")
MemoryManager = MEMORY_MANAGER.MemoryManager
MemoryStore = MEMORY_STORE.MemoryStore


class Message:
    def __init__(self, content):
        self.content = content


class ToolCallMessage(Message):
    def __init__(self, content, tool_calls):
        super().__init__(content)
        self.tool_calls = tool_calls


class Schema:
    def model_json_schema(self):
        return {"type": "object", "properties": {"x": {"type": "number"}}}


class Tool:
    name = "ccad_draw_line"
    description = "Draw a line through the CCad broker."
    args_schema = Schema()


class ContextBudgetTests(unittest.TestCase):
    def test_package_reports_memory_tiers_and_history_transport_separately(self):
        package = CONTEXT.build_context_package(
            '{"project":{"tracks":[1,2]}}',
            [{"id": "m1", "tier": "stm", "scope": "run", "content": "Keep vias clear."}],
            [Message("old turn")], char_limit=8192,
            memory_retrieval=[{"entry_id": "m1", "rank": 1, "tier": "stm", "query_overlap_terms": 2,
                               "namespace_hash": "ab12"}],
            memory_runtime={"stm": {"enabled": True, "runtime_entries": 1,
                                     "persistent_entries": 1, "loaded_into_process": True,
                                     "namespace_hash": "ab12"}})
        meta = package["metadata"]
        self.assertIn("retrieved_memory", meta["sources"])
        self.assertNotIn("recent_conversation", meta["sources"])
        self.assertFalse(meta["history_in_context_package"])
        self.assertTrue(meta["history_sent_as_provider_messages"])
        self.assertEqual(meta["memory_tier_counts"], {"stm": 1})
        self.assertTrue(meta["memory_runtime"]["stm"]["enabled"])
        self.assertEqual(meta["memory_retrieval"][0]["rank"], 1)
        self.assertIn("Keep vias clear", package["content"])

    def test_retrieved_turn_context_keeps_source_message_provenance(self):
        package = CONTEXT.build_context_package("{}", [], [], char_limit=4096,
            turn_records=[{"turn_id": "turn-1", "source_message_ids": ["msg-1"],
                           "user_request_summary": "Place U3 beside USB",
                           "assistant_summary": "Placed U3", "outcome": "completed",
                           "tool_ids": ["project.inspect"]}])
        envelope = json.loads(package["content"].split("\n", 1)[1])
        self.assertEqual(package["metadata"]["prior_turn_count"], 1)
        self.assertIn("retrieved_conversation_turns", package["metadata"]["sources"])
        self.assertEqual(envelope["prior_turns"][0]["source_message_ids"], ["msg-1"])
        self.assertIn("Place U3 beside USB", package["content"])

    def test_thread_recap_is_injected_with_turn_provenance(self):
        package = CONTEXT.build_context_package("{}", [], [], char_limit=4096,
            thread_recap={"thread_id": "thread-1", "source_turn_ids": ["turn-1"],
                          "turns": [{"turn_id": "turn-1",
                                     "user_request_summary": "Fix USB route",
                                     "assistant_summary": "Moved T1",
                                     "outcome": "completed",
                                     "tool_ids": ["project.route"]}]})
        envelope = json.loads(package["content"].split("\n", 1)[1])
        self.assertEqual(package["metadata"]["thread_recap_turn_count"], 1)
        self.assertIn("thread_recap", package["metadata"]["sources"])
        self.assertEqual(envelope["thread_recap"]["source_turn_ids"], ["turn-1"])

    def test_overflow_keeps_valid_json_and_reports_exact_omissions(self):
        package = CONTEXT.build_context_package(
            json.dumps({"project": {"tracks": [{"id": "T1"}],
                        "notes": "board snapshot " * 400}}),
            [{"id": "best", "tier": "ltm", "content": "USB power constraints"},
             {"id": "lower", "tier": "episodic", "content": "x" * 800}],
            [], char_limit=1024,
            memory_retrieval=[
                {"entry_id": "best", "rank": 1, "tier": "ltm",
                 "query_overlap_terms": 3, "namespace_hash": "t1"},
                {"entry_id": "lower", "rank": 2, "tier": "episodic",
                 "query_overlap_terms": 1, "namespace_hash": "p1"},
            ])
        meta = package["metadata"]
        envelope = json.loads(package["content"].split("\n", 1)[1])
        self.assertLessEqual(meta["content_size"], meta["context_limit"])
        self.assertTrue(meta["truncated"])
        self.assertTrue(meta["project_snapshot_omitted"])
        self.assertEqual(meta["project_snapshot_chars"], 0)
        self.assertGreater(meta["project_source_chars"], 0)
        self.assertEqual(meta["memory_entry_count"], 1)
        self.assertEqual(meta["omitted_memory_entry_count"], 1)
        self.assertEqual(meta["memory_retrieval"][0]["rank"], 1)
        self.assertEqual(meta["memory_retrieval"][0]["tier"], "ltm")
        self.assertTrue(envelope["project"]["snapshot_omitted"])
        self.assertEqual(envelope["constraints"]["omitted_memory_entry_count"], 1)
        self.assertIn("USB power constraints", package["content"])
        self.assertNotIn("board snapshot", package["content"])
        report = CONTEXT.build_provider_request_report(
            "System rules\nContext: " + package["content"], [], [],
            provider="local", model="unknown",
            context_content=package["content"], context_metadata=meta)
        self.assertEqual(report["omitted_memory_entry_count"], 1)
        self.assertEqual(report["context_package_limit_chars"], 1024)
        self.assertEqual(report["project_counts"]["tracks"], 1)
        explanation = CONTEXT.format_large_context_explanation(report, meta)
        for expected in ("STM is transient and scoped to an explicit `/task start`",
                         "stored scope label is descriptive metadata",
                         "Memory capture remains explicit",
                         "unsafe legacy records are excluded",
                         "1 omitted by package budget",
                         "exact design work must first call project.state",
                         "Model context limit: unavailable"):
            self.assertIn(expected, explanation)
        self.assertNotIn("USB power constraints", explanation)
        self.assertNotIn("board snapshot", explanation)

        preview = CONTEXT.format_large_context_explanation(report, meta, mode="preview")
        self.assertIn("Local context preview", preview)
        self.assertIn("no provider request was sent", preview)
        self.assertNotIn("USB power constraints", preview)
        self.assertNotIn("board snapshot", preview)

    def test_request_report_counts_live_components_without_content(self):
        context = CONTEXT.build_context_package(
            json.dumps({"project": {"notes": "x" * 4000}}), [], [], char_limit=8192)
        report = CONTEXT.build_provider_request_report(
            "System rules\nContext: " + context["content"],
            [Message("prior conversation"),
             ToolCallMessage("", [{"name": "ccad_draw_line", "args": {
                 "x": "private-tool-coordinate"}}]),
             Message("current question")],
            [Tool()], provider="openrouter", model="model-x",
            context_content=context["content"], context_metadata=context["metadata"],
            large_context_threshold=512)
        components = report["components"]
        self.assertGreater(components["system_instructions"]["chars"], 0)
        self.assertGreater(components["project_snapshot"]["chars"], 0)
        self.assertEqual(components["conversation_messages"]["chars"],
                         len("prior conversationcurrent question"))
        self.assertGreater(components["prior_tool_call_payloads"]["chars"], 0)
        self.assertEqual(report["prior_tool_call_count"], 1)
        self.assertGreater(components["bound_tool_schemas"]["chars"], 0)
        self.assertEqual(report["model_context_limit"], None)
        self.assertEqual(report["model_context_limit_source"], "unavailable")
        self.assertTrue(report["large_context"])
        self.assertFalse(report["content_emitted"])
        self.assertFalse(report["secret_value_visible"])
        encoded_report = json.dumps(report)
        for private_content in ("prior conversation", "current question", "tracks",
                                "private-tool-coordinate"):
            self.assertNotIn(private_content, encoded_report)

    def test_non_text_payloads_are_explicitly_excluded_from_estimate(self):
        context = CONTEXT.build_context_package("{}", [], [], char_limit=1024)
        report = CONTEXT.build_provider_request_report(
            "System\nContext: " + context["content"],
            [Message([{"type": "image", "data": "private-image-payload"}])], [],
            provider="local", model="vision-model", context_content=context["content"],
            context_metadata=context["metadata"])
        self.assertEqual(report["multimodal_blocks_not_estimated"], 1)
        self.assertFalse(report["estimate_includes_all_payloads"])
        self.assertNotIn("private-image-payload", json.dumps(report))

    def test_memory_retrieval_reports_rank_tier_and_opaque_namespace(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = MemoryManager(
                MemoryStore(Path(directory) / "memory.json"),
                task_id="run-42", thread_id="thread-7", project_id="project-3")
            manager.configure({"stm": True, "ltm": True, "episodic": True})
            manager.add("USB power return rail", tier="stm")
            manager.add("USB power connector on F.Cu", tier="ltm")
            manager.add("USB project uses 5V input", tier="episodic")
            entries, provenance = manager.retrieve_with_metadata("USB power return")
            self.assertEqual(len(entries), 3)
            self.assertEqual([entry["tier"] for entry in entries],
                             ["stm", "ltm", "episodic"])
            self.assertEqual([item["rank"] for item in provenance], [1, 2, 3])
            self.assertEqual([item["query_overlap_terms"] for item in provenance],
                             [3, 2, 1])
            self.assertNotIn("thread-7", json.dumps(provenance))
            self.assertNotIn("run-42", json.dumps(provenance))
            self.assertEqual(manager.state("ltm")["persistent_entries"], 1)

    def test_disabled_memory_tier_is_not_injected(self):
        with tempfile.TemporaryDirectory() as directory:
            manager = MemoryManager(
                MemoryStore(Path(directory) / "memory.json"), thread_id="thread-7")
            manager.store.add("USB power detail", tier="ltm", namespace="thread-7")
            manager.configure({"stm": False, "ltm": False, "episodic": False})
            entries, provenance = manager.retrieve_with_metadata("USB power")
            self.assertEqual(entries, [])
            self.assertEqual(provenance, [])


if __name__ == "__main__":
    unittest.main()
