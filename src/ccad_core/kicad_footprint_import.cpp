#include "ccad_core/kicad_footprint_import.hpp"

#include "ccad_core/json.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ccad {
namespace {

struct SExpr {
  std::string value;
  std::vector<SExpr> children;
};

class SExprReader {
 public:
  explicit SExprReader(std::string_view source) : source_(source) {}

  SExpr readRoot() {
    skipUtf8Bom();
    SExpr root = readExpr();
    skipWhitespace();
    if (pos_ != source_.size()) {
      throw std::runtime_error("trailing content after footprint expression");
    }
    return root;
  }

 private:
  SExpr readExpr() {
    skipWhitespace();
    expect('(');
    SExpr expr;
    expr.value = readAtom();
    while (true) {
      skipWhitespace();
      if (consume(')')) {
        return expr;
      }
      if (peek('(')) {
        expr.children.push_back(readExpr());
      } else {
        expr.children.push_back(SExpr{.value = readAtom(), .children = {}});
      }
    }
  }

  void skipUtf8Bom() {
    if (source_.size() >= 3 && static_cast<unsigned char>(source_.at(0)) == 0xEF &&
        static_cast<unsigned char>(source_.at(1)) == 0xBB &&
        static_cast<unsigned char>(source_.at(2)) == 0xBF) {
      pos_ = 3;
    }
  }

  std::string readAtom() {
    skipWhitespace();
    if (pos_ >= source_.size()) {
      throw std::runtime_error("unexpected end of input");
    }
    if (source_.at(pos_) == '"') {
      return readQuoted();
    }
    const std::size_t start = pos_;
    while (pos_ < source_.size() && !std::isspace(static_cast<unsigned char>(source_.at(pos_))) &&
           source_.at(pos_) != '(' && source_.at(pos_) != ')') {
      ++pos_;
    }
    if (start == pos_) {
      throw std::runtime_error("expected atom");
    }
    return std::string(source_.substr(start, pos_ - start));
  }

  std::string readQuoted() {
    expect('"');
    std::string value;
    while (pos_ < source_.size()) {
      const char current = source_.at(pos_++);
      if (current == '"') {
        return value;
      }
      if (current == '\\') {
        if (pos_ >= source_.size()) {
          throw std::runtime_error("unterminated escape sequence");
        }
        value.push_back(source_.at(pos_++));
      } else {
        value.push_back(current);
      }
    }
    throw std::runtime_error("unterminated quoted string");
  }

  bool peek(const char expected) const {
    return pos_ < source_.size() && source_.at(pos_) == expected;
  }

  bool consume(const char expected) {
    if (peek(expected)) {
      ++pos_;
      return true;
    }
    return false;
  }

  void expect(const char expected) {
    skipWhitespace();
    if (!consume(expected)) {
      throw std::runtime_error(std::string("expected '") + expected + "'");
    }
  }

  void skipWhitespace() {
    while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_.at(pos_)))) {
      ++pos_;
    }
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

double parseDouble(const std::string& value, const std::string& label) {
  try {
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size()) {
      throw std::runtime_error(label + " is not a number");
    }
    return number;
  } catch (const std::exception&) {
    throw std::runtime_error(label + " is not a number");
  }
}

Length parseMillimeters(const std::string& value, const std::string& label) {
  return millimeters(parseDouble(value, label));
}

bool isList(const SExpr& expr, const std::string& name) {
  return expr.value == name;
}

