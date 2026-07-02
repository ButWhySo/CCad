#include "ccad_core/placement.hpp"
#include "ccad_core/pin_type.hpp"
#include "ccad_core/serialize.hpp"

#include "ccad_core/json.hpp"
#include "ccad_core/symbol_json_reader.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ccad {
namespace {

void writeField(std::ostringstream& out, const int indent, const std::string& key,
                const std::string& value, const bool comma = true) {
  out << std::string(indent, ' ') << '"' << key << "\": \"" << escapeJson(value) << '"';
  if (comma) {
    out << ',';
  }
  out << '\n';
}

void writePoint(std::ostringstream& out, const int indent, const Point& point) {
  out << std::string(indent, ' ') << "{\n";
  out << std::string(indent + 2, ' ') << "\"x_nm\": " << point.x.nanometers << ",\n";
  out << std::string(indent + 2, ' ') << "\"y_nm\": " << point.y.nanometers << "\n";
  out << std::string(indent, ' ') << "}";
}

void writeSize(std::ostringstream& out, const int indent, const Size& size) {
  out << std::string(indent, ' ') << "{\n";
  out << std::string(indent + 2, ' ') << "\"width_nm\": " << size.width.nanometers << ",\n";
  out << std::string(indent + 2, ' ') << "\"height_nm\": " << size.height.nanometers << "\n";
  out << std::string(indent, ' ') << "}";
}

void writeSymbol(std::ostringstream& out, const int indent, const Symbol& symbol) {
  const std::string pad(indent, ' ');
  out << "{\n";
  writeField(out, indent + 2, "name", symbol.name);
  writeField(out, indent + 2, "extends", symbol.extends);

  out << pad << "  \"properties\": [\n";
  for (std::size_t i = 0; i < symbol.properties.size(); ++i) {
    const SymbolProperty& property = symbol.properties.at(i);
    out << pad << "    {\n";
    writeField(out, indent + 6, "name", property.name);
    writeField(out, indent + 6, "value", property.value, false);
    out << pad << "    }" << (i + 1 == symbol.properties.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"pins\": [\n";
  for (std::size_t i = 0; i < symbol.pins.size(); ++i) {
    const SymbolPin& pin = symbol.pins.at(i);
    out << pad << "    {\n";
    writeField(out, indent + 6, "name", pin.name);
    writeField(out, indent + 6, "number", pin.number);
    writeField(out, indent + 6, "electrical_type", to_string(pin.electrical_type));
    writeField(out, indent + 6, "graphical_style", to_string(pin.shape));
    out << pad << "      \"x_nm\": " << pin.position.x.nanometers << ",\n";
    out << pad << "      \"y_nm\": " << pin.position.y.nanometers << ",\n";
    writeField(out, indent + 6, "orientation", to_string(pin.orientation));
    out << pad << "      \"length_nm\": " << pin.length.nanometers << '\n';
    out << pad << "    }" << (i + 1 == symbol.pins.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"rectangles\": [\n";
  for (std::size_t i = 0; i < symbol.rectangles.size(); ++i) {
    const SymbolRectangle& rectangle = symbol.rectangles.at(i);
    out << pad << "    {\n";
    out << pad << "      \"start_x\": " << rectangle.start.x.nanometers << ",\n";
    out << pad << "      \"start_y\": " << rectangle.start.y.nanometers << ",\n";
    out << pad << "      \"end_x\": " << rectangle.end.x.nanometers << ",\n";
    out << pad << "      \"end_y\": " << rectangle.end.y.nanometers << ",\n";
    out << pad << "      \"stroke_width\": " << rectangle.stroke_width.nanometers << ",\n";
    writeField(out, indent + 6, "fill_type", rectangle.fill_type, false);
    out << pad << "    }" << (i + 1 == symbol.rectangles.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"lines\": [\n";
  for (std::size_t i = 0; i < symbol.lines.size(); ++i) {
    const SymbolLine& line = symbol.lines.at(i);
    out << pad << "    {\n";
    out << pad << "      \"start_x\": " << line.start.x.nanometers << ",\n";
    out << pad << "      \"start_y\": " << line.start.y.nanometers << ",\n";
    out << pad << "      \"end_x\": " << line.end.x.nanometers << ",\n";
    out << pad << "      \"end_y\": " << line.end.y.nanometers << ",\n";
    out << pad << "      \"stroke_width\": " << line.stroke_width.nanometers << '\n';
    out << pad << "    }" << (i + 1 == symbol.lines.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"arcs\": [\n";
  for (std::size_t i = 0; i < symbol.arcs.size(); ++i) {
    const SymbolArc& arc = symbol.arcs.at(i);
    out << pad << "    {\n";
    out << pad << "      \"start_x\": " << arc.start.x.nanometers << ",\n";
    out << pad << "      \"start_y\": " << arc.start.y.nanometers << ",\n";
    out << pad << "      \"end_x\": " << arc.end.x.nanometers << ",\n";
    out << pad << "      \"end_y\": " << arc.end.y.nanometers << ",\n";
    out << pad << "      \"center_x\": " << arc.center.x.nanometers << ",\n";
    out << pad << "      \"center_y\": " << arc.center.y.nanometers << ",\n";
    out << pad << "      \"stroke_width\": " << arc.stroke_width.nanometers << '\n';
    out << pad << "    }" << (i + 1 == symbol.arcs.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"circles\": [\n";
  for (std::size_t i = 0; i < symbol.circles.size(); ++i) {
    const SymbolCircle& circle = symbol.circles.at(i);
    out << pad << "    {\n";
    out << pad << "      \"center_x\": " << circle.center.x.nanometers << ",\n";
    out << pad << "      \"center_y\": " << circle.center.y.nanometers << ",\n";
    out << pad << "      \"radius\": " << circle.radius.nanometers << ",\n";
    out << pad << "      \"stroke_width\": " << circle.stroke_width.nanometers << ",\n";
    writeField(out, indent + 6, "fill_type", circle.fill_type, false);
    out << pad << "    }" << (i + 1 == symbol.circles.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"polylines\": [\n";
  for (std::size_t i = 0; i < symbol.polylines.size(); ++i) {
    const SymbolPolyline& polyline = symbol.polylines.at(i);
    out << pad << "    {\n";
    out << pad << "      \"points\": [\n";
    for (std::size_t j = 0; j < polyline.points.size(); ++j) {
      const Point& point = polyline.points.at(j);
      out << pad << "        {\"x\": " << point.x.nanometers << ", \"y\": "
          << point.y.nanometers << "}" << (j + 1 == polyline.points.size() ? "" : ",")
          << '\n';
    }
    out << pad << "      ],\n";
    out << pad << "      \"stroke_width\": " << polyline.stroke_width.nanometers << ",\n";
    writeField(out, indent + 6, "fill_type", polyline.fill_type, false);
    out << pad << "    }" << (i + 1 == symbol.polylines.size() ? "" : ",") << '\n';
  }
  out << pad << "  ],\n";

  out << pad << "  \"texts\": [\n";
  for (std::size_t i = 0; i < symbol.texts.size(); ++i) {
    const SymbolText& text = symbol.texts.at(i);
    out << pad << "    {\n";
    writeField(out, indent + 6, "text", text.text);
    out << pad << "      \"x_nm\": " << text.position.x.nanometers << ",\n";
    out << pad << "      \"y_nm\": " << text.position.y.nanometers << ",\n";
    out << pad << "      \"rotation_degrees\": " << text.rotation_degrees << ",\n";
    out << pad << "      \"size\": " << text.size.nanometers << '\n';
    out << pad << "    }" << (i + 1 == symbol.texts.size() ? "" : ",") << '\n';
  }
  out << pad << "  ]\n";
  out << pad << "}";
}

class JsonReader {
 public:
  explicit JsonReader(std::string_view source) : source_(source) {}

  // Helper: ensure at least one schematic exists (legacy compat)
  static Schematic& ensureSchematic(Project& project) {
    if (project.schematics.empty()) {
      project.schematics.emplace_back();
    }
    return project.schematics[0];
  }

  Project readProject() {
    Project project;
    expect('{');
    if (consume('}')) {
      return project;
    }
    while (true) {
      const std::string key = readString();
      expect(':');
      if (key == "schema_version") {
        project.schema_version = readInt();
      } else if (key == "id") {
        project.id = readString();
      } else if (key == "name") {
        project.name = readString();
      } else if (key == "text_variables") {
        project.text_variables = readStringMap();
      } else if (key == "board") {
        project.boards.clear();
        project.boards.push_back(readBoard());
      } else if (key == "components" || key == "symbols") {
        ensureSchematic(project).symbols = readComponents();
      } else if (key == "nets") {
        ensureSchematic(project).nets = readNets();
      } else if (key == "wires") {
        ensureSchematic(project).wires = readWireSegments();
      } else if (key == "buses") {
        ensureSchematic(project).buses = readBusSegments();
      } else if (key == "labels") {
        ensureSchematic(project).labels = readLabels();
      } else if (key == "power_symbols") {
        ensureSchematic(project).power_symbols = readPowerSymbols();
      } else if (key == "constraints") {
        ensureSchematic(project).constraints = readConstraints();
      } else {
        throw std::runtime_error("unknown project key: " + key);
      }
      if (consume('}')) {
        return project;
      }
      expect(',');
      if (peek('}')) {
        throw std::runtime_error("trailing comma in project object");
      }
    }
  }

  void finish() {
    skipWhitespace();
    if (pos_ != source_.size()) {
      throw std::runtime_error("trailing content after root object");
    }
  }

 private:
  std::map<std::string, std::string> readStringMap() {
    std::map<std::string, std::string> values;
    expect('{');
    if (consume('}')) {
      return values;
    }
    while (true) {
      const std::string key = readString();
      expect(':');
      values[key] = readString();
      if (consume('}')) {
        return values;
      }
      expect(',');
      if (peek('}')) {
        throw std::runtime_error("trailing comma in string map");
      }
    }
  }

  Board readBoard() {
    Board board;
    expect('{');
    if (!consume('}')) {
      while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "outline") {
          board.outline = readRect();
        } else if (key == "design_rules") {
          board.design_rules = readDesignRules();
        } else if (key == "layers") {
          board.layers = readLayers();
        } else if (key == "footprints") {
          board.footprints = readBoardFootprints();
        } else if (key == "placement_regions") {
          board.placement_regions = readPlacementRegions();
        } else if (key == "keepouts") {
          board.keepouts = readKeepouts();
        } else if (key == "pads") {
          board.pads = readPads();
        } else if (key == "vias") {
          board.vias = readVias();
        } else if (key == "tracks") {
          board.tracks = readTracks();
        } else if (key == "track_arcs") {
          board.track_arcs = readTrackArcs();
        } else if (key == "graphics") {
          board.graphics = readBoardGraphics();
        } else if (key == "texts") {
          board.texts = readBoardTexts();
        } else if (key == "dimensions") {
          board.dimensions = readBoardDimensions();
        } else if (key == "barcodes") {
          board.barcodes = readBoardBarcodes();
        } else if (key == "targets") {
          board.targets = readBoardTargets();
        } else if (key == "zones") {
          board.zones = readBoardZones();
        } else if (key == "groups") {
          board.groups = readBoardGroups();
        } else if (key == "reference_images") {
          board.reference_images = readBoardReferenceImages();
        } else if (key == "tables") {
          board.tables = readBoardTables();
        } else if (key == "route_requests") {
          board.route_requests = readRouteRequests();
        } else {
          throw std::runtime_error("unknown board key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in board object");
        }
      }
    }
    return board;
  }

  Rect readRect() {
    Rect rect;
    expect('{');
    if (!consume('}')) {
      while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "x_nm") {
          rect.origin.x = nanometers(readInt64());
        } else if (key == "y_nm") {
          rect.origin.y = nanometers(readInt64());
        } else if (key == "width_nm") {
          rect.size.width = nanometers(readInt64());
        } else if (key == "height_nm") {
          rect.size.height = nanometers(readInt64());
        } else {
          throw std::runtime_error("unknown rect key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in rect object");
        }
      }
    }
    return rect;
  }

  Point readPoint() {
    Point point;
    expect('{');
    if (!consume('}')) {
      while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "x_nm") {
          point.x = nanometers(readInt64());
        } else if (key == "y_nm") {
          point.y = nanometers(readInt64());
        } else {
          throw std::runtime_error("unknown point key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in point object");
        }
      }
    }
    return point;
  }

  Size readSize() {
    Size size;
    expect('{');
    if (!consume('}')) {
      while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "width_nm") {
          size.width = nanometers(readInt64());
        } else if (key == "height_nm") {
          size.height = nanometers(readInt64());
        } else {
          throw std::runtime_error("unknown size key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in size object");
        }
      }
    }
    return size;
  }

  DesignRules readDesignRules() {
    DesignRules rules;
    expect('{');
    if (!consume('}')) {
      while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "copper_clearance_nm") {
          rules.copper_clearance = nanometers(readInt64());
        } else if (key == "min_track_width_nm") {
          rules.min_track_width = nanometers(readInt64());
        } else if (key == "min_via_annular_ring_nm") {
          rules.min_via_annular_ring = nanometers(readInt64());
        } else if (key == "min_connection_nm") {
          rules.min_connection = nanometers(readInt64());
        } else if (key == "min_via_diameter_nm") {
          rules.min_via_diameter = nanometers(readInt64());
        } else if (key == "min_through_hole_drill_nm") {
          rules.min_through_hole_drill = nanometers(readInt64());
        } else if (key == "min_microvia_diameter_nm") {
          rules.min_microvia_diameter = nanometers(readInt64());
        } else if (key == "min_microvia_drill_nm") {
          rules.min_microvia_drill = nanometers(readInt64());
        } else if (key == "min_hole_to_hole_nm") {
          rules.min_hole_to_hole = nanometers(readInt64());
        } else if (key == "hole_clearance_nm") {
          rules.hole_clearance = nanometers(readInt64());
        } else if (key == "copper_edge_clearance_nm") {
          rules.copper_edge_clearance = nanometers(readInt64());
        } else if (key == "silk_clearance_nm") {
          rules.silk_clearance = nanometers(readInt64());
        } else if (key == "min_groove_width_nm") {
          rules.min_groove_width = nanometers(readInt64());
        } else if (key == "solder_mask_expansion_nm") {
          rules.solder_mask_expansion = nanometers(readInt64());
        } else if (key == "solder_mask_min_width_nm") {
          rules.solder_mask_min_width = nanometers(readInt64());
        } else if (key == "solder_mask_to_copper_clearance_nm") {
          rules.solder_mask_to_copper_clearance = nanometers(readInt64());
        } else if (key == "solder_paste_margin_nm") {
          rules.solder_paste_margin = nanometers(readInt64());
        } else if (key == "solder_paste_margin_ratio") {
          rules.solder_paste_margin_ratio = readDouble();
        } else if (key == "board_thickness_nm") {
          rules.board_thickness = nanometers(readInt64());
        } else if (key == "use_height_for_length_calcs") {
          rules.use_height_for_length_calcs = readBool();
        } else if (key == "tent_vias_front") {
          rules.tent_vias_front = readBool();
        } else if (key == "tent_vias_back") {
          rules.tent_vias_back = readBool();
        } else if (key == "cover_vias_front") {
          rules.cover_vias_front = readBool();
        } else if (key == "cover_vias_back") {
          rules.cover_vias_back = readBool();
        } else if (key == "plug_vias_front") {
          rules.plug_vias_front = readBool();
        } else if (key == "plug_vias_back") {
          rules.plug_vias_back = readBool();
        } else if (key == "cap_vias") {
          rules.cap_vias = readBool();
        } else if (key == "fill_vias") {
          rules.fill_vias = readBool();
        } else {
          throw std::runtime_error("unknown design rules key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in design rules object");
        }
      }
    }
    return rules;
  }

  std::vector<Layer> readLayers() {
    std::vector<Layer> layers;
    expect('[');
    if (consume(']')) {
      return layers;
    }
    while (true) {
      Layer layer;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            layer.id = readString();
          } else if (key == "name") {
            layer.name = readString();
          } else if (key == "type" || key == "kind") {
            layer.kind = readString();
          } else if (key == "visible") {
            layer.visible = readBool();
          } else {
            throw std::runtime_error("unknown layer key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in layer object");
          }
        }
      }
      layers.push_back(layer);
      if (consume(']')) {
        return layers;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in layers array");
      }
    }
  }

