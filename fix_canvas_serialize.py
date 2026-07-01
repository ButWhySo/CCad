import sys

# Fix canvas.cpp
with open('src/ccad_core/canvas.cpp', 'r') as f:
    c = f.read()

c = c.replace("cc.lib_id = comp.lib_id;", "cc.part = comp.lib_id;")
c = c.replace("cc.has_symbol_graphics = comp.symbol.has_value();", "cc.has_symbol_graphics = false;")
c = c.replace("mergePlacedSymbolScene(scene, comp, buildCanvasScene(*comp.symbol), min_x, min_y, max_x,\n                             max_y);", "")
c = c.replace("if (comp.symbol.has_value()) {\n      \n    }", "")
c = c.replace("if (comp.symbol.has_value()) {", "if (false) {")
c = c.replace("cl.global = label.global;", "cl.global = (label.type == LabelType::Global);")

with open('src/ccad_core/canvas.cpp', 'w') as f:
    f.write(c)

# Fix serialize.cpp
with open('src/ccad_core/serialize.cpp', 'r') as f:
    s = f.read()

s = s.replace('component.symbol = SymbolJsonReader(raw_symbol).readSymbol();', '')
s = s.replace('} else if (key == "symbol") {\n          const std::string raw_symbol = readRawJsonObject();\n          ', '')
s = s.replace('pin.kind = readString();', 'pin.type = readString();')
s = s.replace('if (key == "kind") {', 'if (key == "type") {')

s = s.replace('else if (key == "global") label.global = readBool();', 'else if (key == "global") label.type = readBool() ? LabelType::Global : LabelType::Local;')

s = s.replace('writeField(out, 10, "kind", pin.kind);', 'writeField(out, 10, "type", pin.type);')
s = s.replace('if (component.symbol.has_value()) {\n      out << ",\\n";\n      out << "      \\"symbol\\": ";\n      writeSymbolJson(out, 0, *component.symbol);\n      out << \'\\n\';\n    } else {\n      out << \'\\n\';\n    }', 'out << \'\\n\';')

s = s.replace('out << "      \\"global\\": " << (label.global ? "true" : "false") << \'\\n\';', 'out << "      \\"global\\": " << (label.type == LabelType::Global ? "true" : "false") << \'\\n\';')

with open('src/ccad_core/serialize.cpp', 'w') as f:
    f.write(s)

print("Fixed canvas.cpp and serialize.cpp")
