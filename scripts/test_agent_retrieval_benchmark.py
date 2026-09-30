"""Behavior contracts for the real CCad retrieval evaluation runner."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from benchmark_agent_retrieval import (  # noqa: E402
    aggregate_metrics,
    evaluate_case,
    memory_channels_for_mode,
    project_channels_for_mode,
    run_benchmark,
)
from retrieval_contracts import RetrievalChannel  # noqa: E402


class RetrievalMetricsTests(unittest.TestCase):
    def test_project_channel_modes_are_selected_before_retrieval(self):
        self.assertEqual(project_channels_for_mode("design_intent", True, "exact"),
                         (RetrievalChannel.EXACT,))
        self.assertEqual(project_channels_for_mode("design_intent", True, "lexical"),
                         (RetrievalChannel.LEXICAL,))
        self.assertEqual(project_channels_for_mode("design_intent", True, "semantic"),
                         (RetrievalChannel.SEMANTIC,))
        self.assertEqual(project_channels_for_mode("design_intent", True, "hybrid"),
                         (RetrievalChannel.EXACT, RetrievalChannel.LEXICAL,
                          RetrievalChannel.SEMANTIC))
        self.assertEqual(project_channels_for_mode("project_entity", True, "full"),
                         (RetrievalChannel.EXACT, RetrievalChannel.LEXICAL,
                          RetrievalChannel.SEMANTIC, RetrievalChannel.GRAPH,
                          RetrievalChannel.SPATIAL))
        self.assertEqual(project_channels_for_mode("exact_cad", False, "semantic"), ())
        self.assertEqual(project_channels_for_mode("design_intent", True, "task_policy"),
                         (RetrievalChannel.SEMANTIC, RetrievalChannel.LEXICAL))
        with self.assertRaisesRegex(ValueError, "project_retrieval_mode_invalid"):
            project_channels_for_mode("design_intent", True, "invented")

    def test_memory_modes_execute_only_requested_channels(self):
        self.assertEqual(memory_channels_for_mode(True, "lexical"),
                         (RetrievalChannel.LEXICAL,))
        self.assertEqual(memory_channels_for_mode(True, "semantic"),
                         (RetrievalChannel.SEMANTIC,))
        self.assertEqual(memory_channels_for_mode(True, "hybrid"),
                         (RetrievalChannel.LEXICAL, RetrievalChannel.SEMANTIC))
        self.assertEqual(memory_channels_for_mode(False, "semantic"), ())
        self.assertEqual(memory_channels_for_mode(False, "hybrid"),
                         (RetrievalChannel.LEXICAL,))
        with self.assertRaisesRegex(ValueError, "memory_retrieval_mode_invalid"):
            memory_channels_for_mode(True, "exact")

    def test_ranked_metrics_use_relevant_ids_and_cutoff(self):
        metrics = evaluate_case(
            ["x", "b", "a", "noise"], {"a", "b"}, {"noise"}, k_values=(1, 3, 5))
        self.assertEqual(metrics["recall_at_1"], 0.0)
        self.assertEqual(metrics["recall_at_3"], 1.0)
        self.assertAlmostEqual(metrics["mrr"], 0.5)
        self.assertAlmostEqual(metrics["ndcg_at_3"], 0.6934264, places=6)
        self.assertAlmostEqual(metrics["precision_at_3"], 2 / 3)
        self.assertEqual(metrics["distractor_hits"], 1)

    def test_no_answer_negative_counts_returned_candidates_as_false_positive(self):
        metrics = evaluate_case(["related", "unrelated"], set(), {"related"})
        self.assertEqual(metrics["false_positive"], 1)
        self.assertEqual(metrics["hard_negative_rejected"], 0)
        clean = evaluate_case([], set(), {"related"})
        self.assertEqual(clean["false_positive"], 0)
        self.assertEqual(clean["hard_negative_rejected"], 1)

    def test_aggregates_are_macro_averaged_and_empty_tasks_are_not_invented(self):
        result = aggregate_metrics([
            {"task": "exact_cad", "metrics": {"mrr": 1.0, "recall_at_1": 1.0}},
            {"task": "exact_cad", "metrics": {"mrr": 0.0, "recall_at_1": 0.0}},
            {"task": "memory", "metrics": {"mrr": 0.5, "recall_at_1": 0.0}},
        ])
        self.assertEqual(result["exact_cad"]["case_count"], 2)
        self.assertEqual(result["exact_cad"]["mrr"], 0.5)
        self.assertEqual(result["memory"]["mrr"], 0.5)
        self.assertNotIn("semantic_paraphrase", result)

    def test_benchmark_executes_real_indexes_and_returns_revisioned_results(self):
        corpus = json.loads((ROOT / "scripts" / "fixtures" /
                             "agent_retrieval_dataset_v1.json").read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as temp:
            report = run_benchmark(corpus, split="held_out", work_dir=Path(temp),
                                   warmups=0, repetitions=2)
        self.assertEqual(report["dataset_version"], corpus["dataset_version"])
        self.assertEqual(report["benchmark_version"], "1.4.0")
        self.assertEqual(report["split"], "held_out")
        self.assertGreater(report["summary"]["case_count"], 0)
        history = next(row for row in report["cases"]
                       if row["task"] == "historical_turn_record")
        self.assertTrue(history["result_ids"])
        self.assertEqual(history["metrics"]["recall_at_1"], 1.0)
        self.assertIn("historical_turn_lexical", history["channels_available_on_hits"])
        self.assertTrue(any(row["task"] == "memory_retrieval" for row in report["cases"]))
        self.assertTrue(all(row["status"] in {"measured", "unavailable"}
                            for row in report["cases"]))
        semantic_allowed = [row for row in report["cases"]
                            if row["semantic_search_allowed"]]
        self.assertTrue(semantic_allowed)
        self.assertTrue(all("semantic" in row["requested_channels"]
                            for row in semantic_allowed))
        self.assertEqual(report["summary"]["semantic_eligible_case_count"],
                         len(semantic_allowed))
        semantic_disallowed = [row for row in report["cases"]
                               if not row["semantic_search_allowed"] and
                               row["task"] != "historical_turn_record"]
        self.assertTrue(all("semantic" not in row["requested_channels"]
                            for row in semantic_disallowed))
        self.assertFalse(any(identity.startswith("unmapped:")
                             for row in report["cases"] for identity in row["result_ids"]))
        project_cases = [row for row in report["cases"]
                         if row["task"] in {"exact_cad", "lexical_engineering",
                                            "graph_relationship", "spatial_geometry",
                                            "project_entity"}]
        self.assertTrue(all(row["source_revision"] and row["result_ids"] is not None
                            for row in project_cases))
        self.assertTrue(all(row["timing_ms"]["p50"] >= 0 for row in report["cases"]))
        self.assertEqual(report["systems"]["product_startup_latency_ms"], None)
        self.assertEqual(report["systems"]["embedding_latency_ms"]["status"],
                         "not_requested")
        self.assertIsNone(report["runtime"]["semantic_model_identity"])
        self.assertEqual(report["systems"]["backend_failure_recovery"]["status"],
                         "not_measured_no_local_semantic_backend")
        self.assertEqual(report["systems"]["follow_up_tool_calls"],
                         "not_measured_offline_retrieval_only")

    def test_semantic_mode_excludes_policy_disallowed_cases_without_scoring_misses(self):
        corpus = json.loads((ROOT / "scripts" / "fixtures" /
                             "agent_retrieval_dataset_v1.json").read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as temp:
            report = run_benchmark(corpus, split="held_out", work_dir=Path(temp),
                                   warmups=0, repetitions=1,
                                   project_retrieval_mode="semantic")
        excluded = [row for row in report["cases"]
                    if row["status"] == "excluded_by_policy"]
        self.assertTrue(excluded)
        self.assertTrue(all(not row["semantic_search_allowed"] and
                            row["retrieval_status"] == "not_requested_policy"
                            for row in excluded))
        self.assertEqual(report["project_retrieval_mode"], "semantic")
        self.assertEqual(report["summary"]["measured_case_count"],
                         len(report["cases"]) - len(excluded))
        self.assertIn("semantic_paraphrase", report["project_mode_quality_by_task"])

    def test_memory_semantic_mode_excludes_policy_disallowed_cases(self):
        corpus = json.loads((ROOT / "scripts" / "fixtures" /
                             "agent_retrieval_dataset_v1.json").read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as temp:
            report = run_benchmark(corpus, split="held_out", work_dir=Path(temp),
                                   warmups=0, repetitions=1,
                                   memory_retrieval_mode="semantic")
        rows = [row for row in report["cases"] if row["task"] == "memory_retrieval"]
        self.assertTrue(rows)
        self.assertTrue(all(row["memory_retrieval_mode"] == "semantic" for row in rows))
        self.assertTrue(all(row["status"] == ("measured" if row["semantic_search_allowed"]
                                               else "excluded_by_policy") for row in rows))
        self.assertTrue(all(row["requested_channels"] == (["semantic"]
                                                            if row["semantic_search_allowed"]
                                                            else []) for row in rows))
        self.assertIn("memory_retrieval", report["memory_mode_quality_by_task"])


if __name__ == "__main__":
    unittest.main()
