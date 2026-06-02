#include "kicad_symbol_import.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/sexpr_parser.hpp"
#include "ccad_core/symbol_json_reader.hpp"

#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace ccad {

namespace {

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open symbol file: " + path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

Point parseAt(const SExpr* at_expr) {
  if (!at_expr || at_expr->children.size() < 3) return {0, 0};
  double x = 0.0;
  double y = 0.0;
  try { x = std::stod(at_expr->children[1]->value); } catch (...) {}
  try { y = std::stod(at_expr->children[2]->value); } catch (...) {}
  // Convert mm to nm
  return {static_cast<int>(x * 1000000.0), static_cast<int>(y * 1000000.0)};
}

double parseRotation(const SExpr* at_expr) {
  if (!at_expr || at_expr->children.size() < 4) return 0.0;
  try { return std::stod(at_expr->children[3]->value); } catch (...) {}
  return 0.0;
}

void parseSymbolRecursive(const SExpr* expr, Symbol& out_sym, const std::string& parent_name) {
  (void)parent_name;
  for (const auto& child : expr->children) {
    if (child->is_list && !child->children.empty()) {
      const std::string& type = child->children[0]->value;
      
      if (type == "property" && child->children.size() >= 3) {
        SymbolProperty prop;
        prop.name = child->children[1]->value;
        prop.value = child->children[2]->value;
        if (const SExpr* at = findSExprChild(child.get(), "at")) {
          prop.position = parseAt(at);
          prop.rotation_degrees = parseRotation(at);
        }
        if (const SExpr* effects = findSExprChild(child.get(), "effects")) {
          if (findSExprChild(effects, "hide")) {
            prop.visible = false;
          }
        }
        out_sym.properties.push_back(prop);
      } else if (type == "pin" && child->children.size() >= 3) {
        SymbolPin pin;
        pin.electrical_type = child->children[1]->value;
        pin.graphical_style = child->children[2]->value;
        if (const SExpr* at = findSExprChild(child.get(), "at")) {
          pin.position = parseAt(at);
          pin.rotation_degrees = parseRotation(at);
        }
        if (const SExpr* length = findSExprChild(child.get(), "length")) {
          if (length->children.size() > 1) {
            try { pin.length = millimeters(std::stod(length->children[1]->value)); } catch(...) {}
          }
        }
        if (const SExpr* name = findSExprChild(child.get(), "name")) {
          if (name->children.size() > 1) pin.name = name->children[1]->value;
        }
        if (const SExpr* number = findSExprChild(child.get(), "number")) {
          if (number->children.size() > 1) pin.number = number->children[1]->value;
        }
        out_sym.pins.push_back(pin);
      } else if (type == "rectangle" && child->children.size() >= 3) {
        SymbolRectangle rect;
        if (const SExpr* start = findSExprChild(child.get(), "start")) {
          rect.start = parseAt(start); // 'start' structure is same as 'at' without rotation
        }
        if (const SExpr* end = findSExprChild(child.get(), "end")) {
          rect.end = parseAt(end);
        }
        if (const SExpr* stroke = findSExprChild(child.get(), "stroke")) {
          if (const SExpr* width = findSExprChild(stroke, "width")) {
            if (width->children.size() > 1) {
              try { rect.stroke_width = millimeters(std::stod(width->children[1]->value)); } catch(...) {}
            }
          }
        }
        if (const SExpr* fill = findSExprChild(child.get(), "fill")) {
          if (const SExpr* fill_type = findSExprChild(fill, "type")) {
             if (fill_type->children.size() > 1) rect.fill_type = fill_type->children[1]->value;
          }
        }
        out_sym.rectangles.push_back(rect);
      } else if (type == "line" && child->children.size() >= 2) {
        SymbolLine line;
        if (const SExpr* pts = findSExprChild(child.get(), "pts")) {
          if (pts->children.size() >= 3) {
            line.start = parseAt(pts->children[1].get()); // xy is essentially 'at'
            line.end = parseAt(pts->children[2].get());
          }
        }
        if (const SExpr* stroke = findSExprChild(child.get(), "stroke")) {
          if (const SExpr* width = findSExprChild(stroke, "width")) {
            if (width->children.size() > 1) {
              try { line.stroke_width = millimeters(std::stod(width->children[1]->value)); } catch(...) {}
            }
          }
        }
        out_sym.lines.push_back(line);
      } else if (type == "arc" && child->children.size() >= 2) {
        SymbolArc arc;
        if (const SExpr* start = findSExprChild(child.get(), "start")) arc.start = parseAt(start);
        if (const SExpr* end = findSExprChild(child.get(), "end")) arc.end = parseAt(end);
        if (const SExpr* center = findSExprChild(child.get(), "center")) arc.center = parseAt(center);
        if (const SExpr* stroke = findSExprChild(child.get(), "stroke")) {
          if (const SExpr* width = findSExprChild(stroke, "width")) {
            if (width->children.size() > 1) {
              try { arc.stroke_width = millimeters(std::stod(width->children[1]->value)); } catch(...) {}
            }
          }
        }
        out_sym.arcs.push_back(arc);
      } else if (type == "circle" && child->children.size() >= 2) {
        SymbolCircle circle;
        if (const SExpr* center = findSExprChild(child.get(), "center")) circle.center = parseAt(center);
        if (const SExpr* radius = findSExprChild(child.get(), "radius")) {
          if (radius->children.size() > 1) {
            try { circle.radius = millimeters(std::stod(radius->children[1]->value)); } catch(...) {}
          }
        }
        if (const SExpr* stroke = findSExprChild(child.get(), "stroke")) {
          if (const SExpr* width = findSExprChild(stroke, "width")) {
            if (width->children.size() > 1) {
              try { circle.stroke_width = millimeters(std::stod(width->children[1]->value)); } catch(...) {}
            }
          }
        }
        if (const SExpr* fill = findSExprChild(child.get(), "fill")) {
          if (const SExpr* fill_type = findSExprChild(fill, "type")) {
             if (fill_type->children.size() > 1) circle.fill_type = fill_type->children[1]->value;
          }
        }
        out_sym.circles.push_back(circle);
      } else if (type == "polyline" && child->children.size() >= 2) {
        SymbolPolyline polyline;
        if (const SExpr* pts = findSExprChild(child.get(), "pts")) {
          for (std::size_t i = 1; i < pts->children.size(); ++i) {
             polyline.points.push_back(parseAt(pts->children[i].get()));
          }
        }
        if (const SExpr* stroke = findSExprChild(child.get(), "stroke")) {
          if (const SExpr* width = findSExprChild(stroke, "width")) {
            if (width->children.size() > 1) {
              try { polyline.stroke_width = millimeters(std::stod(width->children[1]->value)); } catch(...) {}
            }
          }
        }
        if (const SExpr* fill = findSExprChild(child.get(), "fill")) {
          if (const SExpr* fill_type = findSExprChild(fill, "type")) {
             if (fill_type->children.size() > 1) polyline.fill_type = fill_type->children[1]->value;
          }
        }
        out_sym.polylines.push_back(polyline);
      } else if (type == "text" && child->children.size() >= 3) {
        SymbolText text;
        text.text = child->children[1]->value;
        if (const SExpr* at = findSExprChild(child.get(), "at")) {
          text.position = parseAt(at);
          text.rotation_degrees = parseRotation(at);
        }
        if (const SExpr* effects = findSExprChild(child.get(), "effects")) {
          if (const SExpr* font = findSExprChild(effects, "font")) {
            if (const SExpr* size = findSExprChild(font, "size")) {
              if (size->children.size() > 1) {
                 try { text.size = millimeters(std::stod(size->children[1]->value)); } catch(...) {}
              }
            }
          }
        }
        out_sym.texts.push_back(text);
      } else if (type == "symbol" && child->children.size() >= 2) {
        // Recursive symbol definition (e.g. R_0_1, R_1_1 inside R)
        parseSymbolRecursive(child.get(), out_sym, child->children[1]->value);
      } else if (type == "extends" && child->children.size() >= 2) {
        out_sym.extends = child->children[1]->value;
      }
    }
  }
}

} // namespace

