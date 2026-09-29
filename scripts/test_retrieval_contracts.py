"""No-network contracts for canonical retrieval requests, hits, and adapters."""

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from memory_manager import MemoryManager
from memory_store import MemoryStore
from context_broker import ContextBroker
from context_package import build_context_package
from project_index import ProjectIndex
from retrieval_adapters import MemoryManagerRetriever, ProjectIndexRetriever
from retrieval_contracts import (RetrievalChannel, RetrievalRequest,
                                 RetrievalStatus)


def project_snapshot():
    return {"typed_state": {"available": True, "project": {
        "id": "contract-project",
        "board": {"layers": [{"id": "F.Cu", "name": "F.Cu",
                                "kind": "copper", "visible": True}],
                  "footprints": [{"id": "fp-u3", "reference": "U3",
                                  "value": "Regulator", "layer_id": "F.Cu",
                                  "position": {"x_nm": 0, "y_nm": 0}}]},
        "components": [{"id": "sch-u3", "reference": "U3",
                        "value": "Regulator"}],
    }}}


class RetrievalContractTests(unittest.TestCase):
    def test_request_bounds_and_normalizes_typed_filters(self):
        request = RetrievalRequest(
            query="  U3 on F.Cu  ", project_id="project-1", scope="project",
            bbox=(0, 0, 10, 20), entity_types=("footprint",),
            net_ids=("GND", "GND"), channels=(RetrievalChannel.EXACT,
                                               RetrievalChannel.LEXICAL))
        self.assertEqual(request.query, "U3 on F.Cu")
        self.assertEqual(request.channels,
                         (RetrievalChannel.EXACT, RetrievalChannel.LEXICAL))
        self.assertEqual(request.net_ids, ("GND",))
        with self.assertRaises(ValueError):
            RetrievalRequest(query="x", top_k=12, candidate_budget=4)
        with self.assertRaises(ValueError):
            RetrievalRequest(query="x", bbox=(2, 0, 1, 3))
        with self.assertRaises(ValueError):
            RetrievalRequest(query="x", bbox=(0, 0, float("inf"), 3))

    def test_project_adapter_returns_revisioned_canonical_hits_and_stable_payload(self):
        request = RetrievalRequest(query="U3", project_id="contract-project",
                                   requested_revision="revision-1", scope="project",
                                   channels=(RetrievalChannel.EXACT,))
        retriever = ProjectIndexRetriever(ProjectIndex(), project_snapshot(),
                                          project_id="contract-project")
        result = retriever.retrieve(request)
        self.assertEqual(result.status, RetrievalStatus.READY)
        self.assertTrue(result.hits)
        hit = result.hits[0]
        self.assertEqual(result.revision, hit.source_revision)
        self.assertEqual(hit.channel, RetrievalChannel.EXACT)
        self.assertEqual(hit.source_type, "footprint")
        self.assertEqual(hit.content["reference"], "U3")
        payload = retriever.to_context_payload(result)
        self.assertEqual(payload["revision"], result.revision)
        self.assertEqual(hit.canonical_id, "footprint:U3")
        self.assertEqual(payload["entities"][0]["id"], "U3")
        self.assertIn("stats", payload)
        self.assertNotIn("channel_score", str(payload))

    def test_project_adapter_scopes_and_projects_requested_fields(self):
        retriever = ProjectIndexRetriever(ProjectIndex(), project_snapshot(),
                                          project_id="contract-project")
        result = retriever.retrieve(RetrievalRequest(
            query="U3", project_id="contract-project", scope="project",
            entity_types=("footprint",), fields=("reference", "position_mm"),
            channels=(RetrievalChannel.EXACT,)))
        self.assertEqual(result.status, RetrievalStatus.READY)
        self.assertEqual(set(result.hits[0].content), {
            "id", "kind", "reference", "position_mm", "retrieval", "rank"})
        wrong_project = retriever.retrieve(RetrievalRequest(
            query="U3", project_id="other-project", scope="project"))
        self.assertEqual(wrong_project.status, RetrievalStatus.FAILED)
        self.assertEqual(wrong_project.reason, "project_scope_mismatch")

    def test_project_adapter_returns_the_requested_evidence_channel(self):
        retriever = ProjectIndexRetriever(ProjectIndex(), project_snapshot(),
                                          project_id="contract-project")
        result = retriever.retrieve(RetrievalRequest(
            query="U3", project_id="contract-project", scope="project",
            channels=(RetrievalChannel.GRAPH,)))
        self.assertTrue(result.hits)
        self.assertTrue(all(hit.channel == RetrievalChannel.GRAPH
                            for hit in result.hits))
        self.assertTrue(any(hit.relationship_path for hit in result.hits))

    def test_memory_adapter_keeps_scope_and_returns_canonical_results(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread-1", project_id="project-1")
            manager.configure({"ltm": True, "episodic": False})
            saved = manager.add("Preserve GND return near U3", tier="ltm",
                                title="GND rule", scope="conversation")
            self.assertIsNotNone(saved)
            assert saved is not None
            result = MemoryManagerRetriever(manager).retrieve(RetrievalRequest(
                query="GND return U3", project_id="project-1",
                thread_id="thread-1", scope="memory",
                channels=(RetrievalChannel.LEXICAL,)))
            self.assertEqual(result.status, RetrievalStatus.READY)
            self.assertEqual(result.hits[0].channel, RetrievalChannel.LEXICAL)
            self.assertEqual(result.hits[0].canonical_id, saved["id"])
            self.assertEqual(result.hits[0].source_scope, "ltm")
            self.assertEqual(result.hits[0].content["content"],
                             "Preserve GND return near U3")
            self.assertGreater(result.hits[0].channel_rank, 0)
            wrong_thread = MemoryManagerRetriever(manager).retrieve(RetrievalRequest(
                query="GND return U3", project_id="project-1",
                thread_id="other-thread", scope="memory"))
            self.assertEqual(wrong_thread.status, RetrievalStatus.FAILED)
            self.assertEqual(wrong_thread.reason, "thread_scope_mismatch")

    def test_disabled_memory_is_explicit_not_empty_success(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"))
            result = MemoryManagerRetriever(manager).retrieve(
                RetrievalRequest(query="anything", scope="memory"))
            self.assertEqual(result.status, RetrievalStatus.DISABLED)
            self.assertEqual(result.hits, ())

    def test_context_broker_uses_adapter_and_keeps_project_payload_shape(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread-1",
                                    project_id="contract-project")
            manager.configure({"ltm": False, "episodic": False})
            context = ContextBroker().prepare(
                manager, thread_id="thread-1", project_revision="ui-rev-1",
                project_id="contract-project", user_request="Inspect U3",
                project_snapshot=project_snapshot())
            project = context["project_retrieval"]
            self.assertEqual(set(project).intersection(
                {"available", "reason", "revision", "entities", "stats"}),
                {"available", "reason", "revision", "entities", "stats"})
            self.assertTrue(project["available"])
            self.assertTrue(any(entity.get("reference") == "U3"
                                for entity in project["entities"]))
            self.assertEqual(context["project_retrieval_status"]["status"], "ready")
            self.assertEqual(context["memory_retrieval_status"]["status"], "disabled")
            self.assertEqual(context["project_revision"], "ui-rev-1")
            package = build_context_package(
                {}, context["memories"], [], char_limit=4096,
                project_retrieval=project,
                memory_retrieval_status=context["memory_retrieval_status"],
                project_retrieval_status=context["project_retrieval_status"])
            self.assertEqual(package["metadata"]["project_retrieval_status"]["status"],
                             "ready")
            self.assertEqual(package["metadata"]["memory_retrieval_status"]["status"],
                             "disabled")
            self.assertNotIn("project_retrieval_status", package["content"])


if __name__ == "__main__":
    unittest.main()
