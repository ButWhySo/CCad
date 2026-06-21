"""Fix serialize.cpp dumpProjectJson to use 'sch' pointer for all schematics[0] accesses."""

with open('src/ccad_core/serialize.cpp', 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()

orig = content

# The 'sch' pointer is already added at this point. Now replace project.schematics[0].X with sch->X
# in the dumpProjectJson function (after line 1686 or so).
# We use a targeted section replacement approach.

old = """  out << "  \\"components\\": [\\n";
  const Schematic* sch = project.schematics.empty() ? nullptr : &project.schematics[0];
  if (sch) {
  for (std::size_t i = 0; i < sch->components.size(); ++i) {
    const Component& component = sch->components.at(i);"""

# check if already partially done
if 'const Schematic* sch' in content:
    print('sch pointer already present')
    # Still need to fix remaining project.schematics[0] references after the pointer
    import re
    # Replace all remaining project.schematics[0]. with sch-> in the dumpProjectJson scope
    # We do this conservatively: only after the sch line
    idx = content.find('const Schematic* sch = ')
    if idx >= 0:
        before = content[:idx]
        after = content[idx:]
        after = after.replace('project.schematics[0].', 'sch->').replace('project.schematics[0]->', 'sch->')
        content = before + after
    print('Fixed schematics[0] -> sch->')
else:
    print('ERROR: sch pointer not found')

# Also guard the loops: add } // end if(sch) before each closing ],
# Find areas like:
#   }
#   out << "  ],\n";
#   out << "  \"constraints\": [...]";
# and add closing brackets

# Instead of complex parsing, just guard each section individually
# by adding if(sch) at the start of each loop and } after it

# Fix the sch-> references which might now be like sch->(null)->... if sch is null
# The for loops just won't execute if the size() is 0, but dereferencing sch-> when null is UB.
# We need to guard each loop.

# Find and wrap each loop block
import re

def guard_loop_block(c, array_name, block_pattern):
    """Wrap the loop block that uses sch-><array_name> with if(sch) guard."""
    pattern = re.escape(f'  out << "  \\"{array_name}\\": [\\n";\n') + r'(  (?:if \(sch\) \{)?(?:\n|.)*?  out << "  \],\\\\n";)'
    return c

# Simpler approach: just add null guard at start of each block
# The sch is already set. If sch is null, each loop's size() would crash.
# Replace `for (std::size_t i = 0; i < sch->` with `if (sch) for (std::size_t i = 0; i < sch->`

# Actually the cleanest fix: replace all sch-> access patterns inside for loops
# The sch pointer approach with if(sch) was right, but complex. 
# Simpler: use static empty Schematic when sch is null
content = content.replace(
    'const Schematic* sch = project.schematics.empty() ? nullptr : &project.schematics[0];',
    'static const Schematic kEmptySchematic;\n  const Schematic* sch = project.schematics.empty() ? &kEmptySchematic : &project.schematics[0];'
)
# Now remove all "if (sch) {" and their matching "}" since we always have a valid pointer now
content = content.replace('\n  if (sch) {\n', '\n')
content = content.replace('\n  } // end if(sch)\n', '\n')

with open('src/ccad_core/serialize.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print('Done. Changes made:', orig != content)