FootprintPad importPad(const SExpr& expr) {
  if (expr.children.size() < 3) {
    throw std::runtime_error("pad requires number, type, and shape");
  }

  FootprintPad pad;
  pad.number = expr.children.at(0).value;
  pad.type = expr.children.at(1).value;
  pad.shape = expr.children.at(2).value;

  for (std::size_t i = 3; i < expr.children.size(); ++i) {
    const SExpr& child = expr.children.at(i);
    if (isList(child, "at")) {
      if (child.children.size() < 2) {
        throw std::runtime_error("pad at requires x and y");
      }
      pad.position.x = parseMillimeters(child.children.at(0).value, "pad at x");
      pad.position.y = parseMillimeters(child.children.at(1).value, "pad at y");
      if (child.children.size() >= 3) {
        pad.rotation_degrees = parseDouble(child.children.at(2).value, "pad rotation");
      }
    } else if (isList(child, "size")) {
      if (child.children.size() != 2) {
        throw std::runtime_error("pad size requires width and height");
      }
      pad.size.width = parseMillimeters(child.children.at(0).value, "pad width");
      pad.size.height = parseMillimeters(child.children.at(1).value, "pad height");
    } else if (isList(child, "drill")) {
      if (!child.children.empty()) {
        pad.drill = parseMillimeters(child.children.at(0).value, "pad drill");
      }
    } else if (isList(child, "layers")) {
      for (const SExpr& layer : child.children) {
        pad.layers.push_back(layer.value);
      }
    } else if (isList(child, "roundrect_rratio")) {
      if (child.children.size() != 1) {
        throw std::runtime_error("pad roundrect_rratio requires one value");
      }
      pad.roundrect_rratio = parseDouble(child.children.at(0).value, "pad roundrect ratio");
    } else if (isList(child, "chamfer_ratio")) {
      if (child.children.size() != 1) {
        throw std::runtime_error("pad chamfer_ratio requires one value");
      }
      pad.chamfer_ratio = parseDouble(child.children.at(0).value, "pad chamfer ratio");
    }
  }

  return pad;
}

FootprintLine importLine(const SExpr& expr) {
  FootprintLine line;
  for (std::size_t i = 1; i < expr.children.size(); ++i) {
    const SExpr& child = expr.children.at(i);
    if (isList(child, "start") && child.children.size() >= 2) {
      line.start.x = parseMillimeters(child.children.at(0).value, "line start x");
      line.start.y = parseMillimeters(child.children.at(1).value, "line start y");
    } else if (isList(child, "end") && child.children.size() >= 2) {
      line.end.x = parseMillimeters(child.children.at(0).value, "line end x");
      line.end.y = parseMillimeters(child.children.at(1).value, "line end y");
    } else if (isList(child, "layer") && child.children.size() >= 1) {
      line.layer = child.children.at(0).value;
    } else if (isList(child, "stroke")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "width") && s.children.size() >= 1) {
          line.stroke_width = parseMillimeters(s.children.at(0).value, "line width");
        }
      }
    } else if (isList(child, "width") && child.children.size() >= 1) {
      line.stroke_width = parseMillimeters(child.children.at(0).value, "line width");
    }
  }
  return line;
}

FootprintArc importArc(const SExpr& expr) {
  FootprintArc arc;
  for (std::size_t i = 1; i < expr.children.size(); ++i) {
    const SExpr& child = expr.children.at(i);
    if (isList(child, "start") && child.children.size() >= 2) {
      arc.start.x = parseMillimeters(child.children.at(0).value, "arc start x");
      arc.start.y = parseMillimeters(child.children.at(1).value, "arc start y");
    } else if (isList(child, "end") && child.children.size() >= 2) {
      arc.end.x = parseMillimeters(child.children.at(0).value, "arc end x");
      arc.end.y = parseMillimeters(child.children.at(1).value, "arc end y");
    } else if (isList(child, "mid") && child.children.size() >= 2) {
      arc.center.x = parseMillimeters(child.children.at(0).value, "arc mid x");
      arc.center.y = parseMillimeters(child.children.at(1).value, "arc mid y");
    } else if (isList(child, "layer") && child.children.size() >= 1) {
      arc.layer = child.children.at(0).value;
    } else if (isList(child, "stroke")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "width") && s.children.size() >= 1) {
          arc.stroke_width = parseMillimeters(s.children.at(0).value, "arc width");
        }
      }
    } else if (isList(child, "width") && child.children.size() >= 1) {
      arc.stroke_width = parseMillimeters(child.children.at(0).value, "arc width");
    }
  }
  return arc;
}

