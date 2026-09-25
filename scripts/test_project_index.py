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
        result = ProjectIndex().retrieve(
            snapshot, "objects in rectangle from -1,-1 to 3,2 mm", limit=32)
        diagnostics = [item for item in result["entities"]
                       if item["kind"] == "project_diagnostic"]
        self.assertTrue(any(item["code"] == "TRACK_CLEARANCE" and
                            item["object_id"] == "T1" for item in diagnostics))
        self.assertTrue(any(item["code"] == "PIN_NOT_CONNECTED" and
                            item["object_id"] == "sch-u3" for item in diagnostics))
        self.assertFalse(any(item["code"] == "FAR_AWAY" for item in diagnostics))
        self.assertTrue(all(item["retrieval"] == "relationship" and
                            item["relationship"] == "diagnostic_for"
                            for item in diagnostics))

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
