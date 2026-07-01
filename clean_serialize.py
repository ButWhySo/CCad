import sys

with open("src/ccad_core/serialize.cpp", "r") as f:
    content = f.read()

# Replace struct names in serialize.cpp
content = content.replace("std::vector<Component> components;", "std::vector<SchSymbol> symbols;")
content = content.replace("std::vector<Component> readComponents", "std::vector<SchSymbol> readComponents")
content = content.replace("Component component;", "SchSymbol component;")
content = content.replace("const Component& component", "const SchSymbol& component")

content = content.replace("std::vector<Pin> pins;", "std::vector<SchPin> pins;")
content = content.replace("std::vector<Pin> readPins", "std::vector<SchPin> readPins")
content = content.replace("Pin pin;", "SchPin pin;")
content = content.replace("const Pin& pin", "const SchPin& pin")

content = content.replace("std::vector<WireSegment> wires;", "std::vector<SchWire> wires;")
content = content.replace("std::vector<WireSegment> readWireSegments", "std::vector<SchWire> readWireSegments")
content = content.replace("WireSegment wire;", "SchWire wire;")
content = content.replace("const WireSegment& wire", "const SchWire& wire")

content = content.replace("std::vector<BusSegment> buses;", "std::vector<SchBus> buses;")
content = content.replace("std::vector<BusSegment> readBusSegments", "std::vector<SchBus> readBusSegments")
content = content.replace("BusSegment bus;", "SchBus bus;")
content = content.replace("const BusSegment& bus", "const SchBus& bus")

content = content.replace("std::vector<Label> labels;", "std::vector<SchLabel> labels;")
content = content.replace("std::vector<Label> readLabels", "std::vector<SchLabel> readLabels")
content = content.replace("Label label;", "SchLabel label;")
content = content.replace("const Label& label", "const SchLabel& label")

content = content.replace("std::vector<PowerSymbol> power_symbols;", "std::vector<SchPowerSymbol> power_symbols;")
content = content.replace("std::vector<PowerSymbol> readPowerSymbols", "std::vector<SchPowerSymbol> readPowerSymbols")
content = content.replace("PowerSymbol symbol;", "SchPowerSymbol symbol;")
content = content.replace("const PowerSymbol& symbol", "const SchPowerSymbol& symbol")

# Update field names in Schematic access
content = content.replace("ensureSchematic(project).components", "ensureSchematic(project).symbols")
content = content.replace("sch->components", "sch->symbols")

# Field rename: component.part -> component.lib_id
content = content.replace("component.part", "component.lib_id")
content = content.replace("key == \"part\"", "key == \"lib_id\"")

# pin kind -> pin type
content = content.replace("pin.kind", "pin.type")
content = content.replace('key == "kind"', 'key == "type"')
content = content.replace('writeField(out, 10, "kind", pin.type);', 'writeField(out, 10, "type", pin.type);')

# label global -> label type
content = content.replace("label.global = readBool();", "label.type = readBool() ? LabelType::Global : LabelType::Local;")
content = content.replace('out << "      \\"global\\": " << (label.global ? "true" : "false") << \'\\n\';', 'out << "      \\"global\\": " << (label.type == LabelType::Global ? "true" : "false") << \'\\n\';')

# Handle symbol deletion manually
# In readComponents
content = content.replace('} else if (key == "symbol") {\n          const std::string raw_symbol = readRawJsonObject();\n          component.symbol = SymbolJsonReader(raw_symbol).readSymbol();\n        ', '')

# In dumpProjectJson
content = content.replace('if (component.symbol.has_value()) {\n      out << ",\\n";\n      out << "      \\"symbol\\": ";\n      writeSymbolJson(out, 0, *component.symbol);\n      out << \'\\n\';\n    } else {\n      out << \'\\n\';\n    }', 'out << \'\\n\';')

with open("src/ccad_core/serialize.cpp", "w") as f:
    f.write(content)

print("Updated serialize.cpp")
