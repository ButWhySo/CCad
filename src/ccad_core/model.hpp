#pragma once

#include "ccad_core/geometry.hpp"
#include "ccad_core/pin_type.hpp"
#include "ccad_core/symbol.hpp"

#include <map>
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>

namespace ccad {

struct SchPin {
  std::string id;
  std::string name;
  std::string number;
  ElectricalPinType electrical_type = ElectricalPinType::Unspecified;
  GraphicPinShape shape = GraphicPinShape::Line;
  PinOrientation orientation = PinOrientation::Right;
  Point position;
  Length length;
  Length name_text_size;
  Length num_text_size;
  bool visible = true;
};

struct SchField {
  std::string id;
  std::string name;
  std::string text;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  bool visible = true;
};

struct SchSymbol {
  std::string id;
  std::string lib_id;
  std::string reference;
  int unit = 1;
  Point position;
  double rotation_degrees = 0.0;
  bool mirror_x = false;
  bool mirror_y = false;
  bool in_bom = true;
  bool on_board = true;
  std::vector<SchField> fields;
  std::vector<SchPin> pins;
  std::optional<Symbol> symbol = std::nullopt;
};

struct NetMember {
  std::string component_id;
  std::string pin_name;
};

struct Net {
  std::string id;
  std::vector<NetMember> members;
};

struct SchWire {
  std::string id;
  Point start;
  Point end;
  std::string net_id;
};

struct SchBus {
  std::string id;
  Point start;
  Point end;
  std::string bus_id;
  std::vector<std::string> net_ids;
};

enum class LabelType { Local, Global, Hierarchical };

struct SchLabel {
  std::string id;
  std::string text;
  std::string net_id;
  Point position;
  double rotation_degrees = 0.0;
  LabelType type = LabelType::Local;
};


struct SchPowerSymbol {
  std::string id;
  std::string value;
  std::string net_id;
  Point position;
  double rotation_degrees = 0.0;
};

struct SchGroup {
  std::string id;
  std::string name;
  std::vector<std::string> members;
};

struct SchJunction {
  std::string id;
  Point position;
  Length diameter = millimeters(0); // 0 means default
  std::string color = "";
};

struct SchNoConnect {
  std::string id;
  Point position;
};

struct SchSheetPin {
  std::string id;
  std::string name;
  std::string type;
  Point position;
};

struct SchSheet {
  std::string id;
  std::string name;
  std::string file_path;
  Point position;
  Size size;
  std::vector<SchSheetPin> pins;
};

struct SchText {
  std::string id;
  std::string text;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
};

struct SchTextBox {
  std::string id;
  std::string text;
  Rect area;
  Size size;
};

struct SchGraphic {
  std::string id;
  std::string kind; // "line", "rectangle", "polygon"
  Point start;
  Point end;
  std::vector<Point> points;
  Length width;
  std::string color;
};


struct SchMarker {
  std::string id;
  std::string kind;
  std::string severity;
  Point position;
};

struct SchBusEntry {
  std::string id;
  std::string kind; // "wire" or "bus"
  Point position;
  Size size;
};

struct SchBitmap {
  std::string id;
  std::string data; // Base64 encoded image
  Point position;
  double scale = 1.0;
};

struct SchRuleArea {
  std::string id;
  std::string name;
  std::vector<Point> outline;
  bool locked = false;
};

struct SchTableCell {
  int row = 0;
  int col = 0;
  std::string text;
};

struct SchTable {
  std::string id;
  Point position;
  int rows = 1;
  int cols = 1;
  Size size;
  std::vector<SchTableCell> cells;
};

struct Constraint {
  std::string id;
  std::string kind;
  std::string target;
  std::string value;
};

struct Layer {
  std::string id;
  std::string name;
  std::string kind;
  bool visible = true;
};

struct DesignRules {
  Length copper_clearance = millimeters(0.20);
  Length min_track_width = millimeters(0.15);
  Length max_track_width = millimeters(0.0);
  Length min_via_annular_ring = millimeters(0.10);
  Length min_connection = millimeters(0.0);
  Length min_via_diameter = millimeters(0.50);
  Length max_via_diameter = millimeters(0.0);
  Length min_through_hole_drill = millimeters(0.30);
  Length min_microvia_diameter = millimeters(0.20);
  Length min_microvia_drill = millimeters(0.10);
  Length min_hole_to_hole = millimeters(0.25);
  Length hole_clearance = millimeters(0.25);
  Length copper_edge_clearance = millimeters(0.50);
  Length silk_clearance = millimeters(0.0);
  Length min_text_height = millimeters(0.0);
  Length min_text_thickness = millimeters(0.0);
  double min_track_angle_degrees = 0.0;
  double max_track_angle_degrees = 0.0;
  Length min_track_segment_length = millimeters(0.0);
  Length max_track_segment_length = millimeters(0.0);
  Length min_groove_width = millimeters(0.0);
  Length solder_mask_expansion = millimeters(0.0);
  Length solder_mask_min_width = millimeters(0.0);
  Length solder_mask_to_copper_clearance = millimeters(0.0);
  Length solder_paste_margin = millimeters(0.0);
  double solder_paste_margin_ratio = 0.0;
  Length board_thickness = millimeters(1.60);
  bool use_height_for_length_calcs = true;
  bool tent_vias_front = true;
  bool tent_vias_back = true;
  bool cover_vias_front = false;
  bool cover_vias_back = false;
  bool plug_vias_front = false;
  bool plug_vias_back = false;
  bool cap_vias = false;
  bool fill_vias = false;
  std::vector<std::string> drc_exclusions;
  std::vector<std::string> ratsnest_exclusions;
};

enum class PadstackMode { Normal, FrontInnerBack, Custom };

enum class PadShape { Circle, Rectangle, Oval, Trapezoid, RoundRect, ChamferedRect, Custom };

enum class DrillShape { Undefined, Circle, Oval };

struct PadstackDrillProps {
  Size size;
  DrillShape shape = DrillShape::Undefined;
  std::string start_layer;
  std::string end_layer;
  std::optional<bool> is_capped = std::nullopt;
  std::optional<bool> is_filled = std::nullopt;
};

struct PadstackPostMachiningProps {
  std::optional<std::string> mode = std::nullopt;
  Length size;
  Length depth;
  double angle_degrees = 0.0;
};

struct PadstackShapeProps {
  PadShape shape = PadShape::Circle;
  PadShape anchor_shape = PadShape::Rectangle;
  Size size;
  Point offset;
  double roundrect_rratio = 0.0;
  double chamfer_ratio = 0.0;
  int chamfer_positions = 0;
  Size trapezoid_delta_size;
};

struct PadstackCopperLayerProps {
  PadstackShapeProps shape;
  std::optional<Length> clearance = std::nullopt;
  std::optional<std::string> zone_connection = std::nullopt;
  std::optional<Length> thermal_gap = std::nullopt;
  std::optional<Length> thermal_spoke_width = std::nullopt;
  std::optional<double> thermal_spoke_angle_degrees = std::nullopt;
};

struct PadstackOuterLayerProps {
  std::optional<bool> has_solder_mask = std::nullopt;
  std::optional<bool> has_covering = std::nullopt;
  std::optional<bool> has_plugging = std::nullopt;
  std::optional<bool> has_solder_paste = std::nullopt;
  std::optional<Length> solder_mask_margin = std::nullopt;
  std::optional<Length> solder_paste_margin = std::nullopt;
  std::optional<double> solder_paste_margin_ratio = std::nullopt;
};

struct Padstack {
  PadstackMode mode = PadstackMode::Normal;
  std::vector<std::string> layer_set;
  std::unordered_map<std::string, PadstackCopperLayerProps> copper_props;
  
