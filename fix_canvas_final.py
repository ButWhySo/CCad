import re

with open('src/ccad_core/canvas.cpp', 'r') as f:
    c = f.read()

# Add includeBounds
if "void includeBounds(" not in c:
    include_bounds = """
void includeBounds(double& min_x, double& min_y, double& max_x, double& max_y,
                   const double x_units, const double y_units) {
  if (x_units < min_x) min_x = x_units;
  if (y_units < min_y) min_y = y_units;
  if (x_units > max_x) max_x = x_units;
  if (y_units > max_y) max_y = y_units;
}
"""
    c = c.replace("}  // namespace", include_bounds + "\n}  // namespace")

# Fix usages
c = re.sub(r'\bComponent\b', 'SchSymbol', c)
c = re.sub(r'\bWireSegment\b', 'SchWire', c)
c = re.sub(r'\bBusSegment\b', 'SchBus', c)
c = re.sub(r'\bLabel\b', 'SchLabel', c)
c = re.sub(r'\bPowerSymbol\b', 'SchPowerSymbol', c)

# Revert accidental double prefixes
c = c.replace("SchSchSymbol", "SchSymbol")
c = c.replace("SchSchWire", "SchWire")
c = c.replace("SchSchBus", "SchBus")
c = c.replace("SchSchLabel", "SchLabel")
c = c.replace("SchSchPowerSymbol", "SchPowerSymbol")

# Properties
c = c.replace("schematic.components", "schematic.symbols")
c = c.replace("comp.part", "comp.lib_id")
c = c.replace("comp.symbol.has_value()", "false")
c = c.replace("label.global", "(label.type == LabelType::Global)")

with open('src/ccad_core/canvas.cpp', 'w') as f:
    f.write(c)
print("Canvas fixed.")