std::vector<Symbol> importKiCadSymbolLibrary(const std::string& kicad_sym_content) {
  std::vector<Symbol> result;
  std::unique_ptr<SExpr> root = parseSExpr(kicad_sym_content);
  if (!root || !root->is_list || root->children.empty() || root->children[0]->value != "kicad_symbol_lib") {
    throw std::runtime_error("Invalid KiCad symbol library file.");
  }
  
  for (const auto& child : root->children) {
    if (child->is_list && !child->children.empty() && child->children[0]->value == "symbol" && child->children.size() >= 2) {
      Symbol sym;
      sym.name = child->children[1]->value;
      parseSymbolRecursive(child.get(), sym, sym.name);
      result.push_back(sym);
    }
  }
  
  return result;
}

Symbol loadSymbolJson(const std::string_view source) {
  return SymbolJsonReader(source).readSymbol();
}

Symbol loadSymbolJsonFileWithLocalInheritance(const std::filesystem::path& path) {
  Symbol symbol = loadSymbolJson(readTextFile(path));
  if (symbol.extends.empty()) {
    return symbol;
  }

  const std::filesystem::path parent_path = path.parent_path() / (symbol.extends + ".json");
  if (!std::filesystem::exists(parent_path)) {
    return symbol;
  }

  Symbol parent = loadSymbolJsonFileWithLocalInheritance(parent_path);
  if (symbol.pins.empty()) {
    symbol.pins = parent.pins;
  }
  if (symbol.rectangles.empty()) {
    symbol.rectangles = parent.rectangles;
  }
  if (symbol.lines.empty()) {
    symbol.lines = parent.lines;
  }
  if (symbol.arcs.empty()) {
    symbol.arcs = parent.arcs;
  }
  if (symbol.circles.empty()) {
    symbol.circles = parent.circles;
  }
  if (symbol.polylines.empty()) {
    symbol.polylines = parent.polylines;
  }
  if (symbol.texts.empty()) {
    symbol.texts = parent.texts;
  }
  return symbol;
}

