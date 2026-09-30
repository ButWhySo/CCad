"""Validate the versioned, split-safe CCad retrieval evaluation corpus."""

from __future__ import annotations

import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATASET_PATH = ROOT / "scripts" / "fixtures" / "agent_retrieval_dataset_v1.json"
TASKS = {
    "exact_cad", "lexical_engineering", "semantic_paraphrase", "hard_negative",
    "component_function", "design_intent", "graph_relationship", "spatial_geometry",
    "memory", "historical_turn_record",
}
SPLITS = {"calibration", "held_out"}


class RetrievalDatasetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dataset = json.loads(DATASET_PATH.read_text(encoding="utf-8"))

    def test_dataset_is_versioned_and_has_distinct_task_splits(self):
        dataset = self.dataset
        self.assertEqual(dataset["schema_version"], 1)
        self.assertRegex(dataset["dataset_version"], r"^\d+\.\d+\.\d+$")
        self.assertEqual({case["split"] for case in dataset["cases"]}, SPLITS)
        self.assertEqual({case["task"] for case in dataset["cases"]}, TASKS)
        for split in SPLITS:
            self.assertEqual({case["task"] for case in dataset["cases"]
                              if case["split"] == split}, TASKS)

    def test_each_case_is_source_revisioned_explainable_and_labelled(self):
        dataset = self.dataset
        ids = set()
        queries_by_task_split = {}
        expected_by_task_split = {}
        for case in dataset["cases"]:
            with self.subTest(case=case.get("id")):
                self.assertTrue(case["id"] not in ids)
                ids.add(case["id"])
                self.assertTrue(case["query"].strip())
                self.assertTrue(case["rationale"].strip())
                self.assertIsInstance(case["semantic_search_allowed"], bool)
                self.assertTrue(case["distractor_ids"])
                self.assertFalse(set(case["expected_ids"]) & set(case["distractor_ids"]))
                fixture = dataset["fixtures"][case["fixture"]]
                self.assertTrue(fixture["source_revision"])
                if "board_size_mm" in fixture:
                    self.assertTrue(fixture["project_revision"])
                declared_ids = set(fixture["entity_ids"])
                self.assertTrue(set(case["expected_ids"]) <= declared_ids)
                self.assertTrue(set(case["distractor_ids"]) <= declared_ids)
                key = (case["task"], case["split"])
                queries_by_task_split.setdefault(key, set()).add(case["query"].casefold())
                expected_by_task_split.setdefault(key, set()).update(case["expected_ids"])

        for task in TASKS:
            self.assertFalse(queries_by_task_split[(task, "calibration")] &
                             queries_by_task_split[(task, "held_out")])
            self.assertFalse(expected_by_task_split[(task, "calibration")] &
                             expected_by_task_split[(task, "held_out")])

    def test_corpus_covers_board_scale_repeated_classes_and_nonsemantic_queries(self):
        fixtures = self.dataset["fixtures"]
        board_sizes = {tuple(fixtures[name]["board_size_mm"])
                       for name in ("power_small", "controller_large")}
        self.assertGreaterEqual(len(board_sizes), 2)
        self.assertTrue(any(count > 1
                            for name in ("power_small", "controller_large")
                            for count in fixtures[name]["repeated_component_classes"].values()))
        self.assertTrue(any(not case["semantic_search_allowed"]
                            for case in self.dataset["cases"]))
        negatives = [case for case in self.dataset["cases"]
                     if case["task"] == "hard_negative"]
        self.assertTrue(negatives)
        self.assertTrue(all(not case["expected_ids"] and
                            case["semantic_search_allowed"] for case in negatives))
        semantic = [case for case in self.dataset["cases"]
                    if case["semantic_search_allowed"]]
        for split in SPLITS:
            self.assertTrue(any(case["split"] == split and
                                case["task"] == "component_function" for case in semantic))
            self.assertTrue(any(case["split"] == split and
                                case["task"] == "design_intent" for case in semantic))
            self.assertTrue(any(case["split"] == split and
                                case["task"] == "hard_negative" for case in semantic))

    def test_engineering_negatives_cover_distinct_false_inference_pairs(self):
        queries = " ".join(case["query"].casefold() for case in self.dataset["cases"]
                           if case["task"] == "hard_negative")
        for phrase in ("impedance", "differential", "switching", "clock", "gpio"):
            self.assertIn(phrase, queries)


if __name__ == "__main__":
    unittest.main()