  std::vector<BoardFootprint> readBoardFootprints() {
    std::vector<BoardFootprint> footprints;
    expect('[');
    if (consume(']')) {
      return footprints;
    }
    while (true) {
      BoardFootprint footprint;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "reference") {
            footprint.reference = readString();
          } else if (key == "value") {
            footprint.value = readString();
          } else if (key == "footprint_name") {
            footprint.footprint_name = readString();
          } else if (key == "layer_id") {
            footprint.layer_id = readString();
          } else if (key == "position") {
            footprint.position = readPoint();
          } else if (key == "rotation_degrees") {
            footprint.rotation_degrees = readDouble();
          } else if (key == "exclude_from_bom") {
            footprint.exclude_from_bom = readBool();
          } else if (key == "locked") {
            footprint.locked = readBool();
          } else {
            throw std::runtime_error("unknown board footprint key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in board footprint object");
          }
        }
      }
      footprints.push_back(footprint);
      if (consume(']')) {
        return footprints;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in board footprints array");
      }
    }
  }


  Padstack readPadstack() {
    Padstack ps;
    expect('{');
    if (!consume('}')) {
      while (true) {
        std::string key = readString();
        expect(':');
        if (key == "layer_set") {
          expect('[');
          if (!consume(']')) {
            while (true) {
              ps.layer_set.push_back(readString());
              if (consume(']')) break;
              expect(',');
            }
          }
        } else if (key == "copper_props") {
          expect('{');
          if (!consume('}')) {
            while (true) {
              std::string layer_name = readString();
              expect(':');
              expect('{');
              PadstackCopperLayerProps cp;
              if (!consume('}')) {
                while (true) {
                  std::string cp_key = readString();
                  expect(':');
                  if (cp_key == "shape") {
                    expect('{');
                    if (!consume('}')) {
                      while (true) {
                        std::string sp_key = readString();
                        expect(':');
                        if (sp_key == "shape") {
                          std::string s = readString();
                          if (s == "rect") cp.shape.shape = PadShape::Rectangle;
                          else if (s == "oval") cp.shape.shape = PadShape::Oval;
                          else if (s == "trapezoid") cp.shape.shape = PadShape::Trapezoid;
                          else if (s == "roundrect") cp.shape.shape = PadShape::RoundRect;
                          else if (s == "chamfered_rect") cp.shape.shape = PadShape::ChamferedRect;
                          else if (s == "custom") cp.shape.shape = PadShape::Custom;
                          else cp.shape.shape = PadShape::Circle;
                        } else if (sp_key == "size") {
                          cp.shape.size = readSize();
                        } else if (sp_key == "roundrect_rratio") {
                          cp.shape.roundrect_rratio = readDouble();
                        } else if (sp_key == "chamfer_ratio") {
                          cp.shape.chamfer_ratio = readDouble();
                        } else {
                          readRawJsonObject();
                        }
                        if (consume('}')) break;
                        expect(',');
                      }
                    }
                  } else {
                    readRawJsonObject();
                  }
                  if (consume('}')) break;
                  expect(',');
                }
              }
              ps.copper_props[layer_name] = cp;
              if (consume('}')) break;
              expect(',');
            }
          }
        } else if (key == "drill") {
          expect('{');
          if (!consume('}')) {
            while (true) {
              std::string dr_key = readString();
              expect(':');
              if (dr_key == "size") {
                ps.drill.size = readSize();
              } else {
                readRawJsonObject();
              }
              if (consume('}')) break;
              expect(',');
            }
          }
        } else {
          readRawJsonObject();
        }
        if (consume('}')) break;
        expect(',');
      }
    }
    return ps;
  }

  std::vector<Pad> readPads() {
    std::vector<Pad> pads;
    expect('[');
    if (consume(']')) {
      return pads;
    }
    while (true) {
      Pad pad;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            pad.id = readString();
          } else if (key == "component_id") {
            pad.component_id = readString();
          } else if (key == "pin_name") {
            pad.pin_name = readString();
          } else if (key == "net_id") {
            pad.net_id = readString();
          } else if (key == "type") {
            pad.type = readString();
          } else if (key == "position") {
            pad.position = readPoint();
          } else if (key == "rotation_degrees") {
            pad.rotation_degrees = readDouble();
          } else if (key == "pin_type") {
            pad.pin_type = readString();
          } else if (key == "pad_to_die_length_nm") {
            pad.pad_to_die_length = nanometers(readInt64());
          } else if (key == "pad_to_die_delay") {
            pad.pad_to_die_delay = readDouble();
          } else if (key == "teardrops_enabled") {
            pad.teardrops_enabled = readBool();
          } else if (key == "locked") {
            pad.locked = readBool();
          } else if (key == "padstack") {
             pad.padstack = readPadstack();
          } else if (key == "layers") {
            pad.padstack.layer_set.clear();
            expect('[');
            if (!consume(']')) {
              while (true) {
                pad.padstack.layer_set.push_back(readString());
                if (consume(']')) break;
                expect(',');
              }
            }
          } else if (key == "shape") {
            readString();
          } else if (key == "size") {
            readSize();
          } else if (key == "drill") {
            readDouble();
          } else if (key == "drill_shape") {
            readString();
          } else if (key == "drill_size") {
            readSize();
          } else {
            throw std::runtime_error("unknown pad key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in pad object");
          }
        }
      }
      pads.push_back(pad);
      if (consume(']')) {
        return pads;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in pads array");
      }
    }
  }

  std::vector<Keepout> readKeepouts() {
    std::vector<Keepout> keepouts;
    expect('[');
    if (consume(']')) {
      return keepouts;
    }
    while (true) {
      Keepout keepout;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            keepout.id = readString();
          } else if (key == "type" || key == "kind") {
            keepout.kind = readString();
          } else if (key == "area") {
            keepout.area = readRect();
          } else {
            throw std::runtime_error("unknown keepout key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in keepout object");
          }
        }
      }
      keepouts.push_back(keepout);
      if (consume(']')) {
        return keepouts;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in keepouts array");
      }
    }
  }

  std::vector<PlacementRegion> readPlacementRegions() {
    std::vector<PlacementRegion> regions;
    expect('[');
    if (consume(']')) {
      return regions;
    }
    while (true) {
      PlacementRegion region;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            region.id = readString();
          } else if (key == "type" || key == "kind") {
            region.kind = readString();
          } else if (key == "area") {
            region.area = readRect();
          } else {
            throw std::runtime_error("unknown placement region key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in placement region object");
          }
        }
      }
      regions.push_back(region);
      if (consume(']')) {
        return regions;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in placement regions array");
      }
    }
  }

  std::vector<Via> readVias() {
    std::vector<Via> vias;
    expect('[');
    if (consume(']')) {
      return vias;
    }
    while (true) {
      Via via;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            via.id = readString();
          } else if (key == "net_id") {
            via.net_id = readString();
          } else if (key == "position") {
            via.position = readPoint();
          } else if (key == "diameter_nm") {
            via.diameter = nanometers(readInt64());
          } else if (key == "drill_nm") {
            via.drill = nanometers(readInt64());
          } else if (key == "locked") {
            via.locked = readBool();
          } else {
            throw std::runtime_error("unknown via key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in via object");
          }
        }
      }
      vias.push_back(via);
      if (consume(']')) {
        return vias;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in vias array");
      }
    }
  }

  std::vector<TrackSegment> readTracks() {
    std::vector<TrackSegment> tracks;
    expect('[');
    if (consume(']')) {
      return tracks;
    }
    while (true) {
      TrackSegment track;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            track.id = readString();
          } else if (key == "net_id") {
            track.net_id = readString();
          } else if (key == "layer_id") {
            track.layer_id = readString();
          } else if (key == "start") {
            track.start = readPoint();
          } else if (key == "end") {
            track.end = readPoint();
          } else if (key == "width_nm") {
            track.width = nanometers(readInt64());
          } else if (key == "source_route_request_id") {
            track.source_route_request_id = readString();
          } else if (key == "locked") {
            track.locked = readBool();
          } else {
            throw std::runtime_error("unknown track key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in track object");
          }
        }
      }
      tracks.push_back(track);
      if (consume(']')) {
        return tracks;
      }
      expect(',');
      if (track.locked) {
        throw std::runtime_error("trailing comma in tracks array");
      }
    }
  }

  std::vector<TrackArc> readTrackArcs() {
    std::vector<TrackArc> arcs;
    expect('[');
    if (consume(']')) {
      return arcs;
    }
    while (true) {
      TrackArc arc;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            arc.id = readString();
          } else if (key == "net_id") {
            arc.net_id = readString();
          } else if (key == "layer_id") {
            arc.layer_id = readString();
          } else if (key == "start") {
            arc.start = readPoint();
          } else if (key == "mid") {
            arc.mid = readPoint();
          } else if (key == "end") {
            arc.end = readPoint();
          } else if (key == "width_nm") {
            arc.width = nanometers(readInt64());
          } else if (key == "locked") {
            arc.locked = readBool();
          } else {
            throw std::runtime_error("unknown track arc key: " + key);
          }
          if (consume('}')) break;
          expect(',');
        }
      }
      arcs.push_back(std::move(arc));
      if (consume(']')) break;
      expect(',');
    }
    return arcs;
  }

  std::vector<BoardGraphic> readBoardGraphics() {
    std::vector<BoardGraphic> graphics;
    expect('[');
    if (consume(']')) {
      return graphics;
    }
    while (true) {
      BoardGraphic graphic;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            graphic.id = readString();
          } else if (key == "type" || key == "kind") {
            graphic.kind = readString();
          } else if (key == "layer_id") {
            graphic.layer_id = readString();
          } else if (key == "start") {
            graphic.start = readPoint();
          } else if (key == "mid") {
            graphic.mid = readPoint();
          } else if (key == "end") {
            graphic.end = readPoint();
          } else if (key == "angle_degrees") {
            graphic.angle_degrees = readDouble();
          } else if (key == "width_nm") {
            graphic.width = nanometers(readInt64());
          } else if (key == "locked") {
            graphic.locked = readBool();
          } else {
            throw std::runtime_error("unknown board graphic key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in board graphic object");
          }
        }
      }
      graphics.push_back(graphic);
      if (consume(']')) {
        return graphics;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in board graphics array");
      }
    }
  }

  std::vector<BoardText> readBoardTexts() {
    std::vector<BoardText> texts;
    expect('[');
    if (consume(']')) {
      return texts;
    }
    while (true) {
      BoardText text;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            text.id = readString();
          } else if (key == "layer_id") {
            text.layer_id = readString();
          } else if (key == "text") {
            text.text = readString();
          } else if (key == "position") {
            text.position = readPoint();
          } else if (key == "rotation_degrees") {
            text.rotation_degrees = readDouble();
          } else if (key == "size") {
            text.size = readSize();
          } else if (key == "locked") {
            text.locked = readBool();
          } else {
            throw std::runtime_error("unknown board text key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in board text object");
          }
        }
      }
      texts.push_back(text);
      if (consume(']')) {
        return texts;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in board texts array");
      }
    }
  }

  std::vector<BoardDimension> readBoardDimensions() {
    std::vector<BoardDimension> dimensions;
    expect('[');
    if (consume(']')) {
      return dimensions;
    }
    while (true) {
      BoardDimension dim;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            dim.id = readString();
          } else if (key == "layer_id") {
            dim.layer_id = readString();
          } else if (key == "type" || key == "kind") {
            dim.kind = readString();
          } else if (key == "text") {
            dim.text = readString();
          } else if (key == "start") {
            dim.start = readPoint();
          } else if (key == "end") {
            dim.end = readPoint();
          } else if (key == "text_position") {
            dim.text_position = readPoint();
          } else if (key == "locked") {
            dim.locked = readBool();
          } else {
            throw std::runtime_error("unknown board dimension key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in board dimension object");
          }
        }
      }
      dimensions.push_back(dim);
      if (consume(']')) {
        return dimensions;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in board dimensions array");
      }
    }
  }

  std::vector<BoardGroup> readBoardGroups() {
    std::vector<BoardGroup> groups;
    expect('[');
    if (consume(']')) {
      return groups;
    }
    while (true) {
      BoardGroup group;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            group.id = readString();
          } else if (key == "name") {
            group.name = readString();
          } else if (key == "members") {
            expect('[');
            if (!consume(']')) {
              while (true) {
                group.members.push_back(readString());
                if (consume(']')) {
                  break;
                }
                expect(',');
                if (peek(']')) {
                  throw std::runtime_error("trailing comma in board group members array");
                }
              }
            }
          } else {
            throw std::runtime_error("unknown board group key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in board group object");
          }
        }
      }
      groups.push_back(group);
      if (consume(']')) {
        return groups;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in board groups array");
      }
    }
  }

  std::vector<BoardTarget> readBoardTargets() {
    std::vector<BoardTarget> targets;
    expect('[');
    if (consume(']')) {
      return targets;
    }
    while (true) {
      BoardTarget target;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            target.id = readString();
          } else if (key == "layer_id") {
            target.layer_id = readString();
          } else if (key == "shape") {
            const std::string kind_str = readString();
            if (kind_str == "Plus") {
              target.shape = TargetShape::Plus;
            } else if (kind_str == "X") {
              target.shape = TargetShape::X;
            }
          } else if (key == "x_nm") {
            target.position_x = nanometers(readInt64());
          } else if (key == "y_nm") {
            target.position_y = nanometers(readInt64());
          } else if (key == "size_nm") {
            target.size = nanometers(readInt64());
          } else if (key == "line_width_nm") {
            target.line_width = nanometers(readInt64());
          } else {
            throw std::runtime_error("unknown target key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
        }
      }
      targets.push_back(std::move(target));
      if (consume(']')) {
        break;
      }
      expect(',');
    }
    return targets;
  }

  std::vector<BoardReferenceImage> readBoardReferenceImages() {
    std::vector<BoardReferenceImage> results;
    expect('[');
    if (consume(']')) {
      return results;
    }
    while (true) {
      BoardReferenceImage obj;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            obj.id = readString();
          } else if (key == "layer") {
            obj.layer = readString();
          } else if (key == "data") {
            obj.data = readString();
          } else if (key == "x_mm") {
            obj.x_mm = readDouble();
          } else if (key == "y_mm") {
            obj.y_mm = readDouble();
          } else if (key == "scale") {
            obj.scale = readDouble();
          } else if (key == "opacity") {
            obj.opacity = readDouble();
          } else {
            throw std::runtime_error("unknown reference_image key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
        }
      }
      results.push_back(obj);
      if (consume(']')) {
        break;
      }
      expect(',');
    }
    return results;
  }

  std::vector<BoardTable> readBoardTables() {
    std::vector<BoardTable> results;
    expect('[');
    if (consume(']')) {
      return results;
    }
    while (true) {
      BoardTable obj;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            obj.id = readString();
          } else if (key == "layer") {
            obj.layer = readString();
          } else if (key == "x_mm") {
            obj.x_mm = readDouble();
          } else if (key == "y_mm") {
            obj.y_mm = readDouble();
          } else if (key == "rows") {
            obj.rows = (int)readDouble();
          } else if (key == "cols") {
            obj.cols = (int)readDouble();
          } else if (key == "width_mm") {
            obj.width_mm = readDouble();
          } else if (key == "height_mm") {
            obj.height_mm = readDouble();
          } else if (key == "cells") {
            expect('[');
            if (!consume(']')) {
              while (true) {
                BoardTableCell cell;
                expect('{');
                if (!consume('}')) {
                  while (true) {
                    const std::string ckey = readString();
                    expect(':');
                    if (ckey == "row") {
                      cell.row = (int)readDouble();
                    } else if (ckey == "col") {
                      cell.col = (int)readDouble();
                    } else if (ckey == "text") {
                      cell.text = readString();
                    } else {
                      throw std::runtime_error("unknown cell key: " + ckey);
                    }
                    if (consume('}')) {
                      break;
                    }
                    expect(',');
                  }
                }
                obj.cells.push_back(cell);
                if (consume(']')) {
                  break;
                }
                expect(',');
              }
            }
          } else {
            throw std::runtime_error("unknown table key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
        }
      }
      results.push_back(obj);
      if (consume(']')) {
        break;
      }
      expect(',');
    }
    return results;
  }

  std::vector<BoardBarcode> readBoardBarcodes() {
    std::vector<BoardBarcode> barcodes;
    expect('[');
    if (consume(']')) {
      return barcodes;
    }
    while (true) {
      BoardBarcode barcode;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            barcode.id = readString();
          } else if (key == "layer_id") {
            barcode.layer_id = readString();
          } else if (key == "text") {
            barcode.text = readString();
          } else if (key == "type" || key == "kind") {
            barcode.kind = parseBarcodeType(readString());
          } else if (key == "error_correction") {
            barcode.error_correction = parseBarcodeEcc(readString());
          } else if (key == "position") {
            barcode.position = readPoint();
          } else if (key == "rotation_degrees") {
            barcode.rotation_degrees = readDouble();
          } else if (key == "size") {
            barcode.size = readSize();
          } else if (key == "margin") {
            barcode.margin = readSize();
          } else if (key == "locked") {
            barcode.locked = readBool();
          } else {
            throw std::runtime_error("unknown barcode key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
        }
      }
      barcodes.push_back(barcode);
      if (consume(']')) {
        break;
      }
      expect(',');
    }
    return barcodes;
  }

  std::vector<BoardZone> readBoardZones() {
    std::vector<BoardZone> zones;
    expect('[');
    if (consume(']')) {
      return zones;
    }
    while (true) {
      BoardZone zone;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            zone.id = readString();
          } else if (key == "name") {
            zone.name = readString();
          } else if (key == "net_id") {
            zone.net_id = readString();
          } else if (key == "layer_ids") {
            zone.layer_ids = readStringArray();
          } else if (key == "outline") {
            zone.outline = readPointArray();
          } else if (key == "priority") {
            zone.priority = readInt();
          } else if (key == "clearance_nm") {
            zone.clearance = nanometers(readInt64());
          } else if (key == "min_thickness_nm") {
            zone.min_thickness = nanometers(readInt64());
          } else if (key == "fill_enabled") {
            zone.fill_enabled = readBool();
          } else if (key == "pad_connection") {
            zone.pad_connection = readString();
          } else if (key == "locked") {
            zone.locked = readBool();
          } else {
            throw std::runtime_error("unknown board zone key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in board zone object");
          }
        }
      }
      zones.push_back(zone);
      if (consume(']')) {
        return zones;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in board zones array");
      }
    }
  }

  std::vector<RouteRequest> readRouteRequests() {
    std::vector<RouteRequest> route_requests;
    expect('[');
    if (consume(']')) {
      return route_requests;
    }
    while (true) {
      RouteRequest route_request;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            route_request.id = readString();
          } else if (key == "net_id") {
            route_request.net_id = readString();
          } else if (key == "from_object_id") {
            route_request.from_object_id = readString();
          } else if (key == "to_object_id") {
            route_request.to_object_id = readString();
          } else if (key == "preferred_layer_id") {
            route_request.preferred_layer_id = readString();
          } else if (key == "policy") {
            route_request.policy = readString();
          } else if (key == "width_nm") {
            route_request.width = nanometers(readInt64());
          } else {
            throw std::runtime_error("unknown route request key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in route request object");
          }
        }
      }
      route_requests.push_back(route_request);
      if (consume(']')) {
        return route_requests;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in route requests array");
      }
    }
  }

  std::vector<SchSymbol> readComponents() {
    std::vector<SchSymbol> symbols;
    expect('[');
    if (consume(']')) {
      return symbols;
    }
    while (true) {
      SchSymbol component;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          component.id = readString();
        } else if (key == "lib_id" || key == "part") {
          component.lib_id = readString();
        } else if (key == "position") {
          component.position = readPoint();
        } else if (key == "rotation_degrees") {
          component.rotation_degrees = readDouble();
        } else if (key == "pins") {
          component.pins = readPins();
        } else if (key == "symbol") {
          component.symbol = loadSymbolJson(readRawJsonObject());
        } else {
          throw std::runtime_error("unknown component key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in component object");
        }
        }
      }
      symbols.push_back(component);
      if (consume(']')) {
        return symbols;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in components array");
      }
    }
  }

  std::vector<SchPin> readPins() {
    std::vector<SchPin> pins;
    expect('[');
    if (consume(']')) {
      return pins;
    }
    while (true) {
      SchPin pin;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "name") {
          pin.name = readString();
        } else if (key == "number") {
          pin.number = readString();
        } else if (key == "type" || key == "kind" || key == "electrical_type") {
          pin.electrical_type = parse_electrical_pin_type(readString());
        } else if (key == "graphical_style" || key == "shape") {
          pin.shape = parse_graphic_pin_shape(readString());
        } else if (key == "orientation") {
          pin.orientation = parse_pin_orientation(readString());
        } else {
          throw std::runtime_error("unknown pin key (1584): '" + key + "'");
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in pin object");
        }
        }
      }
      pins.push_back(pin);
      if (consume(']')) {
        return pins;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in pins array");
      }
    }
  }

  std::vector<SchWire> readWireSegments() {
    std::vector<SchWire> wires;
    expect('[');
    if (consume(']')) {
      return wires;
    }
    while (true) {
      SchWire wire;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            wire.id = readString();
          } else if (key == "start") {
            wire.start = readPoint();
          } else if (key == "end") {
            wire.end = readPoint();
          } else if (key == "net_id") {
            wire.net_id = readString();
          } else {
            throw std::runtime_error("unknown wire segment key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in wire segment object");
          }
        }
      }
      wires.push_back(wire);
      if (consume(']')) {
        return wires;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in wires array");
      }
    }
  }

  std::vector<SchBus> readBusSegments() {
    std::vector<SchBus> buses;
    expect('[');
    if (consume(']')) {
      return buses;
    }
    while (true) {
      SchBus bus;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") {
            bus.id = readString();
          } else if (key == "start") {
            bus.start = readPoint();
          } else if (key == "end") {
            bus.end = readPoint();
          } else if (key == "bus_id") {
            bus.bus_id = readString();
          } else if (key == "net_ids") {
            bus.net_ids = readStringArray();
          } else {
            throw std::runtime_error("unknown bus segment key: " + key);
          }
          if (consume('}')) {
            break;
          }
          expect(',');
          if (peek('}')) {
            throw std::runtime_error("trailing comma in bus segment object");
          }
        }
      }
      buses.push_back(bus);
      if (consume(']')) {
        return buses;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in buses array");
      }
    }
  }

  std::vector<SchLabel> readLabels() {
    std::vector<SchLabel> labels;
    expect('[');
    if (consume(']')) return labels;
    while (true) {
      SchLabel label;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") label.id = readString();
          else if (key == "text") label.text = readString();
          else if (key == "net_id") label.net_id = readString();
          else if (key == "position") label.position = readPoint();
          else if (key == "rotation_degrees") label.rotation_degrees = readDouble();
          else if (key == "global") label.type = readBool() ? LabelType::Global : LabelType::Local;
          else throw std::runtime_error("unknown label key: " + key);
          if (consume('}')) break;
          expect(',');
          if (peek('}')) throw std::runtime_error("trailing comma in label object");
        }
      }
      labels.push_back(label);
      if (consume(']')) return labels;
      expect(',');
      if (peek(']')) throw std::runtime_error("trailing comma in labels array");
    }
  }

  std::vector<SchPowerSymbol> readPowerSymbols() {
    std::vector<SchPowerSymbol> power_symbols;
    expect('[');
    if (consume(']')) return power_symbols;
    while (true) {
      SchPowerSymbol symbol;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "id") symbol.id = readString();
          else if (key == "value") symbol.value = readString();
          else if (key == "net_id") symbol.net_id = readString();
          else if (key == "position") symbol.position = readPoint();
          else if (key == "rotation_degrees") symbol.rotation_degrees = readDouble();
          else throw std::runtime_error("unknown power symbol key: " + key);
          if (consume('}')) break;
          expect(',');
          if (peek('}')) throw std::runtime_error("trailing comma in power symbol object");
        }
      }
      power_symbols.push_back(symbol);
      if (consume(']')) return power_symbols;
      expect(',');
      if (peek(']')) throw std::runtime_error("trailing comma in power symbols array");
    }
  }

  std::vector<Net> readNets() {
    std::vector<Net> nets;
    expect('[');
    if (consume(']')) {
      return nets;
    }
    while (true) {
      Net net;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          net.id = readString();
        } else if (key == "members") {
          net.members = readMembers();
        } else {
          throw std::runtime_error("unknown net key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in net object");
        }
        }
      }
      nets.push_back(net);
      if (consume(']')) {
        return nets;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in nets array");
      }
    }
  }

  std::vector<NetMember> readMembers() {
    std::vector<NetMember> members;
    expect('[');
    if (consume(']')) {
      return members;
    }
    while (true) {
      NetMember member;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "component_id") {
          member.component_id = readString();
        } else if (key == "pin_name") {
          member.pin_name = readString();
        } else {
          throw std::runtime_error("unknown net member key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in net member object");
        }
        }
      }
      members.push_back(member);
      if (consume(']')) {
        return members;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in members array");
      }
    }
  }

  std::vector<Constraint> readConstraints() {
    std::vector<Constraint> constraints;
    expect('[');
    if (consume(']')) {
      return constraints;
    }
    while (true) {
      Constraint constraint;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          constraint.id = readString();
        } else if (key == "type" || key == "kind") {
          constraint.kind = readString();
        } else if (key == "target") {
          constraint.target = readString();
        } else if (key == "value") {
          constraint.value = readString();
        } else {
          throw std::runtime_error("unknown constraint key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in constraint object");
        }
        }
      }
      constraints.push_back(constraint);
      if (consume(']')) {
        return constraints;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in constraints array");
      }
    }
  }

  int readInt() {
    const std::int64_t value = readInt64();
    if (value > 2147483647) {
      throw std::runtime_error("integer too large");
    }
    return static_cast<int>(value);
  }

  std::int64_t readInt64() {
    skipWhitespace();
    std::int64_t value = 0;
    bool found = false;
    bool negative = false;
    if (pos_ < source_.size() && source_[pos_] == '-') {
      negative = true;
      ++pos_;
    }
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) {
      found = true;
      value = (value * 10) + (source_[pos_] - '0');
      ++pos_;
    }
    if (!found) {
      throw std::runtime_error("expected integer");
    }
    return negative ? -value : value;
  }

  double readDouble() {
    skipWhitespace();
    const std::size_t start = pos_;
    if (pos_ < source_.size() && source_[pos_] == '-') {
      ++pos_;
    }
    bool found_digit = false;
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) {
      found_digit = true;
      ++pos_;
    }
    if (pos_ < source_.size() && source_[pos_] == '.') {
      ++pos_;
      while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) {
        found_digit = true;
        ++pos_;
      }
    }
    if (!found_digit) {
      throw std::runtime_error("expected number");
    }
    return std::stod(std::string(source_.substr(start, pos_ - start)));
  }

  bool readBool() {
    skipWhitespace();
    if (source_.substr(pos_, 4) == "true") {
      pos_ += 4;
      return true;
    }
    if (source_.substr(pos_, 5) == "false") {
      pos_ += 5;
      return false;
    }
    throw std::runtime_error("expected boolean");
  }

  std::string readString() {
    skipWhitespace();
    expectRaw('"');
    std::string value;
    while (pos_ < source_.size() && source_[pos_] != '"') {
      if (source_[pos_] == '\\') {
        ++pos_;
        if (pos_ >= source_.size()) {
          throw std::runtime_error("unterminated escape");
        }
        value += readEscape();
        continue;
      }
      if (static_cast<unsigned char>(source_[pos_]) < 0x20) {
        throw std::runtime_error("unescaped control character in string");
      }
      value.push_back(source_[pos_]);
      ++pos_;
    }
    expectRaw('"');
    return value;
  }

  std::vector<std::string> readStringArray() {
    std::vector<std::string> arr;
    expect('[');
    if (consume(']')) {
      return arr;
    }
    while (true) {
      arr.push_back(readString());
      if (consume(']')) {
        return arr;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in string array");
      }
    }
  }

  std::vector<Point> readPointArray() {
    std::vector<Point> points;
    expect('[');
    if (consume(']')) {
      return points;
    }
    while (true) {
      points.push_back(readPoint());
      if (consume(']')) {
        return points;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in point array");
      }
    }
  }

  std::string readRawJsonObject() {
    skipWhitespace();
    if (pos_ >= source_.size() || source_[pos_] != '{') {
      throw std::runtime_error("expected json object");
    }
    const std::size_t start = pos_;
    std::vector<char> stack;
    bool in_string = false;
    bool escaped = false;
    while (pos_ < source_.size()) {
      const char current = source_[pos_++];
      if (in_string) {
        if (escaped) {
          escaped = false;
        } else if (current == '\\') {
          escaped = true;
        } else if (current == '"') {
          in_string = false;
        }
        continue;
      }
      if (current == '"') {
        in_string = true;
        continue;
      }
      if (current == '{') {
        stack.push_back('}');
        continue;
      }
      if (current == '[') {
        stack.push_back(']');
        continue;
      }
      if (current == '}' || current == ']') {
        if (stack.empty() || stack.back() != current) {
          throw std::runtime_error("mismatched json container");
        }
        stack.pop_back();
        if (stack.empty()) {
          return std::string(source_.substr(start, pos_ - start));
        }
      }
    }
    throw std::runtime_error("unterminated json object");
  }

  bool consume(const char expected) {
    skipWhitespace();
    if (pos_ < source_.size() && source_[pos_] == expected) {
      ++pos_;
      return true;
    }
    return false;
  }

  bool peek(const char expected) {
    skipWhitespace();
    return pos_ < source_.size() && source_[pos_] == expected;
  }

  std::string readEscape() {
    const char escaped = source_[pos_++];
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        return std::string(1, escaped);
      case 'b':
        return "\b";
      case 'f':
        return "\f";
      case 'n':
        return "\n";
      case 'r':
        return "\r";
      case 't':
        return "\t";
      case 'u':
        return readUnicodeEscape();
      default:
        throw std::runtime_error("invalid string escape");
    }
  }

  std::string readUnicodeEscape() {
    int codepoint = 0;
    for (int i = 0; i < 4; ++i) {
      if (pos_ >= source_.size() || std::isxdigit(static_cast<unsigned char>(source_[pos_])) == 0) {
        throw std::runtime_error("invalid unicode escape");
      }
      const char ch = source_[pos_++];
      codepoint *= 16;
      if (ch >= '0' && ch <= '9') {
        codepoint += ch - '0';
      } else if (ch >= 'a' && ch <= 'f') {
        codepoint += 10 + ch - 'a';
      } else {
        codepoint += 10 + ch - 'A';
      }
    }

    std::string out;
    if (codepoint <= 0x7F) {
      out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
      out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
      out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return out;
  }

  void expect(const char expected) {
    if (!consume(expected)) {
      throw std::runtime_error(std::string("expected '") + expected + "'");
    }
  }

  void expectRaw(const char expected) {
    if (pos_ >= source_.size() || source_[pos_] != expected) {
      throw std::runtime_error(std::string("expected '") + expected + "'");
    }
    ++pos_;
  }

  void skipWhitespace() {
    while (pos_ < source_.size() &&
           std::isspace(static_cast<unsigned char>(source_[pos_])) != 0) {
      ++pos_;
    }
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

}  // namespace



static void writePadstack(std::ostringstream& out, const int indent, const Padstack& padstack) {
  const std::string pad(indent, ' ');
  out << "{\n";
  
  out << pad << "  \"layer_set\": [";
  for (std::size_t i = 0; i < padstack.layer_set.size(); ++i) {
    out << "\"" << escapeJson(padstack.layer_set[i]) << "\"" << (i + 1 == padstack.layer_set.size() ? "" : ", ");
  }
  out << "],\n";

  out << pad << "  \"copper_props\": {\n";
  std::size_t c = 0;
  for (auto it = padstack.copper_props.begin(); it != padstack.copper_props.end(); ++it, ++c) {
    out << pad << "    \"" << escapeJson(it->first) << "\": {\n";
    out << pad << "      \"shape\": {\n";
    const auto& sp = it->second.shape;
    std::string shape_str = "circle";
    if (sp.shape == PadShape::Rectangle) shape_str = "rect";
    else if (sp.shape == PadShape::Oval) shape_str = "oval";
    else if (sp.shape == PadShape::Trapezoid) shape_str = "trapezoid";
    else if (sp.shape == PadShape::RoundRect) shape_str = "roundrect";
    else if (sp.shape == PadShape::ChamferedRect) shape_str = "chamfered_rect";
    else if (sp.shape == PadShape::Custom) shape_str = "custom";
    out << pad << "        \"shape\": \"" << shape_str << "\",\n";
    out << pad << "        \"size\": ";
    writeSize(out, indent + 8, sp.size);
    out << ",\n";
    out << pad << "        \"roundrect_rratio\": " << sp.roundrect_rratio << ",\n";
    out << pad << "        \"chamfer_ratio\": " << sp.chamfer_ratio << "\n";
    out << pad << "      }\n";
    out << pad << "    }" << (c + 1 == padstack.copper_props.size() ? "" : ",") << "\n";
  }
  out << pad << "  },\n";

  out << pad << "  \"drill\": {\n";
  out << pad << "    \"size\": ";
  writeSize(out, indent + 4, padstack.drill.size);
  out << "\n";
  out << pad << "  }\n";
  out << pad << "}";
} // namespace

std::string formatBarcodeType(BarcodeType type) {
  switch (type) {
    case BarcodeType::Code39: return "Code39";
    case BarcodeType::Code128: return "Code128";
    case BarcodeType::DataMatrix: return "DataMatrix";
    case BarcodeType::QRCode: return "QRCode";
    case BarcodeType::MicroQRCode: return "MicroQRCode";
  }
  return "QRCode";
}

std::string formatBarcodeEcc(BarcodeEcc ecc) {
  switch (ecc) {
    case BarcodeEcc::Low: return "Low";
    case BarcodeEcc::Medium: return "Medium";
    case BarcodeEcc::Quartile: return "Quartile";
    case BarcodeEcc::High: return "High";
  }
  return "Low";
}

BarcodeType parseBarcodeType(const std::string& str) {
  if (str == "Code39") return BarcodeType::Code39;
  if (str == "Code128") return BarcodeType::Code128;
  if (str == "DataMatrix") return BarcodeType::DataMatrix;
  if (str == "QRCode") return BarcodeType::QRCode;
  if (str == "MicroQRCode") return BarcodeType::MicroQRCode;
  throw std::runtime_error("Unknown BarcodeType: " + str);
}

BarcodeEcc parseBarcodeEcc(const std::string& str) {
  if (str == "Low") return BarcodeEcc::Low;
  if (str == "Medium") return BarcodeEcc::Medium;
  if (str == "Quartile") return BarcodeEcc::Quartile;
  if (str == "High") return BarcodeEcc::High;
  throw std::runtime_error("Unknown BarcodeEcc: " + str);
}
std::string dumpProjectJson(const Project& project) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema_version\": " << project.schema_version << ",\n";
  writeField(out, 2, "id", project.id);
  writeField(out, 2, "name", project.name);
  out << "  \"text_variables\": {\n";
  std::size_t text_variable_index = 0;
  for (const auto& [key, value] : project.text_variables) {
    out << "    \"" << escapeJson(key) << "\": \"" << escapeJson(value) << "\""
        << (++text_variable_index == project.text_variables.size() ? "" : ",") << '\n';
  }
  out << "  },\n";

  if (!project.boards.empty()) {
    const Board& board = project.boards[0];
    out << "  \"board\": {\n";
    out << "    \"outline\": {\n";
    out << "      \"x_nm\": " << board.outline.origin.x.nanometers << ",\n";
    out << "      \"y_nm\": " << board.outline.origin.y.nanometers << ",\n";
    out << "      \"width_nm\": " << board.outline.size.width.nanometers << ",\n";
    out << "      \"height_nm\": " << board.outline.size.height.nanometers << "\n";
    out << "    },\n";
    out << "    \"design_rules\": {\n";
    out << "      \"copper_clearance_nm\": "
        << board.design_rules.copper_clearance.nanometers << ",\n";
    out << "      \"min_track_width_nm\": "
        << board.design_rules.min_track_width.nanometers << ",\n";
    out << "      \"min_via_annular_ring_nm\": "
        << board.design_rules.min_via_annular_ring.nanometers << ",\n";
    out << "      \"min_connection_nm\": "
        << board.design_rules.min_connection.nanometers << ",\n";
    out << "      \"min_via_diameter_nm\": "
        << board.design_rules.min_via_diameter.nanometers << ",\n";
    out << "      \"min_through_hole_drill_nm\": "
        << board.design_rules.min_through_hole_drill.nanometers << ",\n";
    out << "      \"min_microvia_diameter_nm\": "
        << board.design_rules.min_microvia_diameter.nanometers << ",\n";
    out << "      \"min_microvia_drill_nm\": "
        << board.design_rules.min_microvia_drill.nanometers << ",\n";
    out << "      \"min_hole_to_hole_nm\": "
        << board.design_rules.min_hole_to_hole.nanometers << ",\n";
    out << "      \"hole_clearance_nm\": "
        << board.design_rules.hole_clearance.nanometers << ",\n";
    out << "      \"copper_edge_clearance_nm\": "
        << board.design_rules.copper_edge_clearance.nanometers << ",\n";
    out << "      \"silk_clearance_nm\": "
        << board.design_rules.silk_clearance.nanometers << ",\n";
    out << "      \"min_groove_width_nm\": "
        << board.design_rules.min_groove_width.nanometers << ",\n";
    out << "      \"solder_mask_expansion_nm\": "
        << board.design_rules.solder_mask_expansion.nanometers << ",\n";
    out << "      \"solder_mask_min_width_nm\": "
        << board.design_rules.solder_mask_min_width.nanometers << ",\n";
    out << "      \"solder_mask_to_copper_clearance_nm\": "
        << board.design_rules.solder_mask_to_copper_clearance.nanometers << ",\n";
    out << "      \"solder_paste_margin_nm\": "
        << board.design_rules.solder_paste_margin.nanometers << ",\n";
    out << "      \"solder_paste_margin_ratio\": "
        << board.design_rules.solder_paste_margin_ratio << ",\n";
    out << "      \"board_thickness_nm\": "
        << board.design_rules.board_thickness.nanometers << ",\n";
    out << "      \"use_height_for_length_calcs\": "
        << (board.design_rules.use_height_for_length_calcs ? "true" : "false") << ",\n";
    out << "      \"tent_vias_front\": "
        << (board.design_rules.tent_vias_front ? "true" : "false") << ",\n";
    out << "      \"tent_vias_back\": "
        << (board.design_rules.tent_vias_back ? "true" : "false") << ",\n";
    out << "      \"cover_vias_front\": "
        << (board.design_rules.cover_vias_front ? "true" : "false") << ",\n";
    out << "      \"cover_vias_back\": "
        << (board.design_rules.cover_vias_back ? "true" : "false") << ",\n";
    out << "      \"plug_vias_front\": "
        << (board.design_rules.plug_vias_front ? "true" : "false") << ",\n";
    out << "      \"plug_vias_back\": "
        << (board.design_rules.plug_vias_back ? "true" : "false") << ",\n";
    out << "      \"cap_vias\": "
        << (board.design_rules.cap_vias ? "true" : "false") << ",\n";
    out << "      \"fill_vias\": "
        << (board.design_rules.fill_vias ? "true" : "false") << "\n";
    out << "    },\n";
    out << "    \"layers\": [\n";
    for (std::size_t i = 0; i < board.layers.size(); ++i) {
      const Layer& layer = board.layers.at(i);
      out << "      {\n";
      writeField(out, 8, "id", layer.id);
      writeField(out, 8, "kind", layer.kind);
      writeField(out, 8, "name", layer.name);
      out << "        \"visible\": " << (layer.visible ? "true" : "false") << '\n';
      out << "      }" << (i + 1 == board.layers.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"footprints\": [\n";
    for (std::size_t i = 0; i < board.footprints.size(); ++i) {
      const BoardFootprint& footprint = board.footprints.at(i);
      out << "      {\n";
      writeField(out, 8, "reference", footprint.reference);
      writeField(out, 8, "value", footprint.value);
      writeField(out, 8, "footprint_name", footprint.footprint_name);
      writeField(out, 8, "layer_id", footprint.layer_id);
      out << "        \"position\": ";
      writePoint(out, 0, footprint.position);
      out << ",\n";
      out << "        \"rotation_degrees\": " << footprint.rotation_degrees << ",\n";
      out << "        \"exclude_from_bom\": "
          << (footprint.exclude_from_bom ? "true" : "false");
      if (footprint.locked) {
        out << ",\n        \"locked\": true";
      }
      out << '\n';
      out << "      }" << (i + 1 == board.footprints.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"placement_regions\": [\n";
    for (std::size_t i = 0; i < board.placement_regions.size(); ++i) {
      const PlacementRegion& region = board.placement_regions.at(i);
      out << "      {\n";
      writeField(out, 8, "id", region.id);
      writeField(out, 8, "kind", region.kind);
      out << "        \"area\": {\n";
      out << "          \"x_nm\": " << region.area.origin.x.nanometers << ",\n";
      out << "          \"y_nm\": " << region.area.origin.y.nanometers << ",\n";
      out << "          \"width_nm\": " << region.area.size.width.nanometers << ",\n";
      out << "          \"height_nm\": " << region.area.size.height.nanometers << "\n";
      out << "        }\n";
      out << "      }" << (i + 1 == board.placement_regions.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"keepouts\": [\n";
    for (std::size_t i = 0; i < board.keepouts.size(); ++i) {
      const Keepout& keepout = board.keepouts.at(i);
      out << "      {\n";
      writeField(out, 8, "id", keepout.id);
      writeField(out, 8, "kind", keepout.kind);
      out << "        \"area\": {\n";
      out << "          \"x_nm\": " << keepout.area.origin.x.nanometers << ",\n";
      out << "          \"y_nm\": " << keepout.area.origin.y.nanometers << ",\n";
      out << "          \"width_nm\": " << keepout.area.size.width.nanometers << ",\n";
      out << "          \"height_nm\": " << keepout.area.size.height.nanometers << "\n";
      out << "        }\n";
      out << "      }" << (i + 1 == board.keepouts.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"pads\": [\n";
    for (std::size_t i = 0; i < board.pads.size(); ++i) {
      const Pad& pad = board.pads.at(i);
      out << "      {\n";
      writeField(out, 8, "id", pad.id);
      writeField(out, 8, "component_id", pad.component_id);
      writeField(out, 8, "pin_name", pad.pin_name);
      writeField(out, 8, "net_id", pad.net_id);
      writeField(out, 8, "type", pad.type);
      out << "        \"position\": ";
      writePoint(out, 0, pad.position);
      out << ",\n";
      out << "        \"rotation_degrees\": " << pad.rotation_degrees;
      if (!pad.pin_type.empty()) {
        out << ",\n        \"pin_type\": \"" << escapeJson(pad.pin_type) << "\"";
      }
      if (pad.pad_to_die_length.has_value()) {
        out << ",\n        \"pad_to_die_length_nm\": " << pad.pad_to_die_length->nanometers;
      }
      if (pad.pad_to_die_delay.has_value()) {
        out << ",\n        \"pad_to_die_delay\": " << *pad.pad_to_die_delay;
      }
      if (pad.teardrops_enabled) {
        out << ",\n        \"teardrops_enabled\": true";
      }
      if (pad.locked) {
        out << ",\n        \"locked\": true";
      }
      out << ",\n        \"padstack\": ";
      writePadstack(out, 8, pad.padstack);
      out << "\n";
      out << "      }" << (i + 1 == board.pads.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"tracks\": [\n";
    for (std::size_t i = 0; i < board.tracks.size(); ++i) {
      const TrackSegment& track = board.tracks.at(i);
      out << "      {\n";
      writeField(out, 8, "id", track.id);
      writeField(out, 8, "net_id", track.net_id);
      writeField(out, 8, "layer_id", track.layer_id);
      out << "        \"start\": ";
      writePoint(out, 0, track.start);
      out << ",\n";
      out << "        \"end\": ";
      writePoint(out, 0, track.end);
      out << ",\n";
      out << "        \"width_nm\": " << track.width.nanometers << ",\n";
      writeField(out, 8, "source_route_request_id", track.source_route_request_id,
                 track.locked);
      if (track.locked) {
        out << "        \"locked\": true\n";
      }
      out << "      }" << (i + 1 == board.tracks.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"track_arcs\": [\n";
    for (std::size_t i = 0; i < board.track_arcs.size(); ++i) {
      const TrackArc& arc = board.track_arcs.at(i);
      out << "      {\n";
      writeField(out, 8, "id", arc.id);
      writeField(out, 8, "net_id", arc.net_id);
      writeField(out, 8, "layer_id", arc.layer_id);
      out << "        \"start\": ";
      writePoint(out, 0, arc.start);
      out << ",\n";
      out << "        \"mid\": ";
      writePoint(out, 0, arc.mid);
      out << ",\n";
      out << "        \"end\": ";
      writePoint(out, 0, arc.end);
      out << ",\n";
      out << "        \"width_nm\": " << arc.width.nanometers;
      if (arc.locked) {
        out << ",\n        \"locked\": true";
      }
      out << "\n      }" << (i + 1 == board.track_arcs.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"graphics\": [\n";
    for (std::size_t i = 0; i < board.graphics.size(); ++i) {
      const BoardGraphic& graphic = board.graphics.at(i);
      out << "      {\n";
      writeField(out, 8, "id", graphic.id);
      writeField(out, 8, "kind", graphic.kind);
      writeField(out, 8, "layer_id", graphic.layer_id);
      out << "        \"start\": ";
      writePoint(out, 0, graphic.start);
      out << ",\n";
      if (graphic.mid.has_value()) {
        out << "        \"mid\": ";
        writePoint(out, 0, *graphic.mid);
        out << ",\n";
      }
      out << "        \"end\": ";
      writePoint(out, 0, graphic.end);
      out << ",\n";
      if (graphic.angle_degrees.has_value()) {
        out << "        \"angle_degrees\": " << *graphic.angle_degrees << ",\n";
      }
      out << "        \"width_nm\": " << graphic.width.nanometers;
      if (graphic.locked) {
        out << ",\n        \"locked\": true";
      }
      out << "\n";
      out << "      }" << (i + 1 == board.graphics.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"texts\": [\n";
    for (std::size_t i = 0; i < board.texts.size(); ++i) {
      const BoardText& text = board.texts.at(i);
      out << "      {\n";
      writeField(out, 8, "id", text.id);
      writeField(out, 8, "layer_id", text.layer_id);
      writeField(out, 8, "text", text.text);
      out << "        \"position\": ";
      writePoint(out, 0, text.position);
      out << ",\n";
      out << "        \"rotation_degrees\": " << text.rotation_degrees << ",\n";
      out << "        \"size\": ";
      writeSize(out, 0, text.size);
      if (text.locked) {
        out << ",\n        \"locked\": true";
      }
      out << "\n";
      out << "      }" << (i + 1 == board.texts.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"dimensions\": [\n";
    for (std::size_t i = 0; i < board.dimensions.size(); ++i) {
      const BoardDimension& dim = board.dimensions.at(i);
      out << "      {\n";
      writeField(out, 8, "id", dim.id);
      writeField(out, 8, "layer_id", dim.layer_id);
      writeField(out, 8, "kind", dim.kind);
      writeField(out, 8, "text", dim.text);
      out << "        \"start\": ";
      writePoint(out, 0, dim.start);
      out << ",\n";
      out << "        \"end\": ";
      writePoint(out, 0, dim.end);
      out << ",\n";
      out << "        \"text_position\": ";
      writePoint(out, 0, dim.text_position);
      if (dim.locked) {
        out << ",\n        \"locked\": true";
      }
      out << "\n";
      out << "      }" << (i + 1 == board.dimensions.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"targets\": [\n";
    for (std::size_t i = 0; i < board.targets.size(); ++i) {
      const BoardTarget& target = board.targets.at(i);
      out << "      {\n";
      writeField(out, 8, "id", target.id);
      writeField(out, 8, "layer_id", target.layer_id);
      writeField(out, 8, "shape", target.shape == TargetShape::Plus ? "Plus" : "X");
      out << "        \"x_nm\": " << target.position_x.nanometers << ",\n";
      out << "        \"y_nm\": " << target.position_y.nanometers << ",\n";
      out << "        \"size_nm\": " << target.size.nanometers << ",\n";
      out << "        \"line_width_nm\": " << target.line_width.nanometers << "\n";
      out << "      }" << (i + 1 == board.targets.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"barcodes\": [\n";
    for (std::size_t i = 0; i < board.barcodes.size(); ++i) {
      const BoardBarcode& barcode = board.barcodes.at(i);
      out << "      {\n";
      writeField(out, 8, "id", barcode.id);
      writeField(out, 8, "layer_id", barcode.layer_id);
      writeField(out, 8, "text", barcode.text);
      writeField(out, 8, "kind", formatBarcodeType(barcode.kind));
      writeField(out, 8, "error_correction", formatBarcodeEcc(barcode.error_correction));
      out << "        \"position\": ";
      writePoint(out, 0, barcode.position);
      out << ",\n";
      out << "        \"rotation_degrees\": " << barcode.rotation_degrees << ",\n";
      out << "        \"size\": ";
      writeSize(out, 0, barcode.size);
      out << ",\n";
      out << "        \"margin\": ";
      writeSize(out, 0, barcode.margin);
      if (barcode.locked) {
        out << ",\n        \"locked\": true";
      }
      out << "\n";
      out << "      }" << (i + 1 == board.barcodes.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"reference_images\": [\n";
    for (std::size_t i = 0; i < board.reference_images.size(); ++i) {
      const BoardReferenceImage& ref = board.reference_images.at(i);
      out << "      {\n";
      writeField(out, 8, "id", ref.id);
      writeField(out, 8, "layer", ref.layer);
      writeField(out, 8, "data", ref.data);
      out << "        \"x_mm\": " << ref.x_mm << ",\n";
      out << "        \"y_mm\": " << ref.y_mm << ",\n";
      out << "        \"scale\": " << ref.scale << ",\n";
      out << "        \"opacity\": " << ref.opacity << "\n";
      out << "      }" << (i + 1 == board.reference_images.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"tables\": [\n";
    for (std::size_t i = 0; i < board.tables.size(); ++i) {
      const BoardTable& table = board.tables.at(i);
      out << "      {\n";
      writeField(out, 8, "id", table.id);
      writeField(out, 8, "layer", table.layer);
      out << "        \"x_mm\": " << table.x_mm << ",\n";
      out << "        \"y_mm\": " << table.y_mm << ",\n";
      out << "        \"rows\": " << table.rows << ",\n";
      out << "        \"cols\": " << table.cols << ",\n";
      out << "        \"width_mm\": " << table.width_mm << ",\n";
      out << "        \"height_mm\": " << table.height_mm << ",\n";
      out << "        \"cells\": [\n";
      for (std::size_t j = 0; j < table.cells.size(); ++j) {
        const BoardTableCell& cell = table.cells.at(j);
        out << "          {\n";
        out << "            \"row\": " << cell.row << ",\n";
        out << "            \"col\": " << cell.col << ",\n";
        writeField(out, 12, "text", cell.text, true);
        out << "          }" << (j + 1 == table.cells.size() ? "" : ",") << '\n';
      }
      out << "        ]\n";
      out << "      }" << (i + 1 == board.tables.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"zones\": [\n";
    for (std::size_t i = 0; i < board.zones.size(); ++i) {
      const BoardZone& zone = board.zones.at(i);
      out << "      {\n";
      writeField(out, 8, "id", zone.id);
      writeField(out, 8, "name", zone.name);
      writeField(out, 8, "net_id", zone.net_id);
      out << "        \"layer_ids\": [\n";
      for (std::size_t j = 0; j < zone.layer_ids.size(); ++j) {
        out << "          \"" << escapeJson(zone.layer_ids.at(j)) << "\""
            << (j + 1 == zone.layer_ids.size() ? "" : ",") << '\n';
      }
      out << "        ],\n";
      out << "        \"outline\": [\n";
      for (std::size_t j = 0; j < zone.outline.size(); ++j) {
        writePoint(out, 10, zone.outline.at(j));
        out << (j + 1 == zone.outline.size() ? "" : ",") << '\n';
      }
      out << "        ],\n";
      out << "        \"priority\": " << zone.priority << ",\n";
      out << "        \"clearance_nm\": " << zone.clearance.nanometers << ",\n";
      out << "        \"min_thickness_nm\": " << zone.min_thickness.nanometers << ",\n";
      out << "        \"fill_enabled\": " << (zone.fill_enabled ? "true" : "false") << ",\n";
      writeField(out, 8, "pad_connection", zone.pad_connection, zone.locked);
      if (zone.locked) {
        out << "        \"locked\": true\n";
      }
      out << "      }" << (i + 1 == board.zones.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"vias\": [\n";
    for (std::size_t i = 0; i < board.vias.size(); ++i) {
      const Via& via = board.vias.at(i);
      out << "      {\n";
      writeField(out, 8, "id", via.id);
      writeField(out, 8, "net_id", via.net_id);
      out << "        \"position\": ";
      writePoint(out, 0, via.position);
      out << ",\n";
      out << "        \"diameter_nm\": " << via.diameter.nanometers << ",\n";
      out << "        \"drill_nm\": " << via.drill.nanometers;
      if (via.locked) {
        out << ",\n        \"locked\": true";
      }
      out << "\n";
      out << "      }" << (i + 1 == board.vias.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"groups\": [\n";
    for (std::size_t i = 0; i < board.groups.size(); ++i) {
      const BoardGroup& group = board.groups.at(i);
      out << "      {\n";
      writeField(out, 8, "id", group.id);
      writeField(out, 8, "name", group.name);
      out << "        \"members\": [\n";
      for (std::size_t j = 0; j < group.members.size(); ++j) {
        out << "          \"" << escapeJson(group.members.at(j)) << "\""
            << (j + 1 == group.members.size() ? "" : ",") << '\n';
      }
      out << "        ]\n";
      out << "      }" << (i + 1 == board.groups.size() ? "" : ",") << '\n';
    }
    out << "    ],\n";
    out << "    \"route_requests\": [\n";
    for (std::size_t i = 0; i < board.route_requests.size(); ++i) {
      const RouteRequest& route_request = board.route_requests.at(i);
      out << "      {\n";
      writeField(out, 8, "id", route_request.id);
      writeField(out, 8, "net_id", route_request.net_id);
      writeField(out, 8, "from_object_id", route_request.from_object_id);
      writeField(out, 8, "to_object_id", route_request.to_object_id);
      writeField(out, 8, "preferred_layer_id", route_request.preferred_layer_id);
      writeField(out, 8, "policy", route_request.policy);
      out << "        \"width_nm\": " << route_request.width.nanometers << "\n";
      out << "      }" << (i + 1 == board.route_requests.size() ? "" : ",") << '\n';
    }
    out << "    ]\n";
    out << "  },\n";
  }

  out << "  \"components\": [\n";
  static const Schematic kEmptySchematic;
  const Schematic* sch = project.schematics.empty() ? &kEmptySchematic : &project.schematics[0];
  for (std::size_t i = 0; i < sch->symbols.size(); ++i) {
    const SchSymbol& component = sch->symbols.at(i);
    out << "    {\n";
    writeField(out, 6, "id", component.id);
    writeField(out, 6, "part", component.lib_id);
    out << "      \"position\": ";
    writePoint(out, 0, component.position);
    out << ",\n";
    out << "      \"rotation_degrees\": " << component.rotation_degrees << ",\n";
    out << "      \"pins\": [\n";
    for (std::size_t j = 0; j < component.pins.size(); ++j) {
      const SchPin& pin = component.pins.at(j);
      out << "        {\n";
      writeField(out, 10, "name", pin.name);
      writeField(out, 10, "number", pin.number);
      writeField(out, 10, "electrical_type", to_string(pin.electrical_type));
      writeField(out, 10, "graphical_style", to_string(pin.shape));
      writeField(out, 10, "orientation", to_string(pin.orientation), false);
      out << "\n        }" << (j + 1 == component.pins.size() ? "" : ",") << '\n';
    }
    out << "      ]";
    if (component.symbol.has_value()) {
      out << ",\n";
      out << "      \"symbol\": ";
      writeSymbol(out, 6, *component.symbol);
    }
    out << '\n';
    out << "    }" << (i + 1 == sch->symbols.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"constraints\": [\n";
  for (std::size_t i = 0; i < sch->constraints.size(); ++i) {
    const Constraint& constraint = sch->constraints.at(i);
    out << "    {\n";
    writeField(out, 6, "id", constraint.id);
    writeField(out, 6, "kind", constraint.kind);
    writeField(out, 6, "target", constraint.target);
    writeField(out, 6, "value", constraint.value, false);
    out << "    }" << (i + 1 == sch->constraints.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"nets\": [\n";
  for (std::size_t i = 0; i < sch->nets.size(); ++i) {
    const Net& net = sch->nets.at(i);
    out << "    {\n";
    writeField(out, 6, "id", net.id);
    out << "      \"members\": [\n";
    for (std::size_t j = 0; j < net.members.size(); ++j) {
      const NetMember& member = net.members.at(j);
      out << "        {\n";
      writeField(out, 10, "component_id", member.component_id);
      writeField(out, 10, "pin_name", member.pin_name, false);
      out << "        }" << (j + 1 == net.members.size() ? "" : ",") << '\n';
    }
    out << "      ]\n";
    out << "    }" << (i + 1 == sch->nets.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"wires\": [\n";
  for (std::size_t i = 0; i < sch->wires.size(); ++i) {
    const SchWire& wire = sch->wires.at(i);
    out << "    {\n";
    writeField(out, 6, "id", wire.id);
    out << "      \"start\": ";
    writePoint(out, 0, wire.start);
    out << ",\n";
    out << "      \"end\": ";
    writePoint(out, 0, wire.end);
    out << ",\n";
    writeField(out, 6, "net_id", wire.net_id, false);
    out << "    }" << (i + 1 == sch->wires.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"buses\": [\n";
  for (std::size_t i = 0; i < sch->buses.size(); ++i) {
    const SchBus& bus = sch->buses.at(i);
    out << "    {\n";
    writeField(out, 6, "id", bus.id);
    out << "      \"start\": ";
    writePoint(out, 0, bus.start);
    out << ",\n";
    out << "      \"end\": ";
    writePoint(out, 0, bus.end);
    out << ",\n";
    writeField(out, 6, "bus_id", bus.bus_id);
    out << "      \"net_ids\": [\n";
    for (std::size_t j = 0; j < bus.net_ids.size(); ++j) {
      out << "        \"" << escapeJson(bus.net_ids.at(j)) << "\""
          << (j + 1 == bus.net_ids.size() ? "" : ",") << '\n';
    }
    out << "      ]\n";
    out << "    }" << (i + 1 == sch->buses.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"labels\": [\n";
  for (std::size_t i = 0; i < sch->labels.size(); ++i) {
    const SchLabel& label = sch->labels.at(i);
    out << "    {\n";
    writeField(out, 6, "id", label.id);
    writeField(out, 6, "text", label.text);
    writeField(out, 6, "net_id", label.net_id);
    out << "      \"position\": ";
    writePoint(out, 0, label.position);
    out << ",\n";
    out << "      \"rotation_degrees\": " << label.rotation_degrees << ",\n";
    out << "      \"global\": " << (label.type == LabelType::Global ? "true" : "false") << '\n';
    out << "    }" << (i + 1 == sch->labels.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"power_symbols\": [\n";
  for (std::size_t i = 0; i < sch->power_symbols.size(); ++i) {
    const SchPowerSymbol& symbol = sch->power_symbols.at(i);
    out << "    {\n";
    writeField(out, 6, "id", symbol.id);
    writeField(out, 6, "value", symbol.value);
    writeField(out, 6, "net_id", symbol.net_id);
    out << "      \"position\": ";
    writePoint(out, 0, symbol.position);
    out << ",\n";
    out << "      \"rotation_degrees\": " << symbol.rotation_degrees << '\n';
    out << "    }" << (i + 1 == sch->power_symbols.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

Project loadProjectJson(const std::string& json) {
  JsonReader reader(json);
  Project project = reader.readProject();
  reader.finish();
  return project;
}

}  // namespace ccad
