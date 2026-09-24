"""Contract for exact, lexical, relationship, spatial, and incremental retrieval."""

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from project_index import ProjectIndex
from context_package import build_context_package
from context_broker import ContextBroker
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


class ProjectIndexTests(unittest.TestCase):
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
                   if item.get("relationship") == "same_net"]
        self.assertTrue(any(item["kind"] == "track" and item["id"] == "T1" for item in related))
        self.assertTrue(any(item["kind"] == "via" and item["id"] == "V1" for item in related))
        self.assertEqual(result["relationship_semantics"], "shared_net_association_only")
        self.assertFalse(any("connected" in item for item in result["entities"]))

    def test_schematic_net_expands_all_member_pins_and_symbols(self):
        result = ProjectIndex().retrieve(project_snapshot(), "inspect GND net", limit=20)
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
        result = ProjectIndex().retrieve(project_snapshot(), "GND:sch-u3:GND", limit=20)
        by_key = {(item["kind"], item["id"]): item for item in result["entities"]}
        self.assertIn(("schematic_pin", "GND:sch-u3:GND"), by_key)
        self.assertIn(("schematic_wire", "W1"), by_key)
        self.assertIn(("pad", "P1"), by_key)
        self.assertEqual(by_key[("pad", "P1")]["net_id"], "GND")
        self.assertEqual(by_key[("schematic_pin", "GND:sch-u3:GND")]["membership_kind"],
                         "schematic_net_member")

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

    def test_spatial_query_uses_coordinates_and_near_exact_object(self):
        index = ProjectIndex()
        by_origin = index.retrieve(project_snapshot(), "objects within 3 mm of 0,0", limit=12)
        spatial_ids = {item["id"] for item in by_origin["entities"]
                       if item["retrieval"] == "spatial"}
        self.assertTrue({"U3", "P1", "T1", "V1"}.intersection(spatial_ids))
        near_u3 = index.retrieve(project_snapshot(), "what is near U3", limit=12)
        self.assertTrue(any(item["retrieval"] == "spatial" for item in near_u3["entities"]))

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
        self.assertEqual(result["stats"]["unchanged_count"], 12)
        ids = {item["id"] for item in result["entities"]}
        self.assertNotIn("V1", ids)
        self.assertEqual(index.retrieve(after, "USB-C receptacle")["stats"]["index_state"],
                         "cached")

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
        self.assertEqual(
            envelope["project_retrieval"]["logical_net_semantics"],
            "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity")
        self.assertGreater(package["metadata"]["project_retrieval_kinds"].get(
            "schematic_pin", 0), 0)
        self.assertLessEqual(package["metadata"]["content_size"], 4096)

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
