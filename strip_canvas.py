import sys

with open('src/ccad_core/canvas.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
skip = False
for line in lines:
    if "CanvasPoint transformSymbolPoint(" in line:
        skip = True
    elif "void mergePlacedSymbolScene(" in line:
        skip = True
    elif skip and line.strip() == "}":
        skip = False
        continue
    
    if not skip:
        new_lines.append(line)

with open('src/ccad_core/canvas.cpp', 'w') as f:
    f.writelines(new_lines)
print("Stripped dead functions.")
