#include "ccad_core/serialize.hpp"

#include "ccad_core/json.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>

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

class JsonReader {
 public:
  explicit JsonReader(std::string_view source) : source_(source) {}

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
        project.board = readBoard();
      } else if (key == "components") {
        project.components = readComponents();
      } else if (key == "nets") {
        project.nets = readNets();
      } else if (key == "constraints") {
        project.constraints = readConstraints();
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
            pad.layer_id = readString();
          } else if (key == "position") {
            pad.position = readPoint();
          } else if (key == "rotation_degrees") {
            pad.rotation_degrees = readDouble();
          } else if (key == "size") {
            pad.size = readSize();
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
        } else if (key == "pins") {
          component.pins = readPins();
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

  if (project.board.has_value()) {
    const Board& board = *project.board;
    out << "  \"board\": {\n";
    out << "    \"outline\": {\n";
    out << "      \"x_nm\": " << board.outline.origin.x.nanometers << ",\n";
    out << "      \"y_nm\": " << board.outline.origin.y.nanometers << ",\n";
    out << "      \"width_nm\": " << board.outline.size.width.nanometers << ",\n";
    out << "      \"height_nm\": " << board.outline.size.height.nanometers << "\n";
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
      writeField(out, 8, "layer_id", pad.layer_id);
      out << "        \"position\": ";
      writePoint(out, 0, pad.position);
      out << ",\n";
      out << "        \"rotation_degrees\": " << pad.rotation_degrees << ",\n";
      out << "        \"size\": ";
      writeSize(out, 0, pad.size);
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
      out << "        \"width_nm\": " << track.width.nanometers << "\n";
      out << "      }" << (i + 1 == board.tracks.size() ? "" : ",") << '\n';
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
    out << "    ]\n";
    out << "  },\n";
  }

  out << "  \"components\": [\n";
  for (std::size_t i = 0; i < project.components.size(); ++i) {
    const Component& component = project.components.at(i);
    out << "    {\n";
    writeField(out, 6, "id", component.id);
    writeField(out, 6, "part", component.part);
    out << "      \"pins\": [\n";
    for (std::size_t j = 0; j < component.pins.size(); ++j) {
      const Pin& pin = component.pins.at(j);
      out << "        {\n";
      writeField(out, 10, "kind", pin.kind);
      writeField(out, 10, "name", pin.name, false);
      out << "        }" << (j + 1 == component.pins.size() ? "" : ",") << '\n';
    }
    out << "      ]\n";
    out << "    }" << (i + 1 == project.components.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"constraints\": [\n";
  for (std::size_t i = 0; i < project.constraints.size(); ++i) {
    const Constraint& constraint = project.constraints.at(i);
    out << "    {\n";
    writeField(out, 6, "id", constraint.id);
    writeField(out, 6, "kind", constraint.kind);
    writeField(out, 6, "target", constraint.target);
    writeField(out, 6, "value", constraint.value, false);
    out << "    }" << (i + 1 == project.constraints.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"nets\": [\n";
  for (std::size_t i = 0; i < project.nets.size(); ++i) {
    const Net& net = project.nets.at(i);
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
    out << "    }" << (i + 1 == project.nets.size() ? "" : ",") << '\n';
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
