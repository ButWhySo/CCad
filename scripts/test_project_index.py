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
