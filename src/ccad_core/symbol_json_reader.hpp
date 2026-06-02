#pragma once

#pragma once

#include "ccad_core/kicad_symbol_import.hpp"

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ccad {

class SymbolJsonReader {
 public:
  explicit SymbolJsonReader(std::string_view source) : source_(source) {}

  Symbol readSymbol() {
    Symbol symbol;
    expect('{');
    if (consume('}')) {
      return symbol;
    }
    while (true) {
      const std::string key = readString();
      expect(':');
      if (key == "name") {
        symbol.name = readString();
      } else if (key == "extends") {
        symbol.extends = readString();
      } else if (key == "pins") {
        symbol.pins = readPins();
      } else if (key == "properties") {
        symbol.properties = readProperties();
      } else if (key == "rectangles") {
        symbol.rectangles = readRectangles();
      } else if (key == "lines") {
        symbol.lines = readLines();
      } else if (key == "arcs") {
        symbol.arcs = readArcs();
      } else if (key == "circles") {
        symbol.circles = readCircles();
      } else if (key == "polylines") {
        symbol.polylines = readPolylines();
      } else if (key == "texts") {
        symbol.texts = readTexts();
      } else {
        throw std::runtime_error("unknown symbol json key: " + key);
      }
      if (consume('}')) {
        finish();
        return symbol;
      }
      expect(',');
    }
  }

 private:
  std::vector<SymbolPin> readPins() {
    std::vector<SymbolPin> pins;
    expect('[');
    if (consume(']')) return pins;
    while (true) {
      SymbolPin pin;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "name") pin.name = readString();
          else if (key == "number") pin.number = readString();
          else if (key == "electrical_type") pin.electrical_type = readString();
          else if (key == "x_nm") pin.position.x = nanometers(readInt64());
          else if (key == "y_nm") pin.position.y = nanometers(readInt64());
          else if (key == "rotation_degrees") pin.rotation_degrees = readNumber();
          else throw std::runtime_error("unknown pin key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      pins.push_back(pin);
      if (consume(']')) break;
      expect(',');
    }
    return pins;
  }

  std::vector<SymbolProperty> readProperties() {
    std::vector<SymbolProperty> props;
    expect('[');
    if (consume(']')) return props;
    while (true) {
      SymbolProperty prop;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "name") prop.name = readString();
          else if (key == "value") prop.value = readString();
          else throw std::runtime_error("unknown property key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      props.push_back(prop);
      if (consume(']')) break;
      expect(',');
    }
    return props;
  }

  std::vector<SymbolRectangle> readRectangles() {
    std::vector<SymbolRectangle> rects;
    expect('[');
    if (consume(']')) return rects;
    while (true) {
      SymbolRectangle rect;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "start_x") rect.start.x = nanometers(readInt64());
          else if (key == "start_y") rect.start.y = nanometers(readInt64());
          else if (key == "end_x") rect.end.x = nanometers(readInt64());
          else if (key == "end_y") rect.end.y = nanometers(readInt64());
          else if (key == "stroke_width") rect.stroke_width = nanometers(readInt64());
          else if (key == "fill_type") rect.fill_type = readString();
          else throw std::runtime_error("unknown rectangle key: " + key);
          if (consume('}')) break;
          expect(',');
        }
      }
      rects.push_back(rect);
      if (consume(']')) break;
      expect(',');
    }
    return rects;
  }

  std::vector<SymbolLine> readLines() {
    std::vector<SymbolLine> lines;
    expect('[');
    if (consume(']')) return lines;
    while (true) {
      SymbolLine line;
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

  std::vector<SymbolArc> readArcs() {
    std::vector<SymbolArc> arcs;
    expect('[');
    if (consume(']')) return arcs;
    while (true) {
      SymbolArc arc;
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

  std::vector<SymbolCircle> readCircles() {
    std::vector<SymbolCircle> circles;
    expect('[');
    if (consume(']')) return circles;
    while (true) {
      SymbolCircle circle;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "center_x") circle.center.x = nanometers(readInt64());
          else if (key == "center_y") circle.center.y = nanometers(readInt64());
          else if (key == "radius") circle.radius = nanometers(readInt64());
          else if (key == "stroke_width") circle.stroke_width = nanometers(readInt64());
          else if (key == "fill_type") circle.fill_type = readString();
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

  std::vector<SymbolPolyline> readPolylines() {
    std::vector<SymbolPolyline> polys;
    expect('[');
    if (consume(']')) return polys;
    while (true) {
      SymbolPolyline poly;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "points") {
            expect('[');
            if (!consume(']')) {
              while (true) {
                expect('{');
                Point pt;
                if (!consume('}')) {
                  while (true) {
                    std::string ptkey = readString();
                    expect(':');
                    if (ptkey == "x") pt.x = nanometers(readInt64());
                    else if (ptkey == "y") pt.y = nanometers(readInt64());
                    else throw std::runtime_error("unknown point key");
                    if (consume('}')) break;
                    expect(',');
                  }
                }
                poly.points.push_back(pt);
                if (consume(']')) break;
                expect(',');
              }
            }
          } else if (key == "stroke_width") {
            poly.stroke_width = nanometers(readInt64());
          } else if (key == "fill_type") {
            poly.fill_type = readString();
          } else {
            throw std::runtime_error("unknown polyline key: " + key);
          }
          if (consume('}')) break;
          expect(',');
        }
      }
      polys.push_back(poly);
      if (consume(']')) break;
      expect(',');
    }
    return polys;
  }

  std::vector<SymbolText> readTexts() {
    std::vector<SymbolText> texts;
    expect('[');
    if (consume(']')) return texts;
    while (true) {
      SymbolText text;
      expect('{');
      if (!consume('}')) {
        while (true) {
          const std::string key = readString();
          expect(':');
          if (key == "text") text.text = readString();
          else if (key == "x_nm") text.position.x = nanometers(readInt64());
          else if (key == "y_nm") text.position.y = nanometers(readInt64());
          else if (key == "rotation_degrees") text.rotation_degrees = readNumber();
          else if (key == "size") text.size = nanometers(readInt64());
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
    std::size_t parsed = 0;
    std::string s = std::string(source_.substr(start, pos_ - start));
    const double number = std::stod(s, &parsed);
    if (parsed != s.size()) {
      throw std::runtime_error("invalid json number");
    }
    return number;
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
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

}  // namespace ccad
