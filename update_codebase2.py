import os
import glob
import re

def replace_in_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original = content
    # Handle bare Component
    content = re.sub(r'\bComponent\b', 'SchSymbol', content)
    # Handle bare Pin -> SchPin, but be careful not to replace something like pin_name
    content = re.sub(r'\bPin\b', 'SchPin', content)
    # Handle WireSegment
    content = re.sub(r'\bWireSegment\b', 'SchWire', content)
    # Handle BusSegment
    content = re.sub(r'\bBusSegment\b', 'SchBus', content)
    # Handle Label
    content = re.sub(r'\bLabel\b', 'SchLabel', content)
    # Handle PowerSymbol
    content = re.sub(r'\bPowerSymbol\b', 'SchPowerSymbol', content)
    
    # Revert accidental replacements if any (like ccad::SchSymbol if it became ccad::SchSchSymbol)
    content = content.replace("SchSchSymbol", "SchSymbol")
    content = content.replace("SchSchPin", "SchPin")
    content = content.replace("SchSchWire", "SchWire")
    content = content.replace("SchSchBus", "SchBus")
    content = content.replace("SchSchLabel", "SchLabel")
    content = content.replace("SchSchPowerSymbol", "SchPowerSymbol")
    
    # Fix properties access
    # Component -> SchSymbol uses lib_id instead of part
    content = re.sub(r'\.part\b', '.lib_id', content)
    # the optional symbol field was removed from SchSymbol. Wait, some code accessed .symbol.
    # We'll leave it for now and fix compiler errors.
    
    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Updated {filepath}")

for d in ['src/ccad_core/*.cpp', 'src/ccad_core/*.hpp', 'src/ccad_cli/*.cpp', 'src/ccad_cli/*.hpp', 'tests/*.cpp', 'tests/*.hpp']:
    for f in glob.glob(d):
        replace_in_file(f)
