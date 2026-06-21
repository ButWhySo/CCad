"""Fix test files after the Project structure migration to multi-Board/Schematic."""
import re

# ---- test_serialize.cpp ----
with open('tests/test_serialize.cpp', 'r') as f:
    content = f.read()

# Fix multi-line project.board assignment
content = content.replace(
    '  project.board = ccad::Board{',
    '  project.boards.push_back(ccad::Board{'
)
# Close the pushed board - find the closing brace/semicolon pattern
content = content.replace(
    '  };\n  project.schematics',
    '  });\n  project.schematics'
)
# Fix typo from bulk replace: loaded.boards[0]s[0]
content = content.replace('loaded.boards[0]s[0]', 'loaded.boards[0]')
# Fix loaded.constraints -> loaded.schematics[0].constraints
content = content.replace('loaded.constraints', 'loaded.schematics[0].constraints')
# Fix schema_version string in checks
content = content.replace('"schema_version": 1', '"schema_version": 2')

with open('tests/test_serialize.cpp', 'w') as f:
    f.write(content)
print('test_serialize.cpp done')

# ---- test_drc.cpp ----
with open('tests/test_drc.cpp', 'r') as f:
    content = f.read()

# Fix remaining project.board = ccad::Board{ patterns
# These are inside validBoardProject() which already has schematics[0].xxx calls
content = content.replace(
    '  project.board = ccad::Board{',
    '  project.boards.push_back(ccad::Board{'
)
# The closing of the board - it ends with .route_requests = {},\n  };\n  return project;
content = content.replace(
    '      .route_requests = {},\n  };\n  return project;',
    '      .route_requests = {},\n  });\n  return project;'
)

with open('tests/test_drc.cpp', 'w') as f:
    f.write(content)
print('test_drc.cpp done')

# ---- test_placement.cpp ----
with open('tests/test_placement.cpp', 'r') as f:
    content = f.read()

# buildSchematicScene now takes Schematic&, not Project&
# Find calls and fix them
content = re.sub(
    r'ccad::buildSchematicScene\((\w+)\)',
    lambda m: 'ccad::buildSchematicScene(' + m.group(1) + '.schematics.empty() ? ccad::Schematic{} : ' + m.group(1) + '.schematics[0])',
    content
)
# Simpler: if it's just reloaded, it's a Project; fix to pass schematics[0]
content = content.replace(
    'ccad::buildSchematicScene(reloaded.schematics.empty() ? ccad::Schematic{} : reloaded.schematics[0])',
    '(reloaded.schematics.empty() ? ccad::buildSchematicScene(ccad::Schematic{}) : ccad::buildSchematicScene(reloaded.schematics[0]))'
)

with open('tests/test_placement.cpp', 'w') as f:
    f.write(content)
print('test_placement.cpp done')

print('All test files fixed.')