FootprintCircle importCircle(const SExpr& expr) {
  FootprintCircle circle;
  for (std::size_t i = 1; i < expr.children.size(); ++i) {
    const SExpr& child = expr.children.at(i);
    if (isList(child, "center") && child.children.size() >= 2) {
      circle.center.x = parseMillimeters(child.children.at(0).value, "circle center x");
      circle.center.y = parseMillimeters(child.children.at(1).value, "circle center y");
    } else if (isList(child, "end") && child.children.size() >= 2) {
      circle.end.x = parseMillimeters(child.children.at(0).value, "circle end x");
      circle.end.y = parseMillimeters(child.children.at(1).value, "circle end y");
    } else if (isList(child, "layer") && child.children.size() >= 1) {
      circle.layer = child.children.at(0).value;
    } else if (isList(child, "stroke")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "width") && s.children.size() >= 1) {
          circle.stroke_width = parseMillimeters(s.children.at(0).value, "circle width");
        }
      }
    } else if (isList(child, "width") && child.children.size() >= 1) {
      circle.stroke_width = parseMillimeters(child.children.at(0).value, "circle width");
    }
  }
  return circle;
}

FootprintText importText(const SExpr& expr) {
  FootprintText text;
  if (expr.children.size() >= 2) {
    text.type = expr.children.at(0).value;
    text.text = expr.children.at(1).value;
  }
  for (std::size_t i = 2; i < expr.children.size(); ++i) {
    const SExpr& child = expr.children.at(i);
    if (isList(child, "at") && child.children.size() >= 2) {
      text.position.x = parseMillimeters(child.children.at(0).value, "text at x");
      text.position.y = parseMillimeters(child.children.at(1).value, "text at y");
      if (child.children.size() >= 3) {
        text.rotation_degrees = parseDouble(child.children.at(2).value, "text rotation");
      }
    } else if (isList(child, "layer") && child.children.size() >= 1) {
      text.layer = child.children.at(0).value;
    } else if (isList(child, "effects")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "font")) {
          for (const SExpr& f : s.children) {
            if (isList(f, "size") && f.children.size() >= 2) {
              text.size_width = parseMillimeters(f.children.at(0).value, "text size width");
              text.size_height = parseMillimeters(f.children.at(1).value, "text size height");
            } else if (isList(f, "thickness") && f.children.size() >= 1) {
              text.stroke_width = parseMillimeters(f.children.at(0).value, "text thickness");
            }
          }
        }
      }
    }
  }
  return text;
}

FootprintModel3D importModel(const SExpr& expr) {
  FootprintModel3D model;
  if (expr.children.size() >= 1) {
    model.path = expr.children.at(0).value;
  }
  for (std::size_t i = 1; i < expr.children.size(); ++i) {
    const SExpr& child = expr.children.at(i);
    if (isList(child, "offset")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "xyz") && s.children.size() >= 3) {
          model.offset.x = parseMillimeters(s.children.at(0).value, "model offset x");
          model.offset.y = parseMillimeters(s.children.at(1).value, "model offset y");
          model.offset_z = parseDouble(s.children.at(2).value, "model offset z");
        }
      }
    } else if (isList(child, "scale")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "xyz") && s.children.size() >= 3) {
          model.scale_x = parseDouble(s.children.at(0).value, "model scale x");
          model.scale_y = parseDouble(s.children.at(1).value, "model scale y");
          model.scale_z = parseDouble(s.children.at(2).value, "model scale z");
        }
      }
    } else if (isList(child, "rotate")) {
      for (const SExpr& s : child.children) {
        if (isList(s, "xyz") && s.children.size() >= 3) {
          model.rotate_x = parseDouble(s.children.at(0).value, "model rotate x");
          model.rotate_y = parseDouble(s.children.at(1).value, "model rotate y");
          model.rotate_z = parseDouble(s.children.at(2).value, "model rotate z");
        }
      }
    }
  }
  return model;
}

void writeStringArray(std::ostringstream& out, const int indent,
                      const std::vector<std::string>& values) {
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << '"' << escapeJson(values.at(i)) << '"';
  }
  out << "]";
  (void)indent;
}

class FootprintJsonReader {
 public:
  explicit FootprintJsonReader(std::string_view source) : source_(source) {}

