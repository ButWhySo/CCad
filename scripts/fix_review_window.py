"""Fix review_window.cpp for remaining issues after migration."""
import re

with open('src/ccad_gui/review_window.cpp', 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()

orig = content

# 1. Fix insertBoardObjectCounts signature - it takes optional<Board> but we're now passing boards[0]
# which won't compile when boards is empty. Change to pointer-based approach.
content = content.replace(
    'insertBoardObjectCounts(response, project.boards[0]);',
    'insertBoardObjectCounts(response, project.boards.empty() ? std::optional<ccad::Board>{} : std::optional<ccad::Board>{project.boards[0]});'
)

# 2. Fix schematics[0] access without guard in projectObjectCountsObject and similar functions
# Replace project.schematics[0]. with a guarded version
content = content.replace(
    '  response.insert("component_count", static_cast<int>(project.schematics[0].components.size()));\n  response.insert("net_count", static_cast<int>(project.schematics[0].nets.size()));\n  response.insert("wire_count", static_cast<int>(project.schematics[0].wires.size()));\n  response.insert("constraint_count", static_cast<int>(project.schematics[0].constraints.size()));',
    '  const ccad::Schematic* sch0 = project.schematics.empty() ? nullptr : &project.schematics[0];\n  response.insert("component_count", sch0 ? static_cast<int>(sch0->components.size()) : 0);\n  response.insert("net_count", sch0 ? static_cast<int>(sch0->nets.size()) : 0);\n  response.insert("wire_count", sch0 ? static_cast<int>(sch0->wires.size()) : 0);\n  response.insert("constraint_count", sch0 ? static_cast<int>(sch0->constraints.size()) : 0);'
)

# 3. Fix project.schematics[0].nets in availablePcbNetIds
content = content.replace(
    '  for (const ccad::Net& net : project.schematics[0].nets) {',
    '  if (!project.schematics.empty()) for (const ccad::Net& net : project.schematics[0].nets) {'
)

# 4. Fix project.schematics[0].components in nextComponentId
content = content.replace(
    '  for (const ccad::Component& component : project.schematics[0].components) {',
    '  if (!project.schematics.empty()) for (const ccad::Component& component : project.schematics[0].components) {'
)

# 5. Fix any remaining project.boards[0] access without guard (non-size checks)
# Only fix the specific pattern in review_window.cpp that are direct access without empty check
# Find the buildCanvasScene calls that we replaced incorrectly
content = content.replace(
    'ccad::buildCanvasScene(project_.boards[0])',
    'project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])'
)
content = content.replace(
    'ccad::buildCanvasScene(project.boards[0])',
    'project.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project.boards[0])'
)

# Fix double-guard issue from previous Python fix
content = content.replace(
    'project_.boards.empty() ? ccad::CanvasScene{} : project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])',
    'project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])'
)

# Also fix schematic scene calls
content = content.replace(
    'ccad::buildSchematicScene(project_.schematics[0])',
    'project_.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project_.schematics[0])'
)
content = content.replace(
    'ccad::buildSchematicScene(project.schematics[0])',
    'project.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project.schematics[0])'
)

with open('src/ccad_gui/review_window.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print('review_window.cpp fixed. Changed:', orig != content)
