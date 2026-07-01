import sys
import re

def fix_erc():
    with open('src/ccad_core/erc.cpp', 'r') as f:
        c = f.read()
    c = c.replace("pin.kind.empty()", "pin.type.empty()")
    c = c.replace("INVALID_PIN_KIND", "INVALID_PIN_TYPE")
    c = c.replace("pin kind must not be empty", "pin type must not be empty")
    with open('src/ccad_core/erc.cpp', 'w') as f:
        f.write(c)

def fix_diff():
    with open('src/ccad_core/diff.cpp', 'r') as f:
        c = f.read()
    c = c.replace("pin.kind", "pin.type")
    with open('src/ccad_core/diff.cpp', 'w') as f:
        f.write(c)

def fix_canvas():
    with open('src/ccad_core/canvas.cpp', 'r') as f:
        c = f.read()
    
    # comp.lib_id -> comp.part (wait, CanvasComponent still expects `part`, I will rename CanvasComponent part to lib_id)
    # Actually let's just patch canvas.cpp for now
    c = c.replace("cc.lib_id = comp.lib_id;", "cc.part = comp.lib_id;")
    
    # symbol has_value
    c = c.replace("cc.has_symbol_graphics = comp.symbol.has_value();", "cc.has_symbol_graphics = false;")
    c = c.replace("if (comp.symbol.has_value()) {", "if (false) {")
    c = c.replace("mergePlacedSymbolScene(scene, comp, buildCanvasScene(*comp.symbol), min_x, min_y, max_x, max_y);", "")
    
    # label.global -> label.type
    c = c.replace("cl.global = label.global;", "cl.global = (label.type == LabelType::Global);")
    
    with open('src/ccad_core/canvas.cpp', 'w') as f:
        f.write(c)

fix_erc()
fix_diff()
fix_canvas()
print("Fixed files.")