  Footprint readFootprint() {
    Footprint footprint;
    expect('{');
    if (consume('}')) {
      return footprint;
    }
    while (true) {
      const std::string key = readString();
      expect(':');
      if (key == "name") {
        footprint.name = readString();
      } else if (key == "exclude_from_bom") {
        footprint.exclude_from_bom = readBool();
      } else if (key == "pads") {
        footprint.pads = readPads();
      } else if (key == "lines") {
        footprint.lines = readLines();
      } else if (key == "arcs") {
        footprint.arcs = readArcs();
      } else if (key == "circles") {
        footprint.circles = readCircles();
      } else if (key == "texts") {
        footprint.texts = readTexts();
      } else if (key == "models") {
        footprint.models = readModels();
      } else {
        throw std::runtime_error("unknown footprint json key: " + key);
      }
      if (consume('}')) {
        finish();
        return footprint;
      }
      expect(',');
    }
  }

 private:
  std::vector<FootprintPad> readPads() {
    std::vector<FootprintPad> pads;
    expect('[');
    if (consume(']')) {
      return pads;
    }
    while (true) {
      pads.push_back(readPad());
      if (consume(']')) {
        return pads;
      }
      expect(',');
    }
  }

  FootprintPad readPad() {
    FootprintPad pad;
    expect('{');
    if (consume('}')) {
      return pad;
    }
    while (true) {
      const std::string key = readString();
      expect(':');
      if (key == "number") {
        pad.number = readString();
      } else if (key == "type") {
        pad.type = readString();
      } else if (key == "shape") {
        pad.shape = readString();
      } else if (key == "x_nm") {
        pad.position.x = nanometers(readInt64());
      } else if (key == "y_nm") {
        pad.position.y = nanometers(readInt64());
      } else if (key == "rotation_degrees") {
        pad.rotation_degrees = readNumber();
      } else if (key == "width_nm") {
        pad.size.width = nanometers(readInt64());
      } else if (key == "height_nm") {
        pad.size.height = nanometers(readInt64());
      } else if (key == "drill_nm") {
        pad.drill = nanometers(readInt64());
      } else if (key == "layers") {
        pad.layers = readStringArray();
      } else if (key == "roundrect_rratio") {
        pad.roundrect_rratio = readNumber();
      } else if (key == "chamfer_ratio") {
        pad.chamfer_ratio = readNumber();
      } else {
        throw std::runtime_error("unknown footprint pad json key: " + key);
      }
      if (consume('}')) {
        return pad;
      }
      expect(',');
    }
  }

  std::vector<FootprintLine> readLines() {
    std::vector<FootprintLine> lines;
    expect('[');
    if (consume(']')) return lines;
    while (true) {
      FootprintLine line;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "start_x") line.start.x = nanometers(readInt64());
          else if (key == "start_y") line.start.y = nanometers(readInt64());
          else if (key == "end_x") line.end.x = nanometers(readInt64());
          else if (key == "end_y") line.end.y = nanometers(readInt64());
          else if (key == "stroke_width") line.stroke_width = nanometers(readInt64());
          else if (key == "layer") line.layer = readString();
          else throw std::runtime_error("unknown line key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      lines.push_back(line);
      if (consume(']')) break;
      expect(',');
    }
    return lines;
  }

  std::vector<FootprintArc> readArcs() {
    std::vector<FootprintArc> arcs;
    expect('[');
    if (consume(']')) return arcs;
    while (true) {
      FootprintArc arc;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "start_x") arc.start.x = nanometers(readInt64());
          else if (key == "start_y") arc.start.y = nanometers(readInt64());
          else if (key == "end_x") arc.end.x = nanometers(readInt64());
          else if (key == "end_y") arc.end.y = nanometers(readInt64());
          else if (key == "center_x") arc.center.x = nanometers(readInt64());
          else if (key == "center_y") arc.center.y = nanometers(readInt64());
          else if (key == "stroke_width") arc.stroke_width = nanometers(readInt64());
          else if (key == "layer") arc.layer = readString();
          else throw std::runtime_error("unknown arc key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      arcs.push_back(arc);
      if (consume(']')) break;
      expect(',');
    }
    return arcs;
  }

