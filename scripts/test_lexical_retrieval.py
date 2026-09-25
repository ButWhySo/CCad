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


if __name__ == "__main__":
    unittest.main()
