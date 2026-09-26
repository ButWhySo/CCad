"""Contract for exact, lexical, relationship, spatial, and incremental retrieval."""

import sys
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from project_index import ProjectIndex
from context_package import build_context_package
from context_broker import ContextBroker, extract_context_signals
from memory_manager import MemoryManager
from memory_store import MemoryStore


def project_snapshot():
    return {
        "typed_state": {"available": True, "project": {
            "id": "project-1",
            "board": {
                "layers": [
                    {"id": "F.Cu", "name": "F.Cu", "kind": "copper", "visible": True},
                    {"id": "B.Cu", "name": "B.Cu", "kind": "copper", "visible": True},
                ],
                "footprints": [
                    {"reference": "U3", "value": "TPS62130", "footprint_name": "Package_SO:QFN-16",
                     "layer_id": "F.Cu", "position": {"x_nm": 0, "y_nm": 0}},
                    {"reference": "J2", "value": "USB connector", "footprint_name": "Connector_USB:USB_C_Receptacle",
                     "layer_id": "F.Cu", "position": {"x_nm": 30_000_000, "y_nm": 0}},
                ],
                "pads": [
                    {"id": "P1", "component_id": "U3", "pin_name": "GND", "net_id": "GND",
                     "position": {"x_nm": 0, "y_nm": 0}},
                    {"id": "P2", "component_id": "J2", "pin_name": "GND", "net_id": "GND",
                     "position": {"x_nm": 30_000_000, "y_nm": 0}},
                ],
                "tracks": [
                    {"id": "T1", "net_id": "GND", "layer_id": "F.Cu",
                     "start": {"x_nm": 0, "y_nm": 0},
                     "end": {"x_nm": 2_000_000, "y_nm": 0}},
                ],
                "vias": [
                    {"id": "V1", "net_id": "GND", "position": {"x_nm": 1_000_000, "y_nm": 0},
                     "start_layer_id": "F.Cu", "end_layer_id": "B.Cu"},
                ],
                "zones": [], "track_arcs": [], "graphics": [], "texts": [],
            },
            "components": [
                {"id": "sch-u3", "part": "Regulator:TPS62130", "reference": "U3",
                 "value": "TPS62130", "position": {"x_nm": 1_000_000, "y_nm": 2_000_000}},
                {"id": "sch-j2", "part": "Connector:USB_C", "reference": "J2",
                 "value": "USB connector", "position": {"x_nm": 5_000_000, "y_nm": 2_000_000}},
            ],
            "nets": [{"id": "GND", "members": [
                {"component_id": "sch-u3", "pin_name": "GND"},
                {"component_id": "sch-j2", "pin_name": "GND"},
            ]}],
            "wires": [{"id": "W1", "net_id": "GND",
                       "start": {"x_nm": 1_000_000, "y_nm": 2_000_000},
                       "end": {"x_nm": 5_000_000, "y_nm": 2_000_000}}],
            "labels": [], "constraints": [],
        }},
        "active_pcb_layer_id": "F.Cu",
        "active_pcb_net_id": "GND",
        "selection": {"items": [{"object_id": "U3"}]},
    }


def power_passive_snapshot():
    snapshot = project_snapshot()
    project = snapshot["typed_state"]["project"]
    project["board"]["footprints"].append({
        "id": "board-anchor-r", "reference": "U1", "value": "Regulator",
        "footprint_name": "Package_SO:QFN-16", "layer_id": "F.Cu",
        "position": {"x_nm": 10_000_000, "y_nm": 10_000_000},
    })
    project["components"] = [
        {"id": "sch-reg", "part": "Regulator:Example", "reference": "U1",
         "value": "Regulator", "pins": [
             {"name": "VIN", "number": "1", "electrical_type": "power_in"}]},
        {"id": "sch-cap", "part": "Device:C", "reference": "C1",
         "value": "100nF", "pins": [
             {"name": "1", "number": "1", "electrical_type": "passive"}]},
    ]
    project["nets"] = [{"id": "VIN", "members": [
        {"component_id": "sch-reg", "pin_name": "VIN"},
        {"component_id": "sch-cap", "pin_name": "1"},
    ]}]
    return snapshot