  std::vector<FootprintCircle> readCircles() {
    std::vector<FootprintCircle> circles;
    expect('[');
    if (consume(']')) return circles;
    while (true) {
      FootprintCircle circle;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "center_x") circle.center.x = nanometers(readInt64());
          else if (key == "center_y") circle.center.y = nanometers(readInt64());
          else if (key == "end_x") circle.end.x = nanometers(readInt64());
          else if (key == "end_y") circle.end.y = nanometers(readInt64());
          else if (key == "stroke_width") circle.stroke_width = nanometers(readInt64());
          else if (key == "layer") circle.layer = readString();
          else throw std::runtime_error("unknown circle key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      circles.push_back(circle);
      if (consume(']')) break;
      expect(',');
    }
    return circles;
  }

  std::vector<FootprintText> readTexts() {
    std::vector<FootprintText> texts;
    expect('[');
    if (consume(']')) return texts;
    while (true) {
      FootprintText text;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "type") text.type = readString();
          else if (key == "text") text.text = readString();
          else if (key == "x_nm") text.position.x = nanometers(readInt64());
          else if (key == "y_nm") text.position.y = nanometers(readInt64());
          else if (key == "rotation_degrees") text.rotation_degrees = readNumber();
          else if (key == "size_width") text.size_width = nanometers(readInt64());
          else if (key == "size_height") text.size_height = nanometers(readInt64());
          else if (key == "stroke_width") text.stroke_width = nanometers(readInt64());
          else if (key == "layer") text.layer = readString();
          else throw std::runtime_error("unknown text key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      texts.push_back(text);
      if (consume(']')) break;
      expect(',');
    }
    return texts;
  }

  std::vector<FootprintModel3D> readModels() {
    std::vector<FootprintModel3D> models;
    expect('[');
    if (consume(']')) return models;
    while (true) {
      FootprintModel3D model;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "path") model.path = readString();
          else if (key == "offset_x") model.offset.x = nanometers(readInt64());
          else if (key == "offset_y") model.offset.y = nanometers(readInt64());
          else if (key == "offset_z") model.offset_z = readNumber();
          else if (key == "scale_x") model.scale_x = readNumber();
          else if (key == "scale_y") model.scale_y = readNumber();
          else if (key == "scale_z") model.scale_z = readNumber();
          else if (key == "rotate_x") model.rotate_x = readNumber();
          else if (key == "rotate_y") model.rotate_y = readNumber();
          else if (key == "rotate_z") model.rotate_z = readNumber();
          else throw std::runtime_error("unknown model key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      models.push_back(model);
      if (consume(']')) break;
      expect(',');
    }
    return models;
  }

  std::vector<std::string> readStringArray() {
    std::vector<std::string> values;
    expect('[');
    if (consume(']')) {
      return values;
    }
    while (true) {
      values.push_back(readString());
      if (consume(']')) {
        return values;
      }
      expect(',');
    }
  }

  std::string readString() {
    skipWhitespace();
    expectRaw('"');
    std::string value;
    while (pos_ < source_.size()) {
      const char current = source_.at(pos_++);
      if (current == '"') {
        return value;
      }
      if (current == '\\') {
        if (pos_ >= source_.size()) {
          throw std::runtime_error("unterminated json escape");
        }
        const char escaped = source_.at(pos_++);
        if (escaped == 'n') {
          value.push_back('\n');
        } else if (escaped == 't') {
          value.push_back('\t');
        } else {
          value.push_back(escaped);
        }
      } else {
        value.push_back(current);
      }
    }
    throw std::runtime_error("unterminated json string");
  }

  std::int64_t readInt64() {
    return static_cast<std::int64_t>(readNumber());
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
    throw std::runtime_error("expected json bool");
  }

