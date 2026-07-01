import os
import glob

def replace_in_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original = content
    content = content.replace("ccad::Component", "ccad::SchSymbol")
    content = content.replace("std::vector<Component>", "std::vector<SchSymbol>")
    content = content.replace("Component{", "SchSymbol{")
    content = content.replace("components", "symbols")
    content = content.replace("WireSegment", "SchWire")
    content = content.replace("BusSegment", "SchBus")
    content = content.replace("Label{", "SchLabel{")
    content = content.replace("PowerSymbol{", "SchPowerSymbol{")
    content = content.replace("ccad::Label", "ccad::SchLabel")
    content = content.replace("ccad::PowerSymbol", "ccad::SchPowerSymbol")
    content = content.replace("std::vector<Label>", "std::vector<SchLabel>")
    content = content.replace("std::vector<PowerSymbol>", "std::vector<SchPowerSymbol>")
    content = content.replace("std::vector<WireSegment>", "std::vector<SchWire>")

    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Updated {filepath}")

for d in ['src/ccad_core/*.cpp', 'src/ccad_core/*.hpp', 'src/ccad_cli/*.cpp', 'src/ccad_cli/*.hpp', 'tests/*.cpp', 'tests/*.hpp']:
    for f in glob.glob(d):
        replace_in_file(f)
