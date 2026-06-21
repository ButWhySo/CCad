"""Final comprehensive fix for review_window.cpp and test files."""
import re

def fix_review_window():
    with open('src/ccad_gui/review_window.cpp', 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    orig = content

    # Fix !project_cache_.boards[0] (was formerly !board optional) -> project_cache_.boards.empty()
    content = content.replace(
        'if (!project_cache_.boards[0])',
        'if (project_cache_.boards.empty())'
    )
    content = content.replace(
        'if (project_cache_.boards[0])',
        'if (!project_cache_.boards.empty())'
    )

    # Same for project_.boards[0] used as bool
    content = content.replace(
        'if (!project_.boards[0])',
        'if (project_.boards.empty())'
    )
    content = content.replace(
        'if (project_.boards[0])',
        'if (!project_.boards.empty())'
    )

    # Fix buildCanvasScene(project_) -> properly guarded
    content = content.replace(
        'buildCanvasScene(project_)',
        'project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])'
    )
    content = content.replace(
        'buildCanvasScene(project)',
        'project.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project.boards[0])'
    )
    content = content.replace(
        'ccad::buildCanvasScene(project_.boards[0])',
        'project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])'
    )
    content = content.replace(
        'ccad::buildCanvasScene(project.boards[0])',
        'project.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project.boards[0])'
    )
    # Fix double-guarding
    content = content.replace(
        'project_.boards.empty() ? ccad::CanvasScene{} : project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])',
        'project_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_.boards[0])'
    )

    # Fix buildSchematicScene(project_) / buildSchematicScene(project)
    content = content.replace(
        'buildSchematicScene(project_)',
        'project_.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project_.schematics[0])'
    )
    content = content.replace(
        'buildSchematicScene(project)',
        'project.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project.schematics[0])'
    )
    content = content.replace(
        'ccad::buildSchematicScene(project_)',
        'project_.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project_.schematics[0])'
    )

    # Fix the invalid Schematic ref from project - line 3441-3442 area
    # buildSchematicScene takes Schematic&, not Project&
    content = content.replace(
        'const ccad::CanvasScene scene = ccad::buildSchematicScene(project_)',
        'const ccad::CanvasScene scene = project_.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project_.schematics[0])'
    )
    content = content.replace(
        'const ccad::CanvasScene scene = ccad::buildSchematicScene(project)',
        'const ccad::CanvasScene scene = project.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project.schematics[0])'
    )

    if orig != content:
        with open('src/ccad_gui/review_window.cpp', 'w', encoding='utf-8') as f:
            f.write(content)
        print('review_window.cpp fixed')
    else:
        print('review_window.cpp: no changes needed')


def fix_test_diff():
    with open('tests/test_diff.cpp', 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    orig = content
    
    # Fix project.constraints -> project.schematics[0].constraints
    content = content.replace('project.constraints', 'project.schematics[0].constraints')

    # Fix .reset() on boards[0] (was optional, now it's a Board in a vector)
    # boards[0].reset() should become boards.clear() or boards.pop_back()
    # Let's find lines with .boards[0].reset()
    content = re.sub(r'(\w+)\.boards\[0\]\.reset\(\)', r'\1.boards.clear()', content)
    content = re.sub(r'(\w+)\.board\.reset\(\)', r'\1.boards.clear()', content)

    if orig != content:
        with open('tests/test_diff.cpp', 'w', encoding='utf-8') as f:
            f.write(content)
        print('test_diff.cpp fixed')


def fix_test_canvas():
    with open('tests/test_canvas.cpp', 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    orig = content

    # Fix project.board = Board{ ... to push_back
    content = content.replace(
        '  project.board = ccad::Board{',
        '  project.boards.push_back(ccad::Board{'
    )
    # Fix the closing of the pushed board
    content = re.sub(
        r'  \};\n  (const ccad::CanvasScene)',
        r'  });\n  \1',
        content
    )

    # Fix buildCanvasScene(project) -> guarded
    content = content.replace(
        'ccad::buildCanvasScene(project)',
        'project.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project.boards[0])'
    )
    content = content.replace(
        'buildCanvasScene(project)',
        'project.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project.boards[0])'
    )

    if orig != content:
        with open('tests/test_canvas.cpp', 'w', encoding='utf-8') as f:
            f.write(content)
        print('test_canvas.cpp fixed')


fix_review_window()
fix_test_diff()
fix_test_canvas()
print('Done.')
