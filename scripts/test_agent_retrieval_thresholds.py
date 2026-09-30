"""Contract tests for model/task/dataset-scoped retrieval threshold analysis."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from analyze_agent_retrieval_thresholds import (  # noqa: E402
    analyze_scoped_thresholds,
    RetrievalThresholdScopeError,
)


def report(split, mode, rows, *, surface="project", digest="model-digest-a"):
    return {
        "schema_version": 1,
        "benchmark_version": "1.5.0",
        "dataset_id": "fixture-corpus",
        "dataset_version": "1.3.0",
        "dataset_sha256": "fixture-hash",
        "split": split,
        "project_retrieval_mode": mode if surface == "project" else "task_policy",
        "memory_retrieval_mode": mode if surface == "memory" else "not_applicable",
        "runtime": {"semantic_model_identity": {"digest": digest}},
        "cases": rows,
    }


def rows(split, *, semantic, surface="project"):
    scope_task = "memory_retrieval" if surface == "memory" else "project_entity"
    positive_ids = [f"positive-{split}-{index}" for index in range(4)]
    negative_ids = [f"negative-{split}-{index}" for index in range(3)]
    scores = ([0.86, 0.72, 0.66, 0.57] if split == "calibration" else
              [0.74, 0.69, 0.63, 0.58])
    false_scores = ([0.45, 0.39, 0.33] if split == "calibration" else
                    [0.83, 0.37, 0.31])
    result = []
    for index, case_id in enumerate(positive_ids):
        result.append({
            "case_id": case_id, "task": scope_task, "threshold_task": scope_task,
            "semantic_search_allowed": True, "expected_ids": [case_id],
            "result_ids": [], "semantic_candidates": (
                [{"id": case_id, "score": scores[index]}] if semantic else []),
        })
    for index, case_id in enumerate(negative_ids):
        result.append({
            "case_id": case_id, "task": scope_task if surface == "memory" else "hard_negative",
            "threshold_task": scope_task, "semantic_search_allowed": True,
            "expected_ids": [], "result_ids": [],
            "semantic_candidates": (
                [{"id": f"distractor-{index}", "score": false_scores[index]}]
                if semantic else []),
        })
    return result


class ScopedThresholdTests(unittest.TestCase):
    def _analyze(self, *, held_digest="model-digest-a", surface="project"):
        return analyze_scoped_thresholds(
            report("calibration", "semantic", rows("calibration", semantic=True,
                                                      surface=surface), surface=surface),
            report("held_out", "semantic", rows("held_out", semantic=True,
                                                  surface=surface), surface=surface),
            report("calibration", "lexical", rows("calibration", semantic=False,
                                                    surface=surface), surface=surface),
            report("held_out", "lexical", rows("held_out", semantic=False,
                                                surface=surface), surface=surface),
            retrieval_surface=surface,
        )

    def test_calibrates_on_calibration_and_detects_held_out_false_positive_cost(self):
        result = self._analyze()
        self.assertEqual(result["profiles"][0]["model_digest"], "model-digest-a")
        self.assertEqual(result["profiles"][0]["task"], "project_entity")
        self.assertEqual(result["profiles"][0]["calibration"]["incremental_recall"], 1.0)
        self.assertGreater(
            result["profiles"][0]["held_out"]["incremental_hard_negative_false_positive_rate"],
            0.0,
        )
        self.assertGreater(
            result["profiles"][0]["held_out_semantic_all_candidates"][
                "hard_negative_false_positive_cases"], 0,
        )
        self.assertEqual(result["profiles"][0]["held_out"]["status"],
                         "false_positive_cost_increased")

    def test_profile_identity_includes_model_task_dataset_and_evaluation(self):
        result = self._analyze(surface="memory")
        profile = result["profiles"][0]
        self.assertEqual(profile["retrieval_surface"], "memory")
        self.assertEqual(profile["dataset_version"], "1.3.0")
        self.assertEqual(profile["dataset_sha256"], "fixture-hash")
        self.assertEqual(profile["evaluation_version"], "1.0.0")
        self.assertEqual(profile["threshold_kind"],
                         "post_retriever_candidate_score_cutoff")
        self.assertEqual(result["production_threshold_changed"], False)
        self.assertEqual(len(profile["profile_id"]), 64)

    def test_mismatched_model_digest_is_rejected(self):
        calibration_semantic = report("calibration", "semantic",
                                      rows("calibration", semantic=True))
        held_semantic = report("held_out", "semantic",
                               rows("held_out", semantic=True), digest="model-digest-b")
        calibration_lexical = report("calibration", "lexical",
                                     rows("calibration", semantic=False))
        held_lexical = report("held_out", "lexical",
                              rows("held_out", semantic=False))
        with self.assertRaisesRegex(RetrievalThresholdScopeError, "scope_mismatch"):
            analyze_scoped_thresholds(calibration_semantic, held_semantic,
                                      calibration_lexical, held_lexical,
                                      retrieval_surface="project")

    def test_no_calibration_gain_emits_no_cutoff_or_promotion(self):
        calibration_semantic = report(
            "calibration", "semantic", rows("calibration", semantic=True))
        held_semantic = report("held_out", "semantic", rows("held_out", semantic=True))
        calibration_lexical = report(
            "calibration", "lexical", rows("calibration", semantic=False))
        held_lexical = report("held_out", "lexical", rows("held_out", semantic=False))
        for benchmark_report in (calibration_lexical, held_lexical):
            for row in benchmark_report["cases"]:
                row["result_ids"] = list(row["expected_ids"])
        result = analyze_scoped_thresholds(
            calibration_semantic, held_semantic, calibration_lexical, held_lexical,
            retrieval_surface="project")
        profile = result["profiles"][0]
        self.assertEqual(profile["status"], "no_calibration_incremental_gain")
        self.assertIsNone(profile["selected_threshold"])
        self.assertFalse(result["production_threshold_changed"])


if __name__ == "__main__":
    unittest.main()
