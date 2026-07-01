import sys
import re

with open("src/ccad_core/serialize.cpp", "r") as f:
    content = f.read()

# Replace struct names in serialize.cpp
content = content.replace("std::vector<Component> components;", "std::vector<SchSymbol> components;")
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

# We will need to patch serialize.cpp to add the new schematic structures serialization.
# For now this just renames the old structures.
with open("src/ccad_core/serialize.cpp", "w") as f:
    f.write(content)

print("Updated serialize.cpp")
