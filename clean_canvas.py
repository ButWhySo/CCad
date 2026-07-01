import sys
import re

with open("src/ccad_core/canvas.cpp", "r") as f:
    content = f.read()

content = re.sub(r'\bComponent\b', 'SchSymbol', content)
content = re.sub(r'\bLabel\b', 'SchLabel', content)
content = re.sub(r'\bPowerSymbol\b', 'SchPowerSymbol', content)
content = content.replace("SchSchSymbol", "SchSymbol")
content = content.replace("SchSchLabel", "SchLabel")
content = content.replace("SchSchPowerSymbol", "SchPowerSymbol")

content = content.replace("cc.lib_id = comp.part;", "cc.part = comp.lib_id;")
content = content.replace("cc.has_symbol_graphics = comp.symbol.has_value();", "cc.has_symbol_graphics = false;")

# Remove the merge block
content = re.sub(r'if \(comp\.symbol\.has_value\(\)\) \{\n\s*mergePlacedSymbolScene.*?\}\n', '', content, flags=re.DOTALL)

# Label global
content = content.replace("cl.global = label.global;", "cl.global = (label.type == LabelType::Global);")

# Power symbol
content = content.replace("cp.name = ps.value;", "cp.name = ps.value;")

with open("src/ccad_core/canvas.cpp", "w") as f:
    f.write(content)

print("Updated canvas.cpp")