  PadstackDrillProps drill;
  std::optional<PadstackDrillProps> secondary_drill = std::nullopt;
  std::optional<PadstackDrillProps> tertiary_drill = std::nullopt;
  
  PadstackPostMachiningProps front_post_machining;
  PadstackPostMachiningProps back_post_machining;
  
  PadstackOuterLayerProps front_outer_layers;
  PadstackOuterLayerProps back_outer_layers;
  
  std::string unconnected_layer_mode = "keep_all";
};

struct Pad {
  std::string id;
  std::string component_id;
  std::string pin_name;
  std::string net_id;
  std::string type;
  Point position;
  double rotation_degrees = 0.0;
  std::string pin_type = "";
  std::optional<Length> pad_to_die_length = std::nullopt;
  std::optional<double> pad_to_die_delay = std::nullopt;
  bool locked = false;
  Padstack padstack;
  bool teardrops_enabled = false;
};

struct Via {
  std::string id;
  std::string net_id;
  Point position;
  Length diameter;
  Length drill;
  std::string start_layer_id;
  std::string end_layer_id;
  bool locked = false;
  bool teardrops_enabled = false;
};

struct TrackSegment {
  std::string id;
  std::string net_id;
  std::string layer_id;
  Point start;
  Point end;
  Length width;
  std::string source_route_request_id;
  bool locked = false;
};

struct TrackArc {
  std::string id;
  std::string net_id;
  std::string layer_id;
  Point start;
  Point mid;
  Point end;
  Length width;
  bool locked = false;
};

struct BoardGraphic {
  std::string id;
  std::string kind;
  std::string layer_id;
  Point start;
  Point end;
  std::optional<Point> mid = std::nullopt;
  std::optional<double> angle_degrees = std::nullopt;
  Length width;
  bool locked = false;
};

struct BoardText {
  std::string id;
  std::string layer_id;
  std::string text;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  bool mirrored = false;
  Length stroke_width = millimeters(0.0);
  bool locked = false;
};

struct BoardDimension {
  std::string id;
  std::string layer_id;
  std::string kind;
  std::string text;
  Point start;
  Point end;
  Point text_position;
  bool locked = false;
};

struct BoardGroup {
  std::string id;
  std::string name;
  std::vector<std::string> members;
};

enum class BarcodeType { Code39, Code128, DataMatrix, QRCode, MicroQRCode };
enum class BarcodeEcc { Low, Medium, Quartile, High };

struct BoardBarcode {
  std::string id;
  std::string layer_id;
  std::string text;
  BarcodeType kind = BarcodeType::QRCode;
  BarcodeEcc error_correction = BarcodeEcc::Low;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  Size margin;
  bool locked = false;
};

struct BoardReferenceImage {
  std::string id;
  std::string layer;
  std::string data; // Base64 encoded image
  double x_mm = 0.0;
  double y_mm = 0.0;
  double scale = 1.0;
  double opacity = 1.0;
};

struct BoardTableCell {
  int row = 0;
  int col = 0;
  std::string text;
};

struct BoardTable {
  std::string id;
  std::string layer;
  double x_mm = 0.0;
  double y_mm = 0.0;
  int rows = 1;
  int cols = 1;
  double width_mm = 0.0;
  double height_mm = 0.0;
  std::vector<BoardTableCell> cells;
};

enum class TargetShape { Plus, X };

struct BoardTarget {
  std::string id;
  std::string layer_id;
  TargetShape shape = TargetShape::Plus;
  Length position_x = millimeters(0);
  Length position_y = millimeters(0);
  Length size = millimeters(5);
  Length line_width = millimeters(0.25);
};

struct BoardZone {
  std::string id;
  std::string name;
  std::string net_id;
  std::vector<std::string> layer_ids;
  std::vector<Point> outline;
  std::vector<std::vector<Point>> holes;
  int priority = 0;
  Length clearance;
  Length min_thickness;
  bool fill_enabled = true;
  std::string pad_connection;
  bool locked = false;
};

struct BoardTeardrop {
  std::string id;
  std::string net_id;
  std::string layer_id;
  std::vector<Point> outline;
  bool locked = false;
  // Back-references to the pad or via that anchors this teardrop.
  // Exactly one of these should be non-empty.  Used for incremental update
  // (re-generate only teardrops whose anchor was modified) and for DRC
  // reporting (name the anchoring object in diagnostics).
  std::string anchor_pad_id;
  std::string anchor_via_id;
  // Track id that forms the approaching-track side of this teardrop.
  std::string anchor_track_id;
};

struct RouteRequest {
  std::string id;
  std::string net_id;
  std::string from_object_id;
  std::string to_object_id;
  std::string preferred_layer_id;
  std::string policy;
  Length width;
};

struct Keepout {
  std::string id;
  std::string kind;
  Rect area;
};

struct PlacementRegion {
  std::string id;
  std::string kind;
  Rect area;
};

struct BoardFootprint {
  std::string reference;
  std::string value;
  std::string footprint_name;
  std::string layer_id;
  Point position;
  double rotation_degrees = 0.0;
  bool exclude_from_bom = false;
  bool locked = false;
  std::vector<std::vector<Point>> front_courtyard;
  std::vector<std::vector<Point>> back_courtyard;
};

struct Board {
  Rect outline;
  DesignRules design_rules;
  std::vector<Layer> layers;
  std::vector<BoardFootprint> footprints;
  std::vector<PlacementRegion> placement_regions;
  std::vector<Keepout> keepouts;
  std::vector<Pad> pads;
  std::vector<Via> vias;
  std::vector<TrackSegment> tracks;
  std::vector<TrackArc> track_arcs;
  std::vector<BoardGraphic> graphics;
  std::vector<BoardText> texts;
  std::vector<BoardDimension> dimensions;
  std::vector<BoardGroup> groups;
  std::vector<BoardBarcode> barcodes;
  std::vector<BoardReferenceImage> reference_images;
  std::vector<BoardTable> tables;
  std::vector<BoardTarget> targets;
  std::vector<BoardZone> zones;
  std::vector<RouteRequest> route_requests;
  std::vector<BoardTeardrop> teardrops;
};

struct Schematic {
  std::string id;
  std::string name;
  std::vector<SchSymbol> symbols;
  std::vector<SchWire> wires;
  std::vector<SchBus> buses;
  std::vector<SchLabel> labels;
  std::vector<SchPowerSymbol> power_symbols;
  std::vector<SchJunction> junctions;
  std::vector<SchNoConnect> no_connects;
  std::vector<SchSheet> sheets;
  std::vector<SchText> texts;
  std::vector<SchTextBox> textboxes;
  std::vector<SchGraphic> graphics;
  std::vector<SchMarker> markers;
  std::vector<SchBusEntry> bus_entries;
  std::vector<SchBitmap> bitmaps;
  std::vector<SchRuleArea> rule_areas;
  std::vector<SchTable> tables;
  std::vector<SchGroup> groups;
  std::vector<Net> nets;
  std::vector<Constraint> constraints;
};

struct Project {
  int schema_version = 2;
  std::string id;
  std::string name;
  std::map<std::string, std::string> text_variables;
  std::vector<Board> boards;
  std::vector<Schematic> schematics;
};

inline const Board* primaryBoard(const Project& project) {
  return project.boards.empty() ? nullptr : &project.boards.front();
}

inline Board* primaryBoard(Project& project) {
  return project.boards.empty() ? nullptr : &project.boards.front();
}

inline const Schematic* primarySchematic(const Project& project) {
  return project.schematics.empty() ? nullptr : &project.schematics.front();
}

inline Schematic* primarySchematic(Project& project) {
  return project.schematics.empty() ? nullptr : &project.schematics.front();
}

inline Schematic& ensurePrimarySchematic(Project& project) {
  if (project.schematics.empty()) {
    project.schematics.push_back(Schematic{});
  }
  return project.schematics.front();
}

}  // namespace ccad