std::string dumpSymbolsJson(const std::vector<Symbol>& symbols) {
  std::ostringstream out;
  out << "[\n";
  for (std::size_t i = 0; i < symbols.size(); ++i) {
    const auto& sym = symbols[i];
    out << "  {\n";
    out << "    \"name\": \"" << sym.name << "\",\n";
    out << "    \"extends\": \"" << sym.extends << "\",\n";
    
    out << "    \"pins\": [\n";
    for (std::size_t p = 0; p < sym.pins.size(); ++p) {
      const auto& pin = sym.pins[p];
      out << "      {\n";
      out << "        \"name\": \"" << pin.name << "\",\n";
      out << "        \"number\": \"" << pin.number << "\",\n";
      out << "        \"electrical_type\": \"" << pin.electrical_type << "\",\n";
      out << "        \"x_nm\": " << pin.position.x.nanometers << ",\n";
      out << "        \"y_nm\": " << pin.position.y.nanometers << ",\n";
      out << "        \"rotation_degrees\": " << pin.rotation_degrees << "\n";
      out << "      }" << (p + 1 == sym.pins.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";

    out << "    \"properties\": [\n";
    for (std::size_t p = 0; p < sym.properties.size(); ++p) {
      const auto& prop = sym.properties[p];
      out << "      {\n";
      out << "        \"name\": \"" << prop.name << "\",\n";
      out << "        \"value\": \"" << escapeJson(prop.value) << "\"\n";
      out << "      }" << (p + 1 == sym.properties.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";
    
    out << "    \"rectangles\": [\n";
    for (std::size_t p = 0; p < sym.rectangles.size(); ++p) {
      const auto& rect = sym.rectangles[p];
      out << "      {\n";
      out << "        \"start_x\": " << rect.start.x.nanometers << ",\n";
      out << "        \"start_y\": " << rect.start.y.nanometers << ",\n";
      out << "        \"end_x\": " << rect.end.x.nanometers << ",\n";
      out << "        \"end_y\": " << rect.end.y.nanometers << ",\n";
      out << "        \"stroke_width\": " << rect.stroke_width.nanometers << ",\n";
      out << "        \"fill_type\": \"" << escapeJson(rect.fill_type) << "\"\n";
      out << "      }" << (p + 1 == sym.rectangles.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";
    out << "    \"lines\": [\n";
    for (std::size_t p = 0; p < sym.lines.size(); ++p) {
      const auto& line = sym.lines[p];
      out << "      {\n";
      out << "        \"start_x\": " << line.start.x.nanometers << ",\n";
      out << "        \"start_y\": " << line.start.y.nanometers << ",\n";
      out << "        \"end_x\": " << line.end.x.nanometers << ",\n";
      out << "        \"end_y\": " << line.end.y.nanometers << ",\n";
      out << "        \"stroke_width\": " << line.stroke_width.nanometers << "\n";
      out << "      }" << (p + 1 == sym.lines.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";
    out << "    \"arcs\": [\n";
    for (std::size_t p = 0; p < sym.arcs.size(); ++p) {
      const auto& arc = sym.arcs[p];
      out << "      {\n";
      out << "        \"start_x\": " << arc.start.x.nanometers << ",\n";
      out << "        \"start_y\": " << arc.start.y.nanometers << ",\n";
      out << "        \"end_x\": " << arc.end.x.nanometers << ",\n";
      out << "        \"end_y\": " << arc.end.y.nanometers << ",\n";
      out << "        \"center_x\": " << arc.center.x.nanometers << ",\n";
      out << "        \"center_y\": " << arc.center.y.nanometers << ",\n";
      out << "        \"stroke_width\": " << arc.stroke_width.nanometers << "\n";
      out << "      }" << (p + 1 == sym.arcs.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";
    out << "    \"circles\": [\n";
    for (std::size_t p = 0; p < sym.circles.size(); ++p) {
      const auto& circle = sym.circles[p];
      out << "      {\n";
      out << "        \"center_x\": " << circle.center.x.nanometers << ",\n";
      out << "        \"center_y\": " << circle.center.y.nanometers << ",\n";
      out << "        \"radius\": " << circle.radius.nanometers << ",\n";
      out << "        \"stroke_width\": " << circle.stroke_width.nanometers << ",\n";
      out << "        \"fill_type\": \"" << escapeJson(circle.fill_type) << "\"\n";
      out << "      }" << (p + 1 == sym.circles.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";
    out << "    \"polylines\": [\n";
    for (std::size_t p = 0; p < sym.polylines.size(); ++p) {
      const auto& poly = sym.polylines[p];
      out << "      {\n";
      out << "        \"points\": [\n";
      for (std::size_t j = 0; j < poly.points.size(); ++j) {
        out << "          {\"x\": " << poly.points[j].x.nanometers << ", \"y\": " << poly.points[j].y.nanometers << "}" << (j + 1 == poly.points.size() ? "" : ",") << "\n";
      }
      out << "        ],\n";
      out << "        \"stroke_width\": " << poly.stroke_width.nanometers << ",\n";
      out << "        \"fill_type\": \"" << escapeJson(poly.fill_type) << "\"\n";
      out << "      }" << (p + 1 == sym.polylines.size() ? "" : ",") << "\n";
    }
    out << "    ],\n";
    out << "    \"texts\": [\n";
    for (std::size_t p = 0; p < sym.texts.size(); ++p) {
      const auto& text = sym.texts[p];
      out << "      {\n";
      out << "        \"text\": \"" << escapeJson(text.text) << "\",\n";
      out << "        \"x_nm\": " << text.position.x.nanometers << ",\n";
      out << "        \"y_nm\": " << text.position.y.nanometers << ",\n";
      out << "        \"rotation_degrees\": " << text.rotation_degrees << ",\n";
      out << "        \"size\": " << text.size.nanometers << "\n";
      out << "      }" << (p + 1 == sym.texts.size() ? "" : ",") << "\n";
    }
    out << "    ]\n";
    
    out << "  }" << (i + 1 == symbols.size() ? "" : ",") << "\n";
  }
  out << "]\n";
  return out.str();
}

}  // namespace ccad
