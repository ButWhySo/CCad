#include "ccad_core/serialize.hpp"

#include "ccad_core/json.hpp"
#include "ccad_core/symbol_json_reader.hpp"

#include <cctype>
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

void writeSymbolJson(std::ostringstream& out, const int indent, const Symbol& symbol) {
  const std::string pad(indent, ' ');
  out << pad << "{\n";
  writeField(out, indent + 2, "name", symbol.name);
  writeField(out, indent + 2, "extends", symbol.extends);

  out << std::string(indent + 2, ' ') << "\"pins\": [\n";
  for (std::size_t i = 0; i < symbol.pins.size(); ++i) {
    const SymbolPin& pin = symbol.pins.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    writeField(out, indent + 6, "name", pin.name);
    writeField(out, indent + 6, "number", pin.number);
    writeField(out, indent + 6, "electrical_type", pin.electrical_type);
    writeField(out, indent + 6, "graphical_style", pin.graphical_style);
    out << std::string(indent + 6, ' ') << "\"x_nm\": " << pin.position.x.nanometers << ",\n";
    out << std::string(indent + 6, ' ') << "\"y_nm\": " << pin.position.y.nanometers << ",\n";
    out << std::string(indent + 6, ' ') << "\"rotation_degrees\": " << pin.rotation_degrees
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"length_nm\": " << pin.length.nanometers << "\n";
    out << std::string(indent + 4, ' ') << "}" << (i + 1 == symbol.pins.size() ? "" : ",")
        << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"properties\": [\n";
  for (std::size_t i = 0; i < symbol.properties.size(); ++i) {
    const SymbolProperty& property = symbol.properties.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    writeField(out, indent + 6, "name", property.name);
    writeField(out, indent + 6, "value", property.value, false);
    out << std::string(indent + 4, ' ') << "}"
        << (i + 1 == symbol.properties.size() ? "" : ",") << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"rectangles\": [\n";
  for (std::size_t i = 0; i < symbol.rectangles.size(); ++i) {
    const SymbolRectangle& rectangle = symbol.rectangles.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    out << std::string(indent + 6, ' ') << "\"start_x\": " << rectangle.start.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"start_y\": " << rectangle.start.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"end_x\": " << rectangle.end.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"end_y\": " << rectangle.end.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"stroke_width\": "
        << rectangle.stroke_width.nanometers << ",\n";
    writeField(out, indent + 6, "fill_type", rectangle.fill_type, false);
    out << std::string(indent + 4, ' ') << "}"
        << (i + 1 == symbol.rectangles.size() ? "" : ",") << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"lines\": [\n";
  for (std::size_t i = 0; i < symbol.lines.size(); ++i) {
    const SymbolLine& line = symbol.lines.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    out << std::string(indent + 6, ' ') << "\"start_x\": " << line.start.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"start_y\": " << line.start.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"end_x\": " << line.end.x.nanometers << ",\n";
    out << std::string(indent + 6, ' ') << "\"end_y\": " << line.end.y.nanometers << ",\n";
    out << std::string(indent + 6, ' ') << "\"stroke_width\": " << line.stroke_width.nanometers
        << "\n";
    out << std::string(indent + 4, ' ') << "}" << (i + 1 == symbol.lines.size() ? "" : ",")
        << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"arcs\": [\n";
  for (std::size_t i = 0; i < symbol.arcs.size(); ++i) {
    const SymbolArc& arc = symbol.arcs.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    out << std::string(indent + 6, ' ') << "\"start_x\": " << arc.start.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"start_y\": " << arc.start.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"end_x\": " << arc.end.x.nanometers << ",\n";
    out << std::string(indent + 6, ' ') << "\"end_y\": " << arc.end.y.nanometers << ",\n";
    out << std::string(indent + 6, ' ') << "\"center_x\": " << arc.center.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"center_y\": " << arc.center.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"stroke_width\": " << arc.stroke_width.nanometers
        << "\n";
    out << std::string(indent + 4, ' ') << "}" << (i + 1 == symbol.arcs.size() ? "" : ",")
        << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"circles\": [\n";
  for (std::size_t i = 0; i < symbol.circles.size(); ++i) {
    const SymbolCircle& circle = symbol.circles.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    out << std::string(indent + 6, ' ') << "\"center_x\": " << circle.center.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"center_y\": " << circle.center.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"radius\": " << circle.radius.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"stroke_width\": "
        << circle.stroke_width.nanometers << ",\n";
    writeField(out, indent + 6, "fill_type", circle.fill_type, false);
    out << std::string(indent + 4, ' ') << "}"
        << (i + 1 == symbol.circles.size() ? "" : ",") << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"polylines\": [\n";
  for (std::size_t i = 0; i < symbol.polylines.size(); ++i) {
    const SymbolPolyline& polyline = symbol.polylines.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    out << std::string(indent + 6, ' ') << "\"points\": [\n";
    for (std::size_t j = 0; j < polyline.points.size(); ++j) {
      const Point& point = polyline.points.at(j);
      out << std::string(indent + 8, ' ') << "{\n";
      out << std::string(indent + 10, ' ') << "\"x\": " << point.x.nanometers << ",\n";
      out << std::string(indent + 10, ' ') << "\"y\": " << point.y.nanometers << "\n";
      out << std::string(indent + 8, ' ') << "}"
          << (j + 1 == polyline.points.size() ? "" : ",") << '\n';
    }
    out << std::string(indent + 6, ' ') << "],\n";
    out << std::string(indent + 6, ' ') << "\"stroke_width\": "
        << polyline.stroke_width.nanometers << ",\n";
    writeField(out, indent + 6, "fill_type", polyline.fill_type, false);
    out << std::string(indent + 4, ' ') << "}"
        << (i + 1 == symbol.polylines.size() ? "" : ",") << '\n';
  }
  out << std::string(indent + 2, ' ') << "],\n";

  out << std::string(indent + 2, ' ') << "\"texts\": [\n";
  for (std::size_t i = 0; i < symbol.texts.size(); ++i) {
    const SymbolText& text = symbol.texts.at(i);
    out << std::string(indent + 4, ' ') << "{\n";
    writeField(out, indent + 6, "text", text.text);
    out << std::string(indent + 6, ' ') << "\"x_nm\": " << text.position.x.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"y_nm\": " << text.position.y.nanometers
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"rotation_degrees\": " << text.rotation_degrees
        << ",\n";
    out << std::string(indent + 6, ' ') << "\"size\": " << text.size.nanometers << "\n";
    out << std::string(indent + 4, ' ') << "}" << (i + 1 == symbol.texts.size() ? "" : ",")
        << '\n';
  }
  out << std::string(indent + 2, ' ') << "]\n";
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
      } else if (key == "board") {
        project.boards.clear();
        project.boards.push_back(readBoard());
      } else if (key == "components") {
        ensureSchematic(project).components = readComponents();
      } else if (key == "nets") {
        ensureSchematic(project).nets = readNets();
      } else if (key == "wires") {
        ensureSchematic(project).wires = readWireSegments();
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
        } else if (key == "graphics") {
          board.graphics = readBoardGraphics();
        } else if (key == "texts") {
          board.texts = readBoardTexts();
        } else if (key == "zones") {
          board.zones = readBoardZones();
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
          } else if (key == "kind") {
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
          } else if (key == "layer_id") {
            pad.layers = {readString()};
          } else if (key == "layers") {
            pad.layers = readStringArray();
          } else if (key == "type") {
            pad.type = readString();
          } else if (key == "shape") {
            pad.shape = readString();
          } else if (key == "position") {
            pad.position = readPoint();
          } else if (key == "rotation_degrees") {
            pad.rotation_degrees = readDouble();
          } else if (key == "size") {
            pad.size = readSize();
          } else if (key == "drill_nm") {
            pad.drill = nanometers(readInt64());
          } else if (key == "roundrect_rratio") {
            pad.roundrect_rratio = readDouble();
          } else if (key == "chamfer_ratio") {
            pad.chamfer_ratio = readDouble();
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
          } else if (key == "kind") {
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
          } else if (key == "kind") {
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
      if (peek(']')) {
        throw std::runtime_error("trailing comma in tracks array");
      }
    }
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
          } else if (key == "kind") {
            graphic.kind = readString();
          } else if (key == "layer_id") {
            graphic.layer_id = readString();
          } else if (key == "start") {
            graphic.start = readPoint();
          } else if (key == "end") {
            graphic.end = readPoint();
          } else if (key == "width_nm") {
            graphic.width = nanometers(readInt64());
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

  std::vector<Component> readComponents() {
    std::vector<Component> components;
    expect('[');
    if (consume(']')) {
      return components;
    }
    while (true) {
      Component component;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          component.id = readString();
        } else if (key == "part") {
          component.part = readString();
        } else if (key == "position") {
          component.position = readPoint();
        } else if (key == "rotation_degrees") {
          component.rotation_degrees = readDouble();
        } else if (key == "pins") {
          component.pins = readPins();
        } else if (key == "symbol") {
          const std::string raw_symbol = readRawJsonObject();
          component.symbol = SymbolJsonReader(raw_symbol).readSymbol();
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
      components.push_back(component);
      if (consume(']')) {
        return components;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in components array");
      }
    }
  }

  std::vector<Pin> readPins() {
    std::vector<Pin> pins;
    expect('[');
    if (consume(']')) {
      return pins;
    }
    while (true) {
      Pin pin;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "name") {
          pin.name = readString();
        } else if (key == "kind") {
          pin.kind = readString();
        } else {
          throw std::runtime_error("unknown pin key: " + key);
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

  std::vector<WireSegment> readWireSegments() {
    std::vector<WireSegment> wires;
    expect('[');
    if (consume(']')) {
      return wires;
    }
    while (true) {
      WireSegment wire;
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

  std::vector<Label> readLabels() {
    std::vector<Label> labels;
    expect('[');
    if (consume(']')) return labels;
    while (true) {
      Label label;
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
          else if (key == "global") label.global = readBool();
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

  std::vector<PowerSymbol> readPowerSymbols() {
    std::vector<PowerSymbol> power_symbols;
    expect('[');
    if (consume(']')) return power_symbols;
    while (true) {
      PowerSymbol symbol;
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
        } else if (key == "kind") {
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
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) {
      found = true;
      value = (value * 10) + (source_[pos_] - '0');
      ++pos_;
    }
    if (!found) {
      throw std::runtime_error("expected integer");
    }
    return value;
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

std::string dumpProjectJson(const Project& project) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema_version\": " << project.schema_version << ",\n";
  writeField(out, 2, "id", project.id);
  writeField(out, 2, "name", project.name);

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
        << board.design_rules.min_via_annular_ring.nanometers << "\n";
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
      out << "        \"layers\": [\n";
      for (std::size_t j = 0; j < pad.layers.size(); ++j) {
        out << "          \"" << escapeJson(pad.layers.at(j)) << "\""
            << (j + 1 == pad.layers.size() ? "" : ",") << '\n';
      }
      out << "        ],\n";
      writeField(out, 8, "type", pad.type);
      writeField(out, 8, "shape", pad.shape);
      out << "        \"position\": ";
      writePoint(out, 0, pad.position);
      out << ",\n";
      out << "        \"rotation_degrees\": " << pad.rotation_degrees << ",\n";
      out << "        \"size\": ";
      writeSize(out, 0, pad.size);
      if (pad.drill.has_value()) {
        out << ",\n        \"drill_nm\": " << pad.drill->nanometers;
      }
      if (pad.roundrect_rratio.has_value()) {
        out << ",\n        \"roundrect_rratio\": " << *pad.roundrect_rratio;
      }
      if (pad.chamfer_ratio.has_value()) {
        out << ",\n        \"chamfer_ratio\": " << *pad.chamfer_ratio;
      }
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
      writeField(out, 8, "source_route_request_id", track.source_route_request_id, false);
      out << "      }" << (i + 1 == board.tracks.size() ? "" : ",") << '\n';
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
      out << "        \"end\": ";
      writePoint(out, 0, graphic.end);
      out << ",\n";
      out << "        \"width_nm\": " << graphic.width.nanometers << "\n";
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
      out << "\n";
      out << "      }" << (i + 1 == board.texts.size() ? "" : ",") << '\n';
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
      writeField(out, 8, "pad_connection", zone.pad_connection, false);
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
      out << "        \"drill_nm\": " << via.drill.nanometers << "\n";
      out << "      }" << (i + 1 == board.vias.size() ? "" : ",") << '\n';
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
  for (std::size_t i = 0; i < sch->components.size(); ++i) {
    const Component& component = sch->components.at(i);
    out << "    {\n";
    writeField(out, 6, "id", component.id);
    writeField(out, 6, "part", component.part);
    out << "      \"position\": ";
    writePoint(out, 0, component.position);
    out << ",\n";
    out << "      \"rotation_degrees\": " << component.rotation_degrees << ",\n";
    out << "      \"pins\": [\n";
    for (std::size_t j = 0; j < component.pins.size(); ++j) {
      const Pin& pin = component.pins.at(j);
      out << "        {\n";
      writeField(out, 10, "kind", pin.kind);
      writeField(out, 10, "name", pin.name, false);
      out << "        }" << (j + 1 == component.pins.size() ? "" : ",") << '\n';
    }
    out << "      ]";
    if (component.symbol.has_value()) {
      out << ",\n";
      out << "      \"symbol\": ";
      writeSymbolJson(out, 0, *component.symbol);
      out << '\n';
    } else {
      out << '\n';
    }
    out << "    }" << (i + 1 == sch->components.size() ? "" : ",") << '\n';
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
    const WireSegment& wire = sch->wires.at(i);
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

  out << "  \"labels\": [\n";
  for (std::size_t i = 0; i < sch->labels.size(); ++i) {
    const Label& label = sch->labels.at(i);
    out << "    {\n";
    writeField(out, 6, "id", label.id);
    writeField(out, 6, "text", label.text);
    writeField(out, 6, "net_id", label.net_id);
    out << "      \"position\": ";
    writePoint(out, 0, label.position);
    out << ",\n";
    out << "      \"rotation_degrees\": " << label.rotation_degrees << ",\n";
    out << "      \"global\": " << (label.global ? "true" : "false") << '\n';
    out << "    }" << (i + 1 == sch->labels.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"power_symbols\": [\n";
  for (std::size_t i = 0; i < sch->power_symbols.size(); ++i) {
    const PowerSymbol& symbol = sch->power_symbols.at(i);
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
