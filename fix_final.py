import re

# Fix placement.cpp
with open('src/ccad_core/placement.cpp', 'r') as f:
    c = f.read()
c = c.replace(".kind =", ".type =")
with open('src/ccad_core/placement.cpp', 'w') as f:
    f.write(c)

# Fix canvas.cpp
with open('src/ccad_core/canvas.cpp', 'r') as f:
    c = f.read()
c = c.replace("WireSegment", "SchWire")
c = c.replace("BusSegment", "SchBus")
# Strip unused functions
c = re.sub(r'void mergePlacedSymbolScene.*?\}\n', '', c, flags=re.DOTALL)
with open('src/ccad_core/canvas.cpp', 'w') as f:
    f.write(c)

# Fix serialize.cpp
with open('src/ccad_core/serialize.cpp', 'r') as f:
    s = f.read()
s = s.replace("return components;", "return symbols;")
s = s.replace("components.push_back(", "symbols.push_back(")
# Remove writeSymbolJson completely
s = re.sub(r'void writeSymbolJson\(std::ostringstream& out, const int indent, const Symbol& symbol\) \{.*?\}\n\n', '', s, flags=re.DOTALL)
with open('src/ccad_core/serialize.cpp', 'w') as f:
    f.write(s)

print("Fixed final errors")
