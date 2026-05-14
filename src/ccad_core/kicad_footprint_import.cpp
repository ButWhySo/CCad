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
    }
  }

  return pad;
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
    }
  }

  return footprint;
}

std::string dumpFootprintJson(const Footprint& footprint) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"name\": \"" << escapeJson(footprint.name) << "\",\n";
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
    out << "      \"layers\": ";
    writeStringArray(out, 6, pad.layers);
    out << "\n";
    out << "    }" << (i + 1 == footprint.pads.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

}  // namespace ccad
