import re

# Fix placement.cpp
with open('src/ccad_core/placement.cpp', 'r') as f:
    c = f.read()
c = c.replace("comp.symbol = symbol;", "")
c = c.replace("pin.kind =", "pin.type =")
with open('src/ccad_core/placement.cpp', 'w') as f:
    f.write(c)

# Fix serialize.cpp - remove writeSymbolJson
with open('src/ccad_core/serialize.cpp', 'r') as f:
    s = f.read()

s = re.sub(r'void writeSymbolJson\(std::ostringstream& out, const int indent, const Symbol& symbol\) \{.*?\}\n', '', s, flags=re.DOTALL)

with open('src/ccad_core/serialize.cpp', 'w') as f:
    f.write(s)

# Run update_codebase2.py on canvas.cpp
def fix_canvas_component():
    with open('src/ccad_core/canvas.cpp', 'r') as f:
        content = f.read()
    content = re.sub(r'\bComponent\b', 'SchSymbol', content)
    content = re.sub(r'\bLabel\b', 'SchLabel', content)
    content = re.sub(r'\bPowerSymbol\b', 'SchPowerSymbol', content)
    content = content.replace("SchSchSymbol", "SchSymbol")
    content = content.replace("SchSchLabel", "SchLabel")
    content = content.replace("SchSchPowerSymbol", "SchPowerSymbol")
    with open('src/ccad_core/canvas.cpp', 'w') as f:
        f.write(content)

fix_canvas_component()
print("Fixed remnants")