  double readNumber() {
    skipWhitespace();
    const std::size_t start = pos_;
    if (pos_ < source_.size() && source_.at(pos_) == '-') {
      ++pos_;
    }
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_.at(pos_)))) {
      ++pos_;
    }
    if (pos_ < source_.size() && source_.at(pos_) == '.') {
      ++pos_;
      while (pos_ < source_.size() &&
             std::isdigit(static_cast<unsigned char>(source_.at(pos_)))) {
        ++pos_;
      }
    }
    if (start == pos_) {
      throw std::runtime_error("expected json number");
    }
    return parseDouble(std::string(source_.substr(start, pos_ - start)), "json number");
  }

  bool consume(char expected) {
    skipWhitespace();
    if (pos_ < source_.size() && source_.at(pos_) == expected) {
      ++pos_;
      return true;
    }
    return false;
  }

  void expect(char expected) {
    skipWhitespace();
    expectRaw(expected);
  }

  void expectRaw(char expected) {
    if (pos_ >= source_.size() || source_.at(pos_) != expected) {
      throw std::runtime_error(std::string("expected json '") + expected + "'");
    }
    ++pos_;
  }

  void skipWhitespace() {
    while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_.at(pos_)))) {
      ++pos_;
    }
  }

  void finish() {
    skipWhitespace();
    if (pos_ != source_.size()) {
      throw std::runtime_error("trailing content after footprint json");
    }
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

}  // namespace

Footprint importKiCadFootprint(const std::string_view source) {
  const SExpr root = SExprReader(source).readRoot();
  if (root.value != "footprint") {
    throw std::runtime_error("root expression must be footprint");
  }
  if (root.children.empty()) {
    throw std::runtime_error("footprint requires name");
  }

  Footprint footprint;
  footprint.name = root.children.at(0).value;

  for (std::size_t i = 1; i < root.children.size(); ++i) {
    const SExpr& child = root.children.at(i);
    if (isList(child, "pad")) {
      footprint.pads.push_back(importPad(child));
    } else if (isList(child, "fp_line")) {
      footprint.lines.push_back(importLine(child));
    } else if (isList(child, "fp_arc")) {
      footprint.arcs.push_back(importArc(child));
    } else if (isList(child, "fp_circle")) {
      footprint.circles.push_back(importCircle(child));
    } else if (isList(child, "fp_text")) {
      footprint.texts.push_back(importText(child));
    } else if (isList(child, "model")) {
      footprint.models.push_back(importModel(child));
    } else if (isList(child, "attr")) {
      for (const SExpr& attribute : child.children) {
        if (attribute.value == "exclude_from_bom") {
          footprint.exclude_from_bom = true;
        }
      }
    }
  }

  return footprint;
}

std::string dumpFootprintJson(const Footprint& footprint) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"name\": \"" << escapeJson(footprint.name) << "\",\n";
  out << "  \"exclude_from_bom\": " << (footprint.exclude_from_bom ? "true" : "false")
      << ",\n";
  out << "  \"pads\": [\n";
  for (std::size_t i = 0; i < footprint.pads.size(); ++i) {
    const FootprintPad& pad = footprint.pads.at(i);
    out << "    {\n";
    out << "      \"number\": \"" << escapeJson(pad.number) << "\",\n";
    out << "      \"type\": \"" << escapeJson(pad.type) << "\",\n";
    out << "      \"shape\": \"" << escapeJson(pad.shape) << "\",\n";
    out << "      \"x_nm\": " << pad.position.x.nanometers << ",\n";
    out << "      \"y_nm\": " << pad.position.y.nanometers << ",\n";
    out << "      \"rotation_degrees\": " << pad.rotation_degrees << ",\n";
    out << "      \"width_nm\": " << pad.size.width.nanometers << ",\n";
    out << "      \"height_nm\": " << pad.size.height.nanometers << ",\n";
    if (pad.drill.has_value()) {
      out << "      \"drill_nm\": " << pad.drill->nanometers << ",\n";
    }
    if (pad.roundrect_rratio.has_value()) {
      out << "      \"roundrect_rratio\": " << *pad.roundrect_rratio << ",\n";
    }
    if (pad.chamfer_ratio.has_value()) {
      out << "      \"chamfer_ratio\": " << *pad.chamfer_ratio << ",\n";
    }
    out << "      \"layers\": ";
    writeStringArray(out, 6, pad.layers);
    out << "\n";
    out << "    }" << (i + 1 == footprint.pads.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

Footprint loadFootprintJson(const std::string_view source) {
  return FootprintJsonReader(source).readFootprint();
}

}  // namespace ccad