class ProjectIndexTests(unittest.TestCase):
    def test_source_typed_power_and_passive_association_is_retrievable_and_packaged(self):
        snapshot = power_passive_snapshot()
        result = ProjectIndex().retrieve(snapshot, "U1 input power", limit=24)
        capacitor = next(item for item in result["entities"]
                         if item["kind"] == "schematic_symbol" and
                         item.get("reference") == "C1")
        self.assertIn("shares_power_input_net_with_passive",
                      capacitor.get("relationships", []))
        self.assertEqual(result["stats"]["passive_association_match_count"], 1)
        self.assertEqual(
            result["power_passive_association_semantics"],
            "same_sheet_exact_net_source_declared_power_in_and_passive_pins_only; "
            "association_not_decoupling_inference")

        package = build_context_package(
            json.dumps(snapshot), [], [], char_limit=8192,
            project_retrieval=result)
        envelope = json.loads(package["content"].split("\n", 1)[1])
        packaged_capacitor = next(item for item in
                                  envelope["project_retrieval"]["entities"]
                                  if item.get("kind") == "schematic_symbol" and
                                  item.get("reference") == "C1")
        self.assertIn("shares_power_input_net_with_passive",
                      packaged_capacitor["relationships"])
        self.assertIn("not_decoupling_inference",
                      envelope["project_retrieval"][
                          "power_passive_association_semantics"])
        self.assertEqual(package["metadata"]["project_retrieval_stats"][
            "passive_association_match_count"], 1)

        query = "Which passive components share a power input net with U1?"
        goal = "Inspect U1's source-model power-net relationships"
        signals = extract_context_signals(
            query, goal=goal, project_id="project-1", active_editor="schematic")
        with tempfile.TemporaryDirectory() as memory_dir:
            manager = MemoryManager(
                MemoryStore(Path(memory_dir) / "memory.json"),
                thread_id="thread-power-passive", project_id="project-1")
            manager.configure({})
            context = ContextBroker().prepare(
                manager, thread_id="thread-power-passive",
                project_revision="fixture-revision", project_id="project-1",
                user_request=signals["query"], goal=goal,
                active_editor="schematic", signals=signals,
                project_snapshot=snapshot)
        context_capacitor = next(
            item for item in context["project_retrieval"]["entities"]
            if item.get("kind") == "schematic_symbol" and
            item.get("reference") == "C1")
        self.assertIn("shares_power_input_net_with_passive",
                      context_capacitor["relationships"])

    def test_power_passive_relationship_is_retrievable_from_either_component(self):
        snapshot = power_passive_snapshot()
        for query, expected in (("U1", "C1"), ("C1", "U1")):
            result = ProjectIndex().retrieve(snapshot, query, limit=24)
            related = next(item for item in result["entities"]
                           if item["kind"] == "schematic_symbol" and
                           item.get("reference") == expected)
            self.assertIn("shares_power_input_net_with_passive",
                          related.get("relationships", []))
        board_origin = ProjectIndex().retrieve(snapshot, "board-anchor-r", limit=24)
        board_related = next(item for item in board_origin["entities"]
                             if item.get("kind") == "schematic_symbol" and
                             item.get("reference") == "C1")
        self.assertIn("shares_power_input_net_with_passive",
                      board_related["relationships"])

    def test_power_passive_association_requires_same_sheet_and_exact_membership(self):
        snapshot = power_passive_snapshot()
        project = snapshot["typed_state"]["project"]
        project["nets"][0]["members"].clear()
        index = ProjectIndex()
        unconnected = index.retrieve(snapshot, "U1", limit=24)
        self.assertFalse(any("shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in unconnected["entities"]))

        snapshot = power_passive_snapshot()
        project = snapshot["typed_state"]["project"]
        project["schematics"] = [{
            "id": "sheet-2", "name": "Other sheet",
            "components": [{"id": "sch-cap-2", "reference": "C2", "pins": [
                {"name": "1", "number": "1", "electrical_type": "passive"}]}],
            "nets": [{"id": "VIN", "members": [
                {"component_id": "sch-cap-2", "pin_name": "1"}]}],
        }]
        cross_sheet = ProjectIndex().retrieve(snapshot, "U1", limit=24)
        self.assertFalse(any(item.get("reference") == "C2" and
                             "shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in cross_sheet["entities"]))

    def test_power_passive_association_rejects_wrong_pin_types_and_net_names(self):
        snapshot = power_passive_snapshot()
        project = snapshot["typed_state"]["project"]
        project["components"][0]["pins"][0]["electrical_type"] = "power_out"
        result = ProjectIndex().retrieve(snapshot, "U1", limit=24)
        self.assertFalse(any("shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in result["entities"]))

        snapshot = power_passive_snapshot()
        project = snapshot["typed_state"]["project"]
        project["nets"] = [{"id": "VIN", "members": [
            {"component_id": "sch-reg", "pin_name": "VIN"}]}]
        project["components"][1]["pins"][0]["name"] = "VIN"
        same_label = ProjectIndex().retrieve(snapshot, "U1", limit=24)
        self.assertFalse(any("shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in same_label["entities"]))

    def test_power_passive_association_suppresses_ambiguous_membership_and_reference(self):
        snapshot = power_passive_snapshot()
        project = snapshot["typed_state"]["project"]
        project["nets"].append({"id": "VIN2", "members": [
            {"component_id": "sch-reg", "pin_name": "VIN"}]})
        ambiguous_pin = ProjectIndex().retrieve(snapshot, "U1", limit=24)
        self.assertFalse(any("shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in ambiguous_pin["entities"]))

        snapshot = power_passive_snapshot()
        project = snapshot["typed_state"]["project"]
        duplicate = dict(project["components"][0])
        duplicate["id"] = "sch-reg-duplicate"
        project["components"].append(duplicate)
        ambiguous_reference = ProjectIndex().retrieve(snapshot, "U1", limit=24)
        self.assertFalse(any("shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in ambiguous_reference["entities"]))

        snapshot = power_passive_snapshot()
        duplicate_footprint = dict(snapshot["typed_state"]["project"]["board"][
            "footprints"][-1])
        duplicate_footprint["id"] = "board-anchor-r-duplicate"
        snapshot["typed_state"]["project"]["board"]["footprints"].append(
            duplicate_footprint)
        ambiguous_board_reference = ProjectIndex().retrieve(
            snapshot, "board-anchor-r", limit=24)
        self.assertFalse(any(item.get("kind") == "schematic_symbol" and
                             item.get("reference") == "C1" and
                             "shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in ambiguous_board_reference["entities"]))

    def test_power_passive_relationship_updates_when_native_net_membership_changes(self):
        snapshot = power_passive_snapshot()
        index = ProjectIndex()
        initial = index.retrieve(snapshot, "U1", limit=24)
        self.assertTrue(any("shares_power_input_net_with_passive" in
                            item.get("relationships", [])
                            for item in initial["entities"]))
        snapshot["typed_state"]["project"]["nets"][0]["members"].pop()
        updated = index.retrieve(snapshot, "U1", limit=24)
        self.assertEqual(updated["stats"]["index_state"], "incremental")
        self.assertFalse(any("shares_power_input_net_with_passive" in
                             item.get("relationships", [])
                             for item in updated["entities"]))

    def test_semantic_project_retrieval_is_opt_in_and_fuses_nonlexical_matches(self):
        class Backend:
            identity = "contract:project-embeddings-v1"

            def __init__(self):
                self.document_batches = 0

            @staticmethod
            def _vector(text):
                text = text.casefold()
                if "receptacle" in text or "cable" in text:
                    return [1.0, 0.0, 0.0]
                if "usb connector" in text or "usb_c" in text:
                    return [1.0, 0.0, 0.0]
                if "power regulator" in text or "tps62130" in text:
                    return [0.0, 1.0, 0.0]
                return [0.0, 0.0, 1.0]

            def embed_query(self, text):
                return self._vector(text)

            def embed_documents(self, texts):
                self.document_batches += 1
                return [self._vector(text) for text in texts]

        snapshot = project_snapshot()
        backend = Backend()
        index = ProjectIndex()
        lexical_only = index.retrieve(snapshot, "receptacle for a cable", limit=12)
        self.assertEqual(lexical_only["semantic_status"], "disabled")
        self.assertFalse(any(item.get("semantic_similarity") is not None
                             for item in lexical_only["entities"]))

        semantic = index.retrieve(snapshot, "receptacle for a cable", limit=12,
                                  embedding_backend=backend)
        connectors = [item for item in semantic["entities"]
                      if item["kind"] == "footprint" and item["id"] == "J2"]
        self.assertEqual(len(connectors), 1)
        self.assertGreater(connectors[0]["semantic_similarity"], 0.99)
        self.assertIn(connectors[0]["retrieval"], {"semantic", "hybrid"})
        self.assertEqual(semantic["semantic_status"], "ready")
        self.assertGreaterEqual(semantic["stats"]["semantic_match_count"], 1)
        # Tracks and vias remain governed by exact/graph/spatial retrieval; they
        # are not sent to the semantic backend as project-language documents.
        self.assertEqual(backend.document_batches, 1)

    def test_semantic_project_cache_reuses_unchanged_text_and_refreshes_edits(self):
        class Backend:
            identity = "contract:project-embeddings-v1"

            def __init__(self):
                self.calls = []

            def embed_query(self, text):
                return [1.0, 0.0]

            def embed_documents(self, texts):
                self.calls.extend(texts)
                return [[1.0, 0.0] for _ in texts]

        snapshot = project_snapshot()
        backend = Backend()
        index = ProjectIndex()
        index.retrieve(snapshot, "adapter", embedding_backend=backend)
        first_count = len(backend.calls)
        self.assertGreater(first_count, 0)
        index.retrieve(snapshot, "adapter", embedding_backend=backend)
        self.assertEqual(len(backend.calls), first_count)

        snapshot["typed_state"]["project"]["board"]["footprints"][1]["value"] = "USB-C host connector"
        index.retrieve(snapshot, "adapter", embedding_backend=backend)
        self.assertGreater(len(backend.calls), first_count)
        self.assertIn("USB-C host connector", " ".join(backend.calls))

    def test_semantic_candidate_cap_prioritizes_lexical_hits(self):
        class Backend:
            identity = "contract:bounded-project-embeddings"

            def __init__(self):
                self.documents = []

            def embed_query(self, text):
                return [1.0, 0.0]

            def embed_documents(self, texts):
                self.documents.extend(texts)
                return [[1.0, 0.0] for _ in texts]

        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["footprints"] = [
            {"reference": f"A{index:03}", "value": "Unrelated test device",
             "footprint_name": "Package:Generic", "layer_id": "F.Cu",
             "position": {"x_nm": index, "y_nm": 0}}
            for index in range(80)
        ] + board["footprints"]
        backend = Backend()
        result = ProjectIndex().retrieve(snapshot, "USB connector", limit=12,
                                         embedding_backend=backend)
        self.assertLessEqual(len(backend.documents), 64)
        self.assertTrue(any("Connector_USB:USB_C_Receptacle" in text
                            for text in backend.documents))
        self.assertTrue(any(item["kind"] == "footprint" and item["id"] == "J2"
                            for item in result["entities"]))

    def test_semantic_backend_failure_preserves_exact_and_lexical_results(self):
        class Backend:
            identity = "contract:failure"

            def embed_query(self, text):
                raise RuntimeError("must not leak prompt or exception")

            def embed_documents(self, texts):
                raise RuntimeError("unreachable")

        snapshot = project_snapshot()
        result = ProjectIndex().retrieve(snapshot, "TPS62130 power regulator",
                                         embedding_backend=Backend())
        self.assertEqual(result["semantic_status"], "embedding_failed")
        self.assertTrue(any(item["id"] == "U3" for item in result["entities"]))
        self.assertTrue(any(item["retrieval"] in {"exact", "lexical"}
                            for item in result["entities"]))

    def test_context_broker_uses_configured_semantic_backend_and_unloads_it_when_disabled(self):
        class Backend:
            identity = "contract:configured-project-embeddings"

            def __init__(self):
                self.document_calls = 0

            @staticmethod
            def _vector(text):
                text = text.casefold()
                return ([1.0, 0.0] if "receptacle" in text or "usb connector" in text
                        else [0.0, 1.0])

            def embed_query(self, text):
                return self._vector(text)

            def embed_documents(self, texts):
                self.document_calls += len(texts)
                return [self._vector(text) for text in texts]

        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    project_id="project-1")
            backend = Backend()
            manager.set_embedding_backend(backend)
            manager.configure({"semantic": {"enabled": True,
                                               "backend": "ollama_local",
                                               "model": "contract-model"}})
            broker = ContextBroker()
            prepared = broker.prepare(
                manager, thread_id="thread-1", project_revision="rev-1",
                project_id="project-1", user_request="receptacle for a cable",
                project_snapshot=project_snapshot())
            result = prepared["project_retrieval"]
            self.assertEqual(result["semantic_status"], "ready")
            self.assertTrue(any(item["kind"] == "footprint" and item["id"] == "J2"
                                and item.get("semantic_similarity", 0) > 0.99
                                for item in result["entities"]))
            packaged = build_context_package(
                json.dumps(project_snapshot()), [], [], char_limit=8192,
                project_retrieval=result)
            envelope = json.loads(packaged["content"].split("\n", 1)[1])
            packaged_j2 = next(item for item in envelope["project_retrieval"]["entities"]
                               if item["kind"] == "footprint" and item["id"] == "J2")
            self.assertGreater(packaged_j2["semantic_similarity"], 0.99)
            self.assertEqual(packaged_j2["footprint_name"],
                             "Connector_USB:USB_C_Receptacle")
            self.assertEqual(packaged["metadata"]["project_retrieval_semantic_status"],
                             "ready")
            self.assertGreater(packaged["metadata"]["project_retrieval_semantic_count"], 0)

            manager.configure({"semantic": {"enabled": False}})
            disabled = broker.prepare(
                manager, thread_id="thread-1", project_revision="rev-1",
                project_id="project-1", user_request="receptacle for a cable",
                project_snapshot=project_snapshot(), force_refresh=True)
            self.assertEqual(disabled["project_retrieval"]["semantic_status"], "disabled")
            self.assertEqual(disabled["project_retrieval"]["stats"]["semantic_match_count"], 0)
            prior_document_calls = backend.document_calls
            manager.configure({"semantic": {"enabled": True,
                                               "backend": "ollama_local",
                                               "model": "contract-model"}})
            broker.prepare(
                manager, thread_id="thread-1", project_revision="rev-1",
                project_id="project-1", user_request="receptacle for a cable",
                project_snapshot=project_snapshot(), force_refresh=True)
            self.assertGreater(backend.document_calls, prior_document_calls)

    def test_schematic_declared_pins_and_serialized_annotations_are_retrievable(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        symbol = project["components"][0]
        symbol["unit"] = 1
        symbol["pins"] = [
            {"name": "VIN", "number": "1", "electrical_type": "power_in",
             "graphical_style": "line", "orientation": "right"},
            {"id": "LIBPIN_PGOOD", "name": "PGOOD", "number": "2", "electrical_type": "output",
             "graphical_style": "line", "orientation": "left"},
        ]
        project["nets"][0]["members"].append(
            {"component_id": "sch-u3", "pin_name": "VIN"})
        project.update({
            "textboxes": [{"id": "TBX1", "text": "Input power requirements",
                           "area": {"origin": {"x_nm": 0, "y_nm": 0},
                                    "size": {"width_nm": 5_000_000,
                                             "height_nm": 2_000_000}}}],
            "graphics": [{"id": "SG1", "kind": "line",
                          "start": {"x_nm": 0, "y_nm": 0},
                          "end": {"x_nm": 1_000_000, "y_nm": 0}}],
            "rule_areas": [{"id": "RA1", "name": "High voltage keepout",
                            "outline": [{"x_nm": 0, "y_nm": 0},
                                        {"x_nm": 2_000_000, "y_nm": 0},
                                        {"x_nm": 2_000_000, "y_nm": 1_000_000}],
                            "locked": True}],
            "tables": [{"id": "ST1", "rows": 1, "cols": 1,
                        "cells": [{"row": 0, "col": 0, "text": "ACME-42"}]}],
            "junctions": [{"id": "JUNC1", "position": {"x_nm": 1_000_000,
                                                          "y_nm": 2_000_000}}],
            "no_connects": [{"id": "NC1", "position": {"x_nm": 2_000_000,
                                                            "y_nm": 3_000_000}}],
            "markers": [{"id": "MK1", "kind": "erc", "severity": "warning",
                         "position": {"x_nm": 3_000_000, "y_nm": 4_000_000}}],
            "bus_entries": [{"id": "BE1", "kind": "wire",
                             "position": {"x_nm": 4_000_000, "y_nm": 5_000_000}}],
        })
        index = ProjectIndex()
        pin_result = index.retrieve(snapshot, "PGOOD pin 2", limit=24)
        pins = [item for item in pin_result["entities"]
                if item["kind"] == "schematic_pin" and
                item.get("symbol_id") == "sch-u3"]
        unconnected = next(item for item in pins if item.get("pin_name") == "PGOOD")
        connected = next(item for item in pins if item.get("pin_name") == "VIN")
        self.assertEqual(unconnected["pin_number"], "2")
        self.assertEqual(unconnected["identity_source"], "native_pin_id")
        self.assertEqual(unconnected["id"], "declared:sch-u3:1:LIBPIN_PGOOD")
        self.assertEqual(unconnected["electrical_type"], "output")
        self.assertEqual(unconnected["membership_kind"], "declared_pin")
        self.assertNotIn("net_id", unconnected)
        self.assertEqual(connected["net_id"], "GND")
        self.assertEqual(connected["membership_kind"], "schematic_net_member")
        self.assertEqual(connected["identity_source"],
                         "derived_from_symbol_unit_and_pin_number_or_name")
        symbol = next(item for item in pin_result["entities"]
                      if item["kind"] == "schematic_symbol" and
                      item["id"] == "sch-u3")
        self.assertIn("declared_pin", symbol.get("relationships", []))

        expected = {
            ("schematic_textbox", "TBX1"), ("schematic_graphic", "SG1"),
            ("schematic_rule_area", "RA1"), ("schematic_table", "ST1"),
            ("schematic_junction", "JUNC1"), ("schematic_no_connect", "NC1"),
            ("schematic_marker", "MK1"), ("schematic_bus_entry", "BE1"),
        }
        # Query each typed identity independently. Generic junction and marker
        # records have no meaningful prose, so a prose-only query must not be
        # expected to retrieve them by coincidence.
        by_key = {}
        for kind, object_id in expected:
            result = index.retrieve(snapshot, object_id, limit=40)
            by_key.update({(item["kind"], item["id"]): item
                           for item in result["entities"]})
        actual = set(by_key)
        self.assertTrue(expected.issubset(actual), expected - actual)
        self.assertEqual(by_key[("schematic_textbox", "TBX1")]["text"],
                         "Input power requirements")
        self.assertTrue(by_key[("schematic_rule_area", "RA1")]["locked"])
        self.assertEqual(by_key[("schematic_table", "ST1")]["text"], "ACME-42")

    def test_declared_pin_and_schematic_entity_removals_are_incremental(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["components"][0]["pins"] = [
            {"name": "PGOOD", "number": "2", "electrical_type": "output"}]
        project["textboxes"] = [{"id": "TBX1", "text": "Output enable"}]
        index = ProjectIndex()
        before = index.retrieve(snapshot, "PGOOD Output enable", limit=20)
        self.assertTrue(any(item["id"] == "TBX1" for item in before["entities"]))
        project["components"][0]["pins"].clear()
        project["textboxes"].clear()
        after = index.retrieve(snapshot, "PGOOD Output enable", limit=20)
        self.assertEqual(after["stats"]["index_state"], "incremental")
        self.assertFalse(any(item["kind"] == "schematic_pin" and
                             item.get("pin_name") == "PGOOD" for item in after["entities"]))
        self.assertFalse(any(item["kind"] == "schematic_textbox" and
                             item["id"] == "TBX1" for item in after["entities"]))

    def test_exact_identity_and_layer_lookup_use_typed_board_and_schematic(self):
        index = ProjectIndex()
        snapshot = project_snapshot()
        result = index.retrieve(snapshot, "select U3 on F.Cu", limit=12)
        keys = {(item["kind"], item["id"]) for item in result["entities"]}
        self.assertIn(("footprint", "U3"), keys)
        self.assertIn(("schematic_symbol", "sch-u3"), keys)
        self.assertIn(("layer", "F.Cu"), keys)
        self.assertEqual(result["revision"], index.revision("project-1"))
        self.assertEqual(result["stats"]["index_state"], "built")

    def test_via_layer_span_is_retrieved_from_either_endpoint(self):
        index = ProjectIndex()
        result = index.retrieve(project_snapshot(), "B.Cu", limit=20)
        via = next(item for item in result["entities"]
                   if item["kind"] == "via" and item["id"] == "V1")
        self.assertEqual(via["start_layer_id"], "F.Cu")
        self.assertEqual(via["end_layer_id"], "B.Cu")
        self.assertEqual(via["layer_ids"], ["F.Cu", "B.Cu"])
        self.assertEqual(via["relationship"], "same_layer")
        top_copper = index.retrieve(project_snapshot(), "F.Cu", limit=20)
        self.assertTrue(any(item["kind"] == "via" and item["id"] == "V1" and
                            item["layer_ids"] == ["F.Cu", "B.Cu"]
                            for item in top_copper["entities"]))

    def test_pad_layer_set_is_indexed_and_via_endpoint_edits_are_incremental(self):
        index = ProjectIndex()
        before = project_snapshot()
        board = before["typed_state"]["project"]["board"]
        board["layers"].append({"id": "In1.Cu", "name": "In1.Cu"})
        board["pads"][0]["layers"] = ["F.Cu", "B.Cu"]
        found = index.retrieve(before, "B.Cu", limit=20)
        self.assertTrue(any(item["kind"] == "pad" and item["id"] == "P1"
                            and item["layer_ids"] == ["F.Cu", "B.Cu"]
                            for item in found["entities"]))
        board["vias"][0]["end_layer_id"] = "In1.Cu"
        changed = index.retrieve(before, "B.Cu", limit=20)
        self.assertEqual(changed["stats"]["index_state"], "incremental")
        self.assertEqual(changed["stats"]["updated_count"], 1)
        self.assertFalse(any(item["kind"] == "via" and item["id"] == "V1"
                             for item in changed["entities"]))
        in1 = index.retrieve(before, "In1.Cu", limit=20)
        via = next(item for item in in1["entities"]
                   if item["kind"] == "via" and item["id"] == "V1")
        self.assertEqual(via["layer_ids"], ["F.Cu", "In1.Cu"])
        self.assertEqual(in1["stats"]["index_state"], "cached")

    def test_real_serialized_padstack_layer_set_is_retrievable_and_incremental(self):
        index = ProjectIndex()
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["pads"][0]["padstack"] = {"layer_set": ["F.Cu", "B.Cu"]}
        found = index.retrieve(snapshot, "B.Cu", limit=20)
        pad = next(item for item in found["entities"]
                   if item["kind"] == "pad" and item["id"] == "P1")
        self.assertEqual(pad["layer_ids"], ["F.Cu", "B.Cu"])
        self.assertEqual(pad["relationship"], "same_layer")

        snapshot["typed_state"]["project"]["board"]["pads"][0]["padstack"][
            "layer_set"] = ["F.Cu"]
        changed = index.retrieve(snapshot, "B.Cu", limit=20)
        self.assertEqual(changed["stats"]["updated_count"], 1)
        self.assertFalse(any(item["kind"] == "pad" and item["id"] == "P1"
                             for item in changed["entities"]))

    def test_all_serialized_board_layer_variants_retrieve_from_exact_layer(self):
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["graphics"] = [{"id": "G1", "layer_id": "B.Cu",
                              "start": {"x_nm": 1_000_000, "y_nm": 2_000_000},
                              "end": {"x_nm": 3_000_000, "y_nm": 2_000_000}}]
        board["track_arcs"] = [{"id": "A1", "layer_id": "B.Cu", "net_id": "GND",
                                "start": {"x_nm": 4_000_000, "y_nm": 0},
                                "mid": {"x_nm": 5_000_000, "y_nm": 1_000_000},
                                "end": {"x_nm": 6_000_000, "y_nm": 0}}]
        board["texts"] = [{"id": "TX1", "layer_id": "B.Cu", "text": "GND_LABEL",
                           "position": {"x_nm": 7_000_000, "y_nm": 0}}]
        board["dimensions"] = [{"id": "D1", "layer_id": "B.Cu",
                                "start": {"x_nm": 8_000_000, "y_nm": 0},
                                "end": {"x_nm": 9_000_000, "y_nm": 0},
                                "text_position": {"x_nm": 8_500_000, "y_nm": 500_000}}]
        board["barcodes"] = [{"id": "BC1", "layer_id": "B.Cu", "text": "LOT42",
                              "position": {"x_nm": 10_000_000, "y_nm": 0}}]
        board["reference_images"] = [{"id": "IMG1", "layer": "B.Cu", "data": "binary",
                                      "x_mm": 12, "y_mm": 2}]
        board["tables"] = [{"id": "TB1", "layer": "B.Cu", "x_mm": 14, "y_mm": 3,
                            "width_mm": 2, "height_mm": 1}]
        board["teardrops"] = [{"id": "TD1", "layer_id": "B.Cu", "net_id": "GND",
                               "anchor_track_id": "T1",
                               "outline": [{"x_nm": 15_000_000, "y_nm": 0},
                                           {"x_nm": 16_000_000, "y_nm": 1_000_000}]}]
        board["route_requests"] = [{"id": "RR1", "net_id": "GND",
                                    "preferred_layer_id": "B.Cu",
                                    "from_object_id": "P1", "to_object_id": "P2"}]
        board["targets"] = [{"id": "TG1", "layer_id": "B.Cu",
                             "position_x_nm": 17_000_000,
                             "position_y_nm": 1_000_000}]

        result = ProjectIndex().retrieve(snapshot, "B.Cu", limit=30)
        found = {(item["kind"], item["id"]): item for item in result["entities"]}
        for key in (("graphic", "G1"), ("track_arc", "A1"), ("board_text", "TX1"),
                    ("dimension", "D1"), ("barcode", "BC1"),
                    ("reference_image", "IMG1"), ("board_table", "TB1"),
                    ("teardrop", "TD1"), ("target", "TG1")):
            self.assertIn(key, found)
            self.assertEqual(found[key]["layer_id"], "B.Cu")
        self.assertNotIn("data", found[("reference_image", "IMG1")])
        self.assertEqual(found[("reference_image", "IMG1")]["position_mm"],
                         {"x": 12.0, "y": 2.0})
        self.assertEqual(found[("route_request", "RR1")]["preferred_layer_id"], "B.Cu")
        self.assertEqual(found[("route_request", "RR1")]["layer_ids"], ["B.Cu"])
        self.assertEqual(found[("target", "TG1")]["position_mm"], {"x": 17.0, "y": 1.0})

    def test_every_serialized_board_collection_layer_field_is_indexed(self):
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board.update({
            "footprints": [{"reference": "U4", "layer_id": "B.Cu"}],
            "pads": [{"id": "P_B", "padstack": {"layer_set": ["F.Cu", "B.Cu"]}}],
            "tracks": [{"id": "T_B", "layer_id": "B.Cu"}],
            "track_arcs": [{"id": "A_B", "layer_id": "B.Cu"}],
            "vias": [{"id": "V_B", "start_layer_id": "F.Cu", "end_layer_id": "B.Cu"}],
            "zones": [{"id": "Z_B", "layer_ids": ["F.Cu", "B.Cu"]}],
            "graphics": [{"id": "G_B", "layer_id": "B.Cu"}],
            "texts": [{"id": "TX_B", "layer_id": "B.Cu"}],
            "dimensions": [{"id": "D_B", "layer_id": "B.Cu"}],
            "keepouts": [{"id": "KO_B", "layer_ids": ["B.Cu"]}],
            "route_requests": [{"id": "RR_B", "preferred_layer_id": "B.Cu"}],
            "groups": [{"id": "BG_B", "layer_id": "B.Cu"}],
            "targets": [{"id": "TG_B", "layer_id": "B.Cu"}],
            "barcodes": [{"id": "BC_B", "layer_id": "B.Cu"}],
            "tables": [{"id": "TB_B", "layer": "B.Cu"}],
            "placement_regions": [{"id": "PR_B", "layer_ids": ["B.Cu"]}],
            "reference_images": [{"id": "IMG_B", "layer": "B.Cu"}],
            "teardrops": [{"id": "TD_B", "layer_id": "B.Cu"}],
        })
        result = ProjectIndex(max_entities=32).retrieve(snapshot, "B.Cu", limit=32)
        found = {(item["kind"], item["id"]): item for item in result["entities"]}
        expected = {("footprint", "U4"), ("pad", "P_B"), ("track", "T_B"),
                    ("track_arc", "A_B"), ("via", "V_B"), ("zone", "Z_B"),
                    ("graphic", "G_B"), ("board_text", "TX_B"),
                    ("dimension", "D_B"), ("keepout", "KO_B"),
                    ("route_request", "RR_B"), ("board_group", "BG_B"),
                    ("target", "TG_B"), ("barcode", "BC_B"),
                    ("board_table", "TB_B"), ("placement_region", "PR_B"),
                    ("reference_image", "IMG_B"), ("teardrop", "TD_B")}
        self.assertTrue(expected.issubset(found.keys()), expected - found.keys())
        for key in expected:
            self.assertIn("B.Cu", found[key]["layer_ids"], key)

    def test_diagnostic_code_and_message_are_searchable_without_provider(self):
        snapshot = project_snapshot()
        snapshot["project_diagnostics"] = [{
            "engine": "drc", "severity": "error", "code": "TRACK_CLEARANCE",
            "message": "Copper clearance below minimum", "object_id": "T1"}]
        result = ProjectIndex().retrieve(snapshot, "minimum copper clearance", limit=12)
        diagnostic = next(item for item in result["entities"]
                          if item["kind"] == "project_diagnostic")
        self.assertEqual(diagnostic["code"], "TRACK_CLEARANCE")
        self.assertEqual(diagnostic["object_id"], "T1")
        by_code = ProjectIndex().retrieve(snapshot, "TRACK_CLEARANCE", limit=12)
        self.assertTrue(any(item["kind"] == "project_diagnostic" and
                            item["code"] == "TRACK_CLEARANCE"
                            for item in by_code["entities"]))

    def test_regions_and_rectangular_object_bounds_are_spatially_searchable(self):
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["keepouts"] = [{"id": "KO1", "kind": "copper",
                              "area": {"origin": {"x_nm": 20_000_000, "y_nm": 20_000_000},
                                       "size": {"width_nm": 4_000_000,
                                                "height_nm": 2_000_000}}}]
        board["placement_regions"] = [{"id": "PR1", "kind": "placement",
                                       "area": {"x_nm": 30_000_000, "y_nm": 20_000_000,
                                                "width_nm": 5_000_000,
                                                "height_nm": 3_000_000}}]
        board["footprints"][1]["front_courtyard"] = [[
            {"x_nm": 40_000_000, "y_nm": 30_000_000},
            {"x_nm": 42_000_000, "y_nm": 32_000_000}]]
        index = ProjectIndex()
        result = index.retrieve(snapshot, "objects within 1 mm of 22,21", limit=20)
        by_key = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertIn(("keepout", "KO1"), by_key)
        self.assertEqual(by_key[("keepout", "KO1")]["bounds_mm"],
                         {"min_x_mm": 20.0, "min_y_mm": 20.0,
                          "max_x_mm": 24.0, "max_y_mm": 22.0})
        self.assertEqual(by_key[("keepout", "KO1")]["retrieval"], "spatial")
        self.assertNotIn(("placement_region", "PR1"), by_key)
        farther = index.retrieve(snapshot, "objects within 2 mm of 30,21", limit=20)
        self.assertTrue(any(item["kind"] == "placement_region" and item["id"] == "PR1"
                            and item["retrieval"] == "spatial"
                            for item in farther["entities"]))
        courtyard = index.retrieve(snapshot, "objects within 1 mm of 41,31", limit=20)
        j2 = next(item for item in courtyard["entities"]
                  if item["kind"] == "footprint" and item["id"] == "J2")
        self.assertEqual(j2["bounds_mm"]["max_x_mm"], 42.0)
        self.assertEqual(j2["bounds_mm"]["max_y_mm"], 32.0)

    def test_bounding_box_query_returns_intersections_and_refreshes_moved_objects(self):
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["zones"] = [{"id": "Z_IN", "layer_id": "F.Cu",
                           "outline": [{"x_nm": 44_000_000, "y_nm": 44_000_000},
                                       {"x_nm": 48_000_000, "y_nm": 48_000_000}]}]
        board["texts"] = [{"id": "TX_OUT", "text": "outside",
                           "position": {"x_nm": 150_000_000, "y_nm": 150_000_000}}]
        index = ProjectIndex()
        result = index.retrieve(snapshot,
                                "objects in bounding box from 0,0 to 50,50 mm", limit=20)
        found = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertIn(("zone", "Z_IN"), found)
        self.assertNotIn(("board_text", "TX_OUT"), found)
        self.assertEqual(found[("zone", "Z_IN")]["retrieval"], "spatial")
        self.assertEqual(found[("zone", "Z_IN")]["distance_mm"], 0.0)

        board["zones"][0]["outline"][0]["x_nm"] = 60_000_000
        board["zones"][0]["outline"][1]["x_nm"] = 65_000_000
        moved = index.retrieve(snapshot,
                               "objects in bounding box from 0,0 to 50,50 mm", limit=20)
        self.assertEqual(moved["stats"]["index_state"], "incremental")
        self.assertFalse(any(item["kind"] == "zone" and item["id"] == "Z_IN"
                             and item["retrieval"] == "spatial"
                             for item in moved["entities"]))

    def test_spatial_query_includes_only_diagnostics_linked_to_region_objects(self):
        snapshot = project_snapshot()
        snapshot["project_diagnostics"] = [
            {"engine": "drc", "severity": "error", "code": "TRACK_CLEARANCE",
             "message": "Clearance violation", "object_id": "T1"},
            {"engine": "erc", "severity": "warning", "code": "PIN_NOT_CONNECTED",
             "message": "Unconnected pin", "object_id": "sch-u3"},
            {"engine": "drc", "severity": "error", "code": "FAR_AWAY",
             "message": "Unrelated violation", "object_id": "J2"},
        ]
        index = ProjectIndex()
        result = index.retrieve(
            snapshot, "PCB objects in rectangle from -1,-1 to 3,2 mm", limit=32)
        diagnostics = [item for item in result["entities"]
                       if item["kind"] == "project_diagnostic"]
        self.assertTrue(any(item["code"] == "TRACK_CLEARANCE" and
                            item["object_id"] == "T1" for item in diagnostics))
        self.assertFalse(any(item["code"] == "PIN_NOT_CONNECTED"
                             for item in diagnostics))
        self.assertFalse(any(item["code"] == "FAR_AWAY" for item in diagnostics))
        self.assertTrue(all(item["retrieval"] == "relationship" and
                            item["relationship"] == "diagnostic_for"
                            for item in diagnostics))

        schematic_result = index.retrieve(
            snapshot, "schematic symbols in rectangle from -1,-1 to 3,2 mm", limit=32)
        schematic_diagnostics = [item for item in schematic_result["entities"]
                                 if item["kind"] == "project_diagnostic"]
        self.assertTrue(any(item["code"] == "PIN_NOT_CONNECTED" and
                            item["object_id"] == "sch-u3"
                            for item in schematic_diagnostics))
        self.assertFalse(any(item["code"] == "TRACK_CLEARANCE"
                             for item in schematic_diagnostics))

        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread", project_id="project-1")
            manager.configure({})
            assembled = ContextBroker().prepare(
                manager, thread_id="thread", project_revision="spatial-region",
                project_snapshot=snapshot,
                user_request="Find DRC markers in bounding box from 10,10 to 14,14 mm")
            packaged_diagnostics = [item for item in
                                    assembled["project_retrieval"]["entities"]
                                    if item["kind"] == "project_diagnostic"]
            self.assertTrue(any(item["code"] == "TRACK_CLEARANCE" and
                                item["object_id"] == "T1"
                                for item in packaged_diagnostics))

        snapshot["project_diagnostics"].pop(0)
        refreshed = ProjectIndex()
        refreshed.retrieve(snapshot, "T1")
        snapshot["project_diagnostics"].clear()
        stale = refreshed.retrieve(
            snapshot, "objects in rectangle from -1,-1 to 3,2 mm", limit=32)
        self.assertFalse(any(item["kind"] == "project_diagnostic"
                             for item in stale["entities"]))

    def test_board_design_rule_values_are_searchable_incremental_and_survive_compaction(self):
        import json
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["design_rules"] = {
            "copper_clearance_nm": 200_000,
            "min_track_width_nm": 150_000,
            "min_via_diameter_nm": 500_000,
            "tent_vias_front": True,
        }
        index = ProjectIndex()
        result = index.retrieve(snapshot, "minimum track width", limit=12)
        rules = next(item for item in result["entities"] if item["kind"] == "design_rules")
        self.assertEqual(rules["design_rules"]["min_track_width_nm"], 150_000)
        self.assertEqual(rules["design_rules"]["copper_clearance_nm"], 200_000)
        self.assertEqual(rules["retrieval"], "exact")

        board["design_rules"]["min_track_width_nm"] = 175_000
        changed = index.retrieve(snapshot, "minimum track width", limit=12)
        self.assertEqual(changed["stats"]["index_state"], "incremental")
        self.assertEqual(changed["stats"]["updated_count"], 1)
        updated = next(item for item in changed["entities"]
                       if item["kind"] == "design_rules")
        package = build_context_package(json.dumps(snapshot), [], [], char_limit=4096,
                                        project_retrieval=changed)
        envelope = json.loads(package["content"].split("\n", 1)[1])
        packaged = next(item for item in envelope["project_retrieval"]["entities"]
                        if item["kind"] == "design_rules")
        self.assertEqual(updated["design_rules"]["min_track_width_nm"], 175_000)
        self.assertEqual(packaged["design_rules"]["min_track_width_nm"], 175_000)

    def test_schematic_sheet_name_and_project_relative_path_are_exact_and_searchable(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["sheets"] = [{"id": "sheet-power", "name": "Power Stage",
                              "file_path": r"sheets\power_stage.kicad_sch"}]
        index = ProjectIndex()

        by_title = index.retrieve(snapshot, "Power Stage", limit=12)
        sheet = next(item for item in by_title["entities"]
                     if item["kind"] == "schematic_sheet")
        self.assertEqual(sheet["name"], "Power Stage")
        self.assertEqual(sheet["sheet_path"], "sheets/power_stage.kicad_sch")

        by_path = index.retrieve(snapshot, "sheets/power_stage.kicad_sch", limit=12)
        self.assertTrue(any(item["kind"] == "schematic_sheet" and
                            item["id"] == "sheet-power" and
                            item["retrieval"] == "exact"
                            for item in by_path["entities"]))
        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=by_path)
        envelope = __import__("json").loads(package["content"].split("\n", 1)[1])
        packaged_sheet = next(item for item in
                              envelope["project_retrieval"]["entities"]
                              if item["kind"] == "schematic_sheet")
        self.assertEqual(packaged_sheet["sheet_path"], "sheets/power_stage.kicad_sch")

    def test_symbol_fields_are_searchable_and_returned_without_secret_properties(self):
        snapshot = project_snapshot()
        symbol = snapshot["typed_state"]["project"]["components"][0]
        symbol["fields"] = [
            {"name": "Manufacturer", "text": "Acme Circuits", "visible": True},
            {"name": "Order code", "text": "ACME-42", "visible": False},
            {"name": "Zero value", "text": "0", "visible": True},
        ]
        symbol["properties"] = {"Footprint family": "QFN", "access_token": "do-not-index"}
        index = ProjectIndex()

        result = index.retrieve(snapshot, "Acme Circuits ACME-42", limit=12)
        component = next(item for item in result["entities"]
                         if item["kind"] == "schematic_symbol" and item["id"] == "sch-u3")
        properties = {item["name"]: item for item in component["properties"]}
        self.assertEqual(properties["Manufacturer"],
                         {"name": "Manufacturer", "value": "Acme Circuits", "visible": True})
        self.assertEqual(properties["Order code"]["value"], "ACME-42")
        self.assertIn("Zero value", properties)
        self.assertNotIn("access_token", properties)
        self.assertNotIn("do-not-index", str(result))

        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=result)
        envelope = __import__("json").loads(package["content"].split("\n", 1)[1])
        packaged_component = next(item for item in
                                  envelope["project_retrieval"]["entities"]
                                  if item["kind"] == "schematic_symbol" and
                                  item["id"] == "sch-u3")
        self.assertIn("ACME-42", str(packaged_component["properties"]))
        self.assertNotIn("do-not-index", str(packaged_component))

        zero = index.retrieve(snapshot, "Zero value 0", limit=12)
        self.assertTrue(any(item["id"] == "sch-u3" for item in zero["entities"]))

    def test_sheet_absolute_paths_are_not_added_to_retrieval(self):
        snapshot = project_snapshot()
        snapshot["typed_state"]["project"]["sheets"] = [{
            "id": "SHEET_ABSOLUTE_PATH", "name": "ConfidentialSheet",
            "file_path": r"C:\Users\Alice\private.kicad_sch"}]
        result = ProjectIndex().retrieve(snapshot, "SHEET_ABSOLUTE_PATH", limit=12)
        sheet = next(item for item in result["entities"]
                     if item["kind"] == "schematic_sheet")
        self.assertNotIn("sheet_path", sheet)
        self.assertNotIn("C:\\Users", str(result))
        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=result)
        envelope = __import__("json").loads(package["content"].split("\n", 1)[1])
        self.assertNotIn("sheet_path", str(envelope["project_retrieval"]))
        self.assertNotIn("C:\\Users", str(envelope["project_retrieval"]))
        private_path_search = ProjectIndex().retrieve(
            snapshot, "Alice private.kicad_sch", limit=12)
        self.assertFalse(any(item["kind"] == "schematic_sheet"
                             for item in private_path_search["entities"]))

    def test_bounded_context_preserves_only_included_layer_memberships(self):
        import json
        snapshot = project_snapshot()
        snapshot["typed_state"]["project"]["extra"] = "x" * 9000
        retrieval = ProjectIndex().retrieve(snapshot, "via V1", limit=12)
        package = build_context_package(json.dumps(snapshot), [], [], char_limit=4096,
                                        project_retrieval=retrieval)
        envelope = json.loads(package["content"].split("\n", 1)[1])
        via = next(item for item in envelope["project_retrieval"]["entities"]
                   if item["kind"] == "via" and item["id"] == "V1")
        self.assertEqual(via["layer_ids"], ["F.Cu", "B.Cu"])
        self.assertEqual(package["metadata"]["project_retrieval_layer_ids"],
                         ["B.Cu", "F.Cu"])
        self.assertEqual(package["metadata"]["project_retrieval_layer_count"], 2)

    def test_bm25_finds_natural_language_entity_and_tracks_provenance(self):
        index = ProjectIndex()
        result = index.retrieve(project_snapshot(), "USB connector receptacle", limit=8)
        top = result["entities"][0]
        self.assertEqual(top["kind"], "footprint")
        self.assertEqual(top["id"], "J2")
        self.assertEqual(top["retrieval"], "lexical")
        self.assertGreater(result["stats"]["lexical_match_count"], 0)

    def test_net_relationship_is_explicit_not_claimed_as_physical_connectivity(self):
        index = ProjectIndex()
        result = index.retrieve(project_snapshot(), "GND net", limit=12)
        related = [item for item in result["entities"]
                   if item.get("relationship") in {"same_net", "board_net_member"}]
        self.assertTrue(any(item["kind"] == "track" and item["id"] == "T1" for item in related))
        self.assertTrue(any(item["kind"] == "via" and item["id"] == "V1" for item in related))
        self.assertEqual(result["relationship_semantics"], "shared_net_association_only")
        self.assertFalse(any("connected" in item for item in result["entities"]))

    def test_board_net_is_an_exact_retrievable_node_with_typed_members(self):
        result = ProjectIndex(max_entities=24).retrieve(
            project_snapshot(), "inspect GND", limit=24)
        by_key = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertIn(("board_net", "GND"), by_key)
        self.assertEqual(by_key[("board_net", "GND")]["net_id"], "GND")
        for kind, object_id in (("pad", "P1"), ("pad", "P2"),
                                ("track", "T1"), ("via", "V1")):
            self.assertIn((kind, object_id), by_key)
            self.assertEqual(by_key[(kind, object_id)]["net_id"], "GND")
        for kind, object_id in (("track", "T1"), ("via", "V1")):
            self.assertEqual(by_key[(kind, object_id)]["relationship"],
                             "board_net_member")
        self.assertEqual(result["board_net_semantics"],
                         "native_net_id_association_not_physical_continuity")

    def test_board_net_retrieval_does_not_require_a_schematic_netlist(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["nets"] = []
        project["wires"] = []
        result = ProjectIndex().retrieve(snapshot, "GND", limit=12)
        self.assertTrue(any(item["kind"] == "board_net" and item["id"] == "GND"
                            for item in result["entities"]))
        self.assertTrue(any(item["kind"] == "track" and item["id"] == "T1"
                            for item in result["entities"]))
        self.assertFalse(any(item["kind"] == "schematic_net"
                             for item in result["entities"]))

    def test_explicit_board_references_groups_diagnostics_and_sheet_membership(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        board = project["board"]
        board["groups"] = [{"id": "BG1", "name": "Power input",
                             "members": ["U3", "P1", "T1"]}]
        board["route_requests"] = [{"id": "RR1", "net_id": "GND",
                                    "from_object_id": "P1", "to_object_id": "P2"}]
        board["teardrops"] = [{"id": "TD1", "net_id": "GND",
                               "anchor_track_id": "T1",
                               "outline": [{"x_nm": 0, "y_nm": 0},
                                           {"x_nm": 100_000, "y_nm": 100_000}]}]
        schematic = {
            "id": "SCH_ROOT", "name": "Power",
            "components": project.pop("components"),
            "nets": project.pop("nets"),
            "wires": project.pop("wires"),
            "labels": project.pop("labels"),
            "sheets": [{"id": "SCH_CHILD", "name": "Regulator",
                        "file_path": "regulator.ccad.sch"}],
            "groups": [{"id": "SG1", "name": "Regulator core",
                        "members": ["sch-u3", "GND:sch-u3:GND"]}],
        }
        project["schematics"] = [schematic]
        snapshot["project_diagnostics"] = [{
            "id": "drc-zero-track-T1", "engine": "drc", "severity": "error",
            "code": "ZERO_LENGTH_TRACK", "message": "Track start and end must differ",
            "object_id": "T1"}]

        index = ProjectIndex(max_entities=32)
        group = index.retrieve(snapshot, "Power input BG1", limit=32)
        by_key = {(item["kind"], item["id"]): item for item in group["entities"]}
        self.assertIn(("board_group", "BG1"), by_key)
        self.assertEqual(by_key[("footprint", "U3")]["relationship"], "group_member")
        self.assertEqual(by_key[("pad", "P1")]["relationship"], "group_member")

        route = index.retrieve(snapshot, "route request RR1", limit=32)
        self.assertEqual(next(item for item in route["entities"]
                              if item["kind"] == "pad" and item["id"] == "P1")[
                                  "relationship"], "route_endpoint")
        anchor = index.retrieve(snapshot, "teardrop TD1", limit=32)
        self.assertEqual(next(item for item in anchor["entities"]
                              if item["kind"] == "track" and item["id"] == "T1")[
                                  "relationship"], "anchored_to")

        diagnostic = index.retrieve(snapshot, "ZERO_LENGTH_TRACK", limit=32)
        found_diagnostic = next(item for item in diagnostic["entities"]
                                if item["kind"] == "project_diagnostic")
        self.assertEqual(found_diagnostic["object_id"], "T1")
        self.assertEqual(found_diagnostic["engine"], "drc")
        self.assertEqual(next(item for item in diagnostic["entities"]
                              if item["kind"] == "track" and item["id"] == "T1")[
                                  "relationship"], "diagnostic_for")
        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=diagnostic)
        self.assertEqual(package["metadata"]["project_retrieval_kinds"].get(
            "project_diagnostic"), 1)
        gui_context = {
            "typed_state": snapshot["typed_state"],
            "project_diagnostics": snapshot["project_diagnostics"],
        }
        gui_index = ProjectIndex(max_entities=32)
        gui_diagnostic = gui_index.retrieve(gui_context, "ZERO_LENGTH_TRACK", limit=32)
        self.assertTrue(any(item["kind"] == "project_diagnostic" and
                            item["object_id"] == "T1"
                            for item in gui_diagnostic["entities"]))
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread", project_id="project-1")
            manager.configure({})
            assembled = ContextBroker().prepare(
                manager, thread_id="thread", project_revision="ui-diagnostics",
                project_snapshot=gui_context, user_request="ZERO_LENGTH_TRACK T1")
            self.assertTrue(any(item["kind"] == "project_diagnostic" and
                                item["object_id"] == "T1"
                                for item in assembled["project_retrieval"]["entities"]))

        symbol = index.retrieve(snapshot, "sch-u3", limit=32)
        u3 = next(item for item in symbol["entities"]
                  if item["kind"] == "schematic_symbol" and item["id"] == "sch-u3")
        self.assertEqual(u3["sheet_id"], "SCH_ROOT")
        page = index.retrieve(snapshot, "SCH_ROOT", limit=32)
        self.assertTrue(any(item["kind"] == "schematic_symbol" and
                            item["id"] == "sch-u3" and
                            item.get("relationship") == "same_schematic_sheet"
                            for item in page["entities"]))
        self.assertTrue(any(item["kind"] == "schematic_sheet" and
                            item["id"] == "SCH_CHILD" and
                            item.get("relationship") == "child_sheet"
                            for item in page["entities"]))

        snapshot["project_diagnostics"].clear()
        snapshot["typed_state"]["project"]["board"]["groups"].clear()
        refreshed = index.retrieve(snapshot, "ZERO_LENGTH_TRACK BG1", limit=32)
        self.assertEqual(refreshed["stats"]["index_state"], "incremental")
        self.assertFalse(any(item["kind"] in {"project_diagnostic", "board_group"}
                             for item in refreshed["entities"]))

    def test_explicit_user_groups_become_revision_current_functional_blocks(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["board"]["groups"] = [{
            "id": "GROUP_POWER", "name": "Buck converter input stage",
            "members": ["U3", "P1", "T1", "missing-native-reference"],
        }]
        project["schematics"] = [
            {"id": "ROOT", "name": "Root",
             "sheets": [{"id": "SHEET_POWER", "name": "Power conversion"}]},
            {"id": "SHEET_POWER", "name": "Power conversion",
             "components": project["components"], "nets": project["nets"],
             "groups": [{"id": "SG_POWER", "name": "Regulator feedback loop",
                         "members": ["sch-u3", "GND:sch-u3:GND"]}]},
        ]

        index = ProjectIndex(max_entities=32)
        retrieved = index.retrieve(snapshot, "buck converter input stage", limit=32)
        block = next(item for item in retrieved["entities"]
                     if item["kind"] == "functional_block" and
                     item["id"] == "group:board_group:GROUP_POWER")
        self.assertEqual(block["provenance"], "explicit_user_group")
        self.assertEqual(block["source_revision"], retrieved["revision"])
        self.assertEqual(block["members"], ["P1", "T1", "U3"])
        self.assertIn("missing-native-reference", block["source_member_ids"])
        self.assertEqual(block["related_net_ids"], ["GND"])
        self.assertEqual(block["bounds_mm"], {
            "min_x_mm": 0.0, "min_y_mm": 0.0,
            "max_x_mm": 2.0, "max_y_mm": 0.0})
        self.assertTrue(any(item["kind"] == "footprint" and item["id"] == "U3" and
                            item.get("relationship") == "group_member"
                            for item in retrieved["entities"]))
        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=retrieved)
        packaged_json = package["content"].split("\n", 1)[1]
        packaged_content = __import__("json").loads(packaged_json)
        packaged_block = next(item for item in packaged_content["project_retrieval"]["entities"]
                              if item["kind"] == "functional_block")
        self.assertEqual(packaged_block["members"], ["P1", "T1", "U3"])
        self.assertEqual(packaged_block["related_net_ids"], ["GND"])
        self.assertEqual(packaged_block["source_revision"], retrieved["revision"])
        self.assertEqual(package["metadata"]["project_retrieval_kinds"][
            "functional_block"], 1)
        sheet_result = index.retrieve(snapshot, "Power conversion", limit=32)
        sheet_block = next(item for item in sheet_result["entities"]
                           if item["kind"] == "functional_block" and
                           item["id"] == "sheet:SHEET_POWER")
        self.assertEqual(sheet_block["provenance"], "serialized_schematic_sheet")

        project["board"]["groups"].clear()
        refreshed = index.retrieve(snapshot, "buck converter input stage", limit=32)
        self.assertEqual(refreshed["stats"]["index_state"], "incremental")
        self.assertFalse(any(item["kind"] == "functional_block" and
                             item["id"] == "group:board_group:GROUP_POWER"
                             for item in refreshed["entities"]))

    def test_functional_blocks_expand_only_to_their_typed_net_nodes(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["board"]["groups"] = [{
            "id": "GROUP_POWER", "name": "Power conversion stage",
            "members": ["P1"],
        }]
        project["board"]["pads"].append({
            "id": "P3", "component_id": "U3", "pin_name": "VIN",
            "net_id": "VIN", "position": {"x_nm": 1_000_000, "y_nm": 0},
        })
        project["groups"] = [{"id": "SG_POWER", "name": "Regulator supply",
                              "members": ["GND:sch-u3:GND"]}]

        index = ProjectIndex(max_entities=32)
        board = index.retrieve(snapshot, "Power conversion stage", limit=32)
        board_block = next(item for item in board["entities"]
                           if item["kind"] == "functional_block" and
                           item["id"] == "group:board_group:GROUP_POWER")
        board_nets = [item for item in board["entities"]
                      if item["kind"] == "board_net"]
        self.assertEqual(board_block["related_net_ids"], ["GND"])
        self.assertEqual([(item["id"], item["relationship"]) for item in board_nets],
                         [("GND", "block_net_member")])

        schematic = index.retrieve(snapshot, "Regulator supply schematic", limit=32)
        sheet_block = next(item for item in schematic["entities"]
                           if item["kind"] == "functional_block" and
                           item["id"] == "group:schematic_group:SG_POWER")
        schematic_nets = [item for item in schematic["entities"]
                          if item["kind"] == "schematic_net"]
        self.assertEqual(sheet_block["related_net_ids"], ["GND"])
        self.assertEqual([(item["id"], item["relationship"])
                          for item in schematic_nets],
                         [("GND", "block_net_member")])

        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=schematic)
        payload = __import__("json").loads(package["content"].split("\n", 1)[1])
        packaged_net = next(item for item in payload["project_retrieval"]["entities"]
                            if item["kind"] == "schematic_net" and
                            item.get("relationship") == "block_net_member")
        self.assertEqual(packaged_net["relationship"], "block_net_member")
        self.assertEqual(package["metadata"]["project_retrieval_block_net_count"], 1)

    def test_functional_block_net_edges_refresh_after_member_net_changes(self):
        index = ProjectIndex(max_entities=32)
        before = project_snapshot()
        board = before["typed_state"]["project"]["board"]
        board["groups"] = [{"id": "BG_NET", "name": "Grounded group",
                            "members": ["P1"]}]
        first = index.retrieve(before, "Grounded group", limit=32)
        self.assertTrue(any(item["kind"] == "board_net" and item["id"] == "GND"
                            and item.get("relationship") == "block_net_member"
                            for item in first["entities"]))

        after = project_snapshot()
        board = after["typed_state"]["project"]["board"]
        board["groups"] = [{"id": "BG_NET", "name": "Grounded group",
                            "members": ["P1"]}]
        board["pads"][0]["net_id"] = "VIN"
        refreshed = index.retrieve(after, "Grounded group", limit=32)
        self.assertEqual(refreshed["stats"]["index_state"], "incremental")
        net_edges = {(item["kind"], item["id"]) for item in refreshed["entities"]
                     if item.get("relationship") == "block_net_member"}
        self.assertEqual(net_edges, {("board_net", "VIN")})

    def test_board_net_id_change_incrementally_removes_stale_membership(self):
        index = ProjectIndex()
        before = project_snapshot()
        index.retrieve(before, "GND", limit=20)
        after = project_snapshot()
        after["typed_state"]["project"]["board"]["tracks"][0]["net_id"] = "VCC"
        result = index.retrieve(after, "GND VCC", limit=20)
        ids = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertEqual(result["stats"]["index_state"], "incremental")
        self.assertIn(("board_net", "GND"), ids)
        self.assertIn(("board_net", "VCC"), ids)
        self.assertEqual(ids[("track", "T1")]["net_id"], "VCC")
        self.assertEqual(ids[("track", "T1")]["relationship"], "board_net_member")
        self.assertGreater(result["stats"]["updated_count"], 0)

    def test_schematic_net_expands_all_member_pins_and_symbols(self):
        result = ProjectIndex(max_entities=24).retrieve(
            project_snapshot(), "inspect GND net", limit=24)
        kinds_by_id = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertIn(("schematic_pin", "GND:sch-u3:GND"), kinds_by_id)
        self.assertIn(("schematic_pin", "GND:sch-j2:GND"), kinds_by_id)
        self.assertIn(("schematic_symbol", "sch-u3"), kinds_by_id)
        self.assertIn(("schematic_symbol", "sch-j2"), kinds_by_id)
        self.assertEqual(kinds_by_id[("schematic_pin", "GND:sch-u3:GND")]["component_id"], "U3")
        self.assertEqual(kinds_by_id[("schematic_pin", "GND:sch-u3:GND")]["net_id"], "GND")
        self.assertNotIn("position_mm", kinds_by_id[("schematic_pin", "GND:sch-u3:GND")])
        self.assertEqual(kinds_by_id[("schematic_pin", "GND:sch-u3:GND")]["symbol_id"], "sch-u3")
        self.assertEqual(kinds_by_id[("schematic_pin", "GND:sch-u3:GND")]["membership_kind"],
                         "schematic_net_member")
        self.assertEqual(kinds_by_id[("schematic_symbol", "sch-u3")]["relationship"],
                         "logical_net_member")
        self.assertEqual(result["logical_net_semantics"],
                         "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity")
        self.assertEqual(result["stats"]["index_state"], "built")

    def test_net_membership_change_invalidates_previous_graph_edges(self):
        index = ProjectIndex()
        before = project_snapshot()
        before_board = before["typed_state"]["project"]["board"]
        before_board["pads"], before_board["tracks"], before_board["vias"] = [], [], []
        index.retrieve(before, "GND net", limit=20)
        after = project_snapshot()
        after_board = after["typed_state"]["project"]["board"]
        after_board["pads"], after_board["tracks"], after_board["vias"] = [], [], []
        after["typed_state"]["project"]["nets"][0]["members"].pop()
        result = index.retrieve(after, "GND net", limit=20)
        found = {(item["kind"], item["id"]) for item in result["entities"]}
        self.assertEqual(result["stats"]["index_state"], "incremental")
        self.assertEqual(result["stats"]["updated_count"], 1)
        self.assertEqual(result["stats"]["removed_count"], 1)
        self.assertIn(("schematic_symbol", "sch-u3"), found)
        self.assertNotIn(("schematic_symbol", "sch-j2"), found)
        self.assertNotIn(("schematic_pin", "GND:sch-j2:GND"), found)

    def test_connected_schematic_pin_links_across_netlist_and_board_views(self):
        result = ProjectIndex(max_entities=24).retrieve(
            project_snapshot(), "inspect GND:sch-u3:GND", limit=24)
        by_key = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertIn(("schematic_pin", "GND:sch-u3:GND"), by_key)
        self.assertIn(("schematic_wire", "W1"), by_key)
        self.assertIn(("pad", "P1"), by_key)
        self.assertEqual(by_key[("pad", "P1")]["net_id"], "GND")
        self.assertEqual(by_key[("schematic_pin", "GND:sch-u3:GND")]["membership_kind"],
                         "schematic_net_member")
        pin = by_key[("schematic_pin", "GND:sch-u3:GND")]
        self.assertEqual(pin["retrieval"], "exact")
        board_net = by_key[("board_net", "GND")]
        self.assertNotEqual(board_net["retrieval"], "exact")
        self.assertEqual(board_net["relationship"], "matching_net_id")

    def test_schematic_net_links_to_its_typed_label(self):
        snapshot = project_snapshot()
        snapshot["typed_state"]["project"]["labels"] = [
            {"id": "L1", "text": "GND", "net_id": "GND"}]
        result = ProjectIndex().retrieve(snapshot, "GND net", limit=20)
        label = next(item for item in result["entities"]
                     if item["kind"] == "schematic_label" and item["id"] == "L1")
        self.assertEqual(label["relationship"], "logical_net_member")

    def test_component_reference_links_schematic_and_physical_objects(self):
        result = ProjectIndex().retrieve(project_snapshot(), "inspect U3 component",
                                         limit=12)
        related = [item for item in result["entities"]
                   if item.get("relationship") == "same_component"]
        self.assertTrue(any(item["kind"] == "pad" and item["id"] == "P1"
                            for item in related))
        self.assertTrue(any(item["kind"] == "schematic_symbol" and
                            item["id"] == "sch-u3" for item in result["entities"]))
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["components"][0]["id"] = "symbol-uuid-3"
        project["nets"][0]["members"][0]["component_id"] = "symbol-uuid-3"
        from_symbol = ProjectIndex().retrieve(snapshot, "symbol-uuid-3", limit=12)
        self.assertTrue(any(item["kind"] == "footprint" and item["id"] == "U3" and
                            item.get("relationship") == "same_component"
                            for item in from_symbol["entities"]))

    def test_embedded_library_definition_links_to_instance_pins_and_survives_context(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        component = project["components"][0]
        component["pins"] = [
            {"id": "instance-vin", "name": "VIN", "number": "1",
             "electrical_type": "power_in"},
            {"id": "instance-pgood", "name": "PGOOD", "number": "2",
             "electrical_type": "output"},
        ]
        component["symbol"] = {
            "name": "TPS62130", "extends": "TPS62130_BASE",
            "pins": [
                {"name": "VIN", "number": "1", "electrical_type": "power_in"},
                {"name": "PGOOD", "number": "2", "electrical_type": "output"},
                {"name": "GND", "number": "3", "electrical_type": "power_in"},
            ],
        }

        index = ProjectIndex(max_entities=24)
        definition_id = next(doc["fields"]["id"] for doc in
                             index._extract(snapshot)[1]
                             if doc["fields"]["kind"] == "library_symbol")
        result = index.retrieve(snapshot, definition_id, limit=24)
        definition = next(item for item in result["entities"]
                          if item["kind"] == "library_symbol")
        library_pin = next(item for item in result["entities"]
                           if item["kind"] == "library_pin" and
                           item.get("pin_number") == "3")
        self.assertEqual(definition["definition_source"], "embedded_project_symbol")
        self.assertEqual(definition["extends"], "TPS62130_BASE")
        self.assertEqual(library_pin["pin_name"], "GND")
        self.assertEqual(library_pin["library_symbol_id"], definition["id"])
        self.assertIn("library_pin", library_pin["relationships"])

        pin_query = index.retrieve(
            snapshot, "declared:sch-u3:1:instance-vin", limit=24)
        instance_pin = next(item for item in pin_query["entities"]
                            if item["kind"] == "schematic_pin" and
                            item.get("pin_number") == "1")
        self.assertTrue(any(item["kind"] == "library_pin" and
                            "embedded_library_pin" in item.get("relationships", ())
                            for item in pin_query["entities"]))
        package = build_context_package(json.dumps(snapshot), [], [], char_limit=8192,
                                        project_retrieval=result)
        envelope = json.loads(package["content"].split("\n", 1)[1])
        packed_pin = next(item for item in envelope["project_retrieval"]["entities"]
                          if item["kind"] == "library_pin" and
                          item.get("pin_number") == "3")
        self.assertEqual(packed_pin["pin_name"], "GND")
        self.assertEqual(packed_pin["library_symbol_id"], definition["id"])

    def test_exact_schematic_pin_to_board_pad_link_is_unambiguous_and_incremental(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["components"][0]["pins"] = [
            {"id": "vin-pin", "name": "VIN", "number": "1"},
            {"id": "pgood-pin", "name": "PGOOD", "number": "2"},
        ]
        project["board"]["pads"][0]["pin_name"] = "1"
        project["board"]["pads"].append({
            "id": "U3.2", "component_id": "U3", "pin_name": "2",
            "net_id": "PGOOD", "position": {"x_nm": 1_000_000, "y_nm": 0},
        })
        index = ProjectIndex()
        initial = index.retrieve(snapshot, "U3 VIN pin 1", limit=24)
        pad = next(item for item in initial["entities"]
                   if item["kind"] == "pad" and item["id"] == "P1")
        pin = next(item for item in initial["entities"]
                   if item["kind"] == "schematic_pin" and
                   item.get("pin_number") == "1")
        self.assertIn("physical_pad_for_pin", pad["relationships"])
        self.assertEqual(pin["pin_number"], "1")

        # A reused pad number makes the mapping ambiguous; never choose one.
        project["board"]["pads"].append({
            "id": "U3.1-duplicate", "component_id": "U3", "pin_name": "1",
            "net_id": "GND", "position": {"x_nm": 2_000_000, "y_nm": 0},
        })
        ambiguous = index.retrieve(snapshot, "U3 VIN pin 1", limit=24)
        ambiguous_pin = next(item for item in ambiguous["entities"]
                             if item["kind"] == "schematic_pin" and
                             item.get("pin_number") == "1")
        self.assertFalse(any(item["kind"] == "pad" and
                             "physical_pad_for_pin" in item.get("relationships", ())
                             for item in ambiguous["entities"]))
        self.assertEqual(ambiguous["stats"]["index_state"], "incremental")

        project["board"]["pads"].pop()
        project["board"]["pads"][0]["pin_name"] = "GND"
        removed = index.retrieve(snapshot, "U3 VIN pin 1", limit=24)
        removed_pin = next(item for item in removed["entities"]
                           if item["kind"] == "schematic_pin" and
                           item.get("pin_number") == "1")
        self.assertEqual(removed_pin["pin_number"], "1")
        self.assertFalse(any(item["kind"] == "pad" and
                             "physical_pad_for_pin" in item.get("relationships", ())
                             for item in removed["entities"]))

    def test_spatial_query_uses_coordinates_and_near_exact_object(self):
        index = ProjectIndex()
        by_origin = index.retrieve(project_snapshot(), "objects within 3 mm of 0,0", limit=12)
        spatial_ids = {item["id"] for item in by_origin["entities"]
                       if item["retrieval"] == "spatial"}
        self.assertTrue({"U3", "P1", "T1", "V1"}.intersection(spatial_ids))
        near_u3 = index.retrieve(project_snapshot(), "what is near U3", limit=12)
        self.assertFalse(any(item["kind"].startswith("schematic_") and
                             item["retrieval"] == "spatial"
                             for item in near_u3["entities"]))

    def test_nearby_component_relation_stays_in_pcb_coordinate_space(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        project["board"]["footprints"].append({
            "reference": "C_NEAR", "value": "100 nF", "footprint_name": "C_0402",
            "layer_id": "F.Cu", "position": {"x_nm": 5_000_000, "y_nm": 0}})
        project["components"].append({
            "id": "sch-c-near", "reference": "C_NEAR", "value": "100 nF",
            "position": {"x_nm": 1_000_000, "y_nm": 0}})

        result = ProjectIndex().retrieve(
            snapshot, "Which PCB footprints are within 10 mm of U3?", limit=20)
        near = next(item for item in result["entities"]
                    if item["kind"] == "footprint" and item["id"] == "C_NEAR")
        self.assertEqual(near["retrieval"], "relationship")
        self.assertEqual(near["relationship"], "near_component")
        self.assertAlmostEqual(near["distance_mm"], 5.0)
        self.assertFalse(any(item["kind"].startswith("schematic_") and
                             item["retrieval"] == "spatial"
                             for item in result["entities"]))
        self.assertEqual(result["stats"]["near_component_match_count"], 1)
        self.assertIn("pcb_coordinates_only", result["geometry_relationship_semantics"])

    def test_identified_placement_region_expands_to_intersecting_board_objects_only(self):
        snapshot = project_snapshot()
        project = snapshot["typed_state"]["project"]
        board = project["board"]
        board["placement_regions"] = [{
            "id": "PR_SPRINT997", "kind": "placement",
            "area": {"x_nm": 20_000_000, "y_nm": 20_000_000,
                     "width_nm": 10_000_000, "height_nm": 10_000_000}}]
        board["footprints"].append({
            "reference": "U_INSIDE", "value": "Controller", "footprint_name": "QFN",
            "position": {"x_nm": 29_000_000, "y_nm": 25_000_000}})
        board["footprints"].append({
            "reference": "C_NEAR", "value": "100 nF", "footprint_name": "C_0402",
            "position": {"x_nm": 28_000_000, "y_nm": 25_000_000}})
        board["texts"] = [{
            "id": "TX_CROSSES_REGION", "text": "Board label",
            "position": {"x_nm": 20_000_000, "y_nm": 25_000_000}}]
        project["components"].append({
            "id": "sch-coincident", "reference": "R_SCHEMATIC",
            "position": {"x_nm": 25_000_000, "y_nm": 25_000_000}})

        result = ProjectIndex().retrieve(
            snapshot, "Which PCB objects intersect placement region PR_SPRINT997?", limit=20)
        related = {(item["kind"], item["id"]): item for item in result["entities"]
                   if item.get("relationship") == "region_member"}
        self.assertIn(("footprint", "U_INSIDE"), related)
        self.assertIn(("board_text", "TX_CROSSES_REGION"), related)
        self.assertNotIn(("footprint", "J2"), related)
        self.assertFalse(any(item["id"] == "sch-coincident" for item in related.values()))
        self.assertEqual(result["stats"]["region_member_match_count"], 3)
        self.assertIn("axis_aligned_bounds_intersection", result["geometry_relationship_semantics"])

        combined = ProjectIndex().retrieve(
            snapshot,
            "Which PCB footprints are within 5 mm of U_INSIDE and which objects intersect placement region PR_SPRINT997?",
            limit=10)
        combined_near = next(item for item in combined["entities"]
                             if item["kind"] == "footprint" and
                             item["id"] == "C_NEAR")
        self.assertIn("near_component", combined_near["relationships"])
        self.assertIn("region_member", combined_near["relationships"])
        self.assertAlmostEqual(combined_near["distance_mm"], 1.0)
        combined_region_members = [item for item in combined["entities"]
                                   if item.get("relationship") == "region_member" or
                                   "region_member" in item.get("relationships", ())]
        self.assertGreater(len(combined_region_members), 0)
        self.assertGreater(combined["stats"]["near_component_match_count"], 0)
        self.assertGreater(combined["stats"]["region_member_match_count"], 0)

        import json
        package = build_context_package(json.dumps(snapshot), [], [], char_limit=4096,
                                        project_retrieval=result)
        envelope = json.loads(package["content"].split("\n", 1)[1])
        retrieval = envelope["project_retrieval"]
        packaged_region_members = [item for item in retrieval["entities"]
                                   if item.get("relationship") == "region_member"]
        self.assertEqual(len(packaged_region_members), 3)
        self.assertEqual(retrieval["stats"]["region_member_match_count"], 3)
        self.assertIn("pcb_coordinates_only",
                      retrieval["geometry_relationship_semantics"])
        self.assertEqual(package["metadata"]["project_retrieval_stats"][
            "region_member_match_count"], 3)
        self.assertNotIn("C:\\Users", package["content"])

        board["placement_regions"][0]["area"]["x_nm"] = 70_000_000
        moved = ProjectIndex().retrieve(
            snapshot, "Which PCB objects intersect placement region PR_SPRINT997?",
            limit=20)
        self.assertEqual(moved["stats"]["region_member_match_count"], 0)
        self.assertFalse(any(item.get("relationship") == "region_member"
                             for item in moved["entities"]))

    def test_production_limit_keeps_explicit_geometry_relations_ahead_of_lexical_noise(self):
        project = project_snapshot()["typed_state"]["project"]
        project["board"].setdefault("footprints", []).extend((
            {"reference": "JAC1", "value": "AC input",
             "footprint_name": "Connector_PinHeader_2.54mm", "layer_id": "F.Cu",
             "position": {"x_nm": 8_000_000, "y_nm": 17_000_000}},
            {"reference": "C_NEAR", "value": "100 nF", "footprint_name": "C_0402",
             "layer_id": "F.Cu",
             "position": {"x_nm": 10_000_000, "y_nm": 17_000_000}},
        ))
        project["board"].setdefault("placement_regions", []).append({
            "id": "PR_SPRINT997", "kind": "placement",
            "area": {"x_nm": 7_000_000, "y_nm": 16_000_000,
                     "width_nm": 5_000_000, "height_nm": 2_000_000},
        })
        snapshot = {"typed_state": {"available": True, "project": project}}
        query = ("Which PCB footprints are within 5 mm of JAC1, and which PCB objects "
                 "intersect placement region PR_SPRINT997?")

        result = ProjectIndex().retrieve(snapshot, query)
        packaged = build_context_package(json.dumps(snapshot), [], [], char_limit=8192,
                                         project_retrieval=result)
        envelope = json.loads(packaged["content"].split("\n", 1)[1])
        entities = envelope["project_retrieval"]["entities"]
        self.assertTrue(any(item["id"] == "C_NEAR" and
                            "near_component" in item.get("relationships", ())
                            for item in entities))
        self.assertTrue(any("region_member" in item.get("relationships", ())
                            for item in entities))
        self.assertGreater(packaged["metadata"]["project_retrieval_stats"][
            "near_component_match_count"], 0)
        self.assertGreater(packaged["metadata"]["project_retrieval_stats"][
            "region_member_match_count"], 0)

        live_signals = extract_context_signals(
            query, goal=query, project_id="proj-sprint160-placement-crash-ci-final",
            active_editor="pcb", selected_objects=("JAC1",))
        with tempfile.TemporaryDirectory() as memory_dir:
            live_manager = MemoryManager(
                MemoryStore(Path(memory_dir) / "memory.json"),
                thread_id="thread-geometry",
                project_id="proj-sprint160-placement-crash-ci-final")
            live_manager.configure({})
            live_context = ContextBroker().prepare(
                live_manager, thread_id="thread-geometry",
                project_revision="fixture-revision", user_request=query,
                goal=query, project_id="proj-sprint160-placement-crash-ci-final",
                active_editor="pcb", selected_objects=("JAC1",),
                signals=live_signals, project_snapshot=snapshot,
                active_layer="F.Cu", active_net="AC1")
        self.assertGreater(live_context["project_retrieval"]["stats"][
            "near_component_match_count"], 0)
        self.assertGreater(live_context["project_retrieval"]["stats"][
            "region_member_match_count"], 0)

        context_snapshot = json.loads(json.dumps(snapshot))
        context_snapshot["project_diagnostics"] = [
            {"id": f"diag-{index}", "code": f"FIXTURE_{index}",
             "message": "Fixture diagnostic linked to selected pad",
             "object_id": "JAC1.1", "severity": "warning", "engine": "drc"}
            for index in range(12)]
        selected_pad = "JAC1.1"
        noisy_signals = extract_context_signals(
            query, goal=query, project_id="proj-sprint160-placement-crash-ci-final",
            active_editor="pcb", selected_objects=(selected_pad,))
        noisy_retrieval = ProjectIndex().retrieve(
            context_snapshot, noisy_signals["query"], active_layer="F.Cu",
            active_net="AC1", selected_objects=(selected_pad,), limit=10)
        self.assertGreater(noisy_retrieval["stats"]["exact_match_count"], 10)
        noisy_package = build_context_package(
            json.dumps(context_snapshot), [], [], char_limit=8192,
            project_retrieval=noisy_retrieval)
        self.assertGreater(noisy_package["metadata"]["project_retrieval_stats"][
            "near_component_match_count"], 0)
        self.assertGreater(noisy_package["metadata"]["project_retrieval_stats"][
            "region_member_match_count"], 0)
        self.assertTrue(any(entity["id"] == "C_NEAR" and
                            "near_component" in entity.get("relationships", ())
                            for entity in noisy_retrieval["entities"]))

    def test_relationship_only_changes_invalidate_incremental_graph_edges(self):
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["groups"] = [{"id": "GR_SPRINT997", "members": ["U3"]}]
        index = ProjectIndex()
        initial = index.retrieve(snapshot, "GR_SPRINT997", limit=20)
        self.assertTrue(any(item["kind"] == "footprint" and item["id"] == "U3" and
                            item.get("relationship") == "group_member"
                            for item in initial["entities"]))

        board["groups"][0]["members"] = ["J2"]
        updated = index.retrieve(snapshot, "GR_SPRINT997", limit=20)
        self.assertEqual(updated["stats"]["index_state"], "incremental")
        self.assertTrue(any(item["kind"] == "footprint" and item["id"] == "J2" and
                            item.get("relationship") == "group_member"
                            for item in updated["entities"]))
        self.assertFalse(any(item["kind"] == "footprint" and item["id"] == "U3" and
                             item.get("relationship") == "group_member"
                             for item in updated["entities"]))

    def test_revision_update_removes_deleted_rows_and_only_reindexes_changes(self):
        index = ProjectIndex()
        before = project_snapshot()
        index.retrieve(before, "USB connector")
        after = project_snapshot()
        after["typed_state"]["project"]["board"]["vias"] = []
        after["typed_state"]["project"]["board"]["footprints"][1]["value"] = "USB-C receptacle"
        result = index.retrieve(after, "V1 USB-C receptacle")
        self.assertEqual(result["stats"]["index_state"], "incremental")
        self.assertEqual(result["stats"]["removed_count"], 1)
        self.assertEqual(result["stats"]["updated_count"], 1)
        self.assertEqual(result["stats"]["unchanged_count"], 13)
        ids = {item["id"] for item in result["entities"]}
        self.assertNotIn("V1", ids)
        self.assertEqual(index.retrieve(after, "USB-C receptacle")["stats"]["index_state"],
                         "cached")

    def test_ephemeral_ui_state_does_not_change_project_index_revision(self):
        index = ProjectIndex()
        first = project_snapshot()
        initial = index.retrieve(first, "U3")
        changed_ui = project_snapshot()
        changed_ui["selection"]["items"] = [{"object_id": "J2"}]
        changed_ui["active_pcb_layer_id"] = "B.Cu"
        result = index.retrieve(changed_ui, "U3")
        self.assertEqual(result["stats"]["index_state"], "cached")
        self.assertEqual(result["revision"], initial["revision"])
        self.assertEqual(index.revision("project-1"), initial["revision"])

    def test_bounds_and_secret_redaction_are_truthful(self):
        snapshot = project_snapshot()
        snapshot["typed_state"]["project"]["board"]["texts"] = [
            {"id": "leak", "text": "api_key=synthetic-secret-value",
             "position": {"x_nm": 0, "y_nm": 0}},
        ]
        result = ProjectIndex(max_entities=2, max_chars=600).retrieve(
            snapshot, "GND USB connector", limit=20)
        self.assertLessEqual(len(result["entities"]), 2)
        self.assertLessEqual(result["characters"], 600)
        self.assertNotIn("synthetic-secret-value", str(result))
        self.assertGreater(result["stats"]["omitted_count"], 0)

    def test_retrieved_project_facts_survive_large_snapshot_compaction(self):
        snapshot = project_snapshot()
        snapshot["project_extra"] = "x" * 9000
        retrieval = ProjectIndex().retrieve(snapshot, "GND net", limit=20)
        package = build_context_package(
            __import__("json").dumps(snapshot), [], [], char_limit=4096,
            project_retrieval=retrieval)
        self.assertTrue(package["metadata"]["project_snapshot_omitted"])
        self.assertIn("project_retrieval", package["metadata"]["sources"])
        envelope = __import__("json").loads(package["content"].split("\n", 1)[1])
        self.assertTrue(any(item["kind"] == "schematic_pin" for item in
                            envelope["project_retrieval"]["entities"]))
        self.assertEqual(envelope["project_retrieval"]["relationship_semantics"],
                         "shared_net_association_only")
        self.assertEqual(envelope["project_retrieval"]["board_net_semantics"],
                         "native_net_id_association_not_physical_continuity")
        self.assertTrue(any(item["kind"] == "board_net" and item["id"] == "GND"
                            for item in envelope["project_retrieval"]["entities"]))
        self.assertEqual(
            envelope["project_retrieval"]["logical_net_semantics"],
            "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity")
        self.assertGreater(package["metadata"]["project_retrieval_kinds"].get(
            "schematic_pin", 0), 0)
        self.assertGreater(package["metadata"]["project_retrieval_kinds"].get(
            "board_net", 0), 0)
        self.assertLessEqual(package["metadata"]["content_size"], 4096)

    def test_explicit_functional_block_native_net_edge_survives_context_packaging(self):
        snapshot = project_snapshot()
        board = snapshot["typed_state"]["project"]["board"]
        board["groups"] = [{"id": "GROUP_RETURN", "name": "Return path",
                            "members": ["P1"]}]
        # P1's typed pad is assigned to native board net GND in this fixture.
        retrieval = ProjectIndex().retrieve(
            snapshot, "Find the Return path functional block and its native PCB net.",
            active_net="GND")
        edge = next(entity for entity in retrieval["entities"]
                    if entity["kind"] == "board_net" and entity["id"] == "GND")
        self.assertEqual(edge["relationship"], "block_net_member")
        package = build_context_package(
            json.dumps(snapshot), [], [], char_limit=8192,
            project_retrieval=retrieval)
        self.assertEqual(package["metadata"]["project_retrieval_block_net_count"], 1)
        self.assertIn("block_net_member", str(package["content"]))

    def test_context_broker_reuses_project_index_and_refreshes_changed_geometry(self):
        with tempfile.TemporaryDirectory() as temp:
            manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                                    thread_id="thread", project_id="project-1")
            manager.configure({})
            broker = ContextBroker()
            first = broker.prepare(
                manager, thread_id="thread", project_revision="ui-1",
                project_snapshot=project_snapshot(), user_request="find USB connector")
            self.assertTrue(first["project_retrieval"]["available"])
            self.assertTrue(any(item["id"] == "J2" for item in
                                first["project_retrieval"]["entities"]))
            changed = project_snapshot()
            changed["typed_state"]["project"]["board"]["footprints"][1][
                "position"]["x_nm"] = 35_000_000
            second = broker.prepare(
                manager, thread_id="thread", project_revision="ui-2",
                project_snapshot=changed, user_request="find USB connector")
            self.assertEqual(second["project_retrieval"]["stats"]["index_state"],
                             "incremental")
            j2 = next(item for item in second["project_retrieval"]["entities"]
                      if item["kind"] == "footprint" and item["id"] == "J2")
            self.assertEqual(j2["position_mm"]["x"], 35.0)
            other_project = project_snapshot()
            other_project["typed_state"]["project"]["id"] = "project-2"
            broker.prepare(manager, thread_id="thread", project_revision="other",
                           project_snapshot=other_project,
                           user_request="find U3")
            self.assertEqual(len(broker._project_indexes), 1)


if __name__ == "__main__":
    unittest.main()
