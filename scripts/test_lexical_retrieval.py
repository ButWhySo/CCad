"""Deterministic lexical retrieval contracts shared by memory and history."""

import importlib.util
import unittest
from pathlib import Path

MODULE = Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "lexical_retrieval.py"
SPEC = importlib.util.spec_from_file_location("ccad_lexical_retrieval", MODULE)
assert SPEC is not None and SPEC.loader is not None
LEXICAL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LEXICAL)


class Bm25Tests(unittest.TestCase):
    def test_ranks_relevant_document_and_returns_match_provenance(self):
        ranked = LEXICAL.rank_documents(
            "GND return clearance U3",
            [{"id": "unrelated", "text": "GND"},
             {"id": "relevant", "text": "GND return path clearance around U3"}],
            text_key="text")
        self.assertEqual(ranked[0]["document"]["id"], "relevant")
        self.assertEqual(ranked[1]["document"]["id"], "unrelated")
        self.assertGreater(ranked[0]["score"], ranked[1]["score"])
        self.assertEqual(set(ranked[0]["matched_terms"]), {"gnd", "return", "clearance", "u3"})

    def test_empty_query_and_nonmatching_docs_do_not_create_results(self):
        docs = [{"text": "USB connector"}]
        self.assertEqual(LEXICAL.rank_documents("the and", docs, text_key="text"), [])
        self.assertEqual(LEXICAL.rank_documents("GND", docs, text_key="text"), [])

    def test_minimum_match_floor_drops_weak_history_hits(self):
        ranked = LEXICAL.rank_documents("GND return clearance U3",
            [{"id": "weak", "text": "GND"},
             {"id": "strong", "text": "GND return clearance"}],
            text_key="text", min_matches=2)
        self.assertEqual([item["document"]["id"] for item in ranked], ["strong"])

    def test_reciprocal_rank_fusion_preserves_cross_field_matches(self):
        document = {"id": "shared"}
        title_ranked = [{"document": document, "score": 2.0, "matched_terms": ["gnd"]}]
        content_ranked = [{"document": document, "score": 1.0, "matched_terms": ["return"]}]
        fused = LEXICAL.fuse_rankings(
            {"title": title_ranked, "content": content_ranked},
            weights={"title": 1.2, "content": 1.0})
        self.assertEqual(len(fused), 1)
        self.assertEqual(fused[0]["document"]["id"], "shared")
        self.assertEqual(fused[0]["channel_ranks"], {"title": 1, "content": 1})
        self.assertEqual(fused[0]["matched_terms"], ["gnd", "return"])
        self.assertAlmostEqual(fused[0]["score"], 2.2 / 61.0)

    def test_diversification_is_bounded_deterministic_and_provenance_preserving(self):
        documents = [
            {"id": "first", "text": "GND return around U3"},
            {"id": "duplicate", "text": "GND return around U3 near U4"},
            {"id": "different", "text": "USB shield connector clearance"},
        ]
        ranked = [{"document": row, "score": score, "matched_terms": ["gnd"]}
                  for row, score in zip(documents, (0.030, 0.026, 0.020))]
        first = LEXICAL.diversify_ranked(ranked, limit=3, text_key="text")
        second = LEXICAL.diversify_ranked(ranked, limit=3, text_key="text")
        self.assertEqual([item["document"]["id"] for item in first],
                         [item["document"]["id"] for item in second])
        self.assertEqual(first[0]["document"]["id"], "first")
        self.assertEqual(first[1]["document"]["id"], "different")
        self.assertTrue(all("diversity_score" in item for item in first))
        self.assertLessEqual(len(LEXICAL.diversify_ranked(ranked, limit=1)), 1)


if __name__ == "__main__":
    unittest.main()
