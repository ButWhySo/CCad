"""Mass fix all remaining project.board and project.constraints references in all source files."""
import re
import glob

def fix_file(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    orig = content

    # Fix project.board = ... (assignment) - already handled
    # Fix project.board. (member access)
    content = re.sub(r'(?<!\w)project_?\.board\.value\(\)', 'project_.boards.empty() ? ccad::Board{} : project_.boards[0]', content)
    content = re.sub(r'(?<!\w)project\.board\.value\(\)', 'project.boards.empty() ? ccad::Board{} : project.boards[0]', content)
    content = re.sub(r'(?<!\w)project_?\.board\.has_value\(\)', '!project_.boards.empty()', content)
    content = re.sub(r'(?<!\w)project\.board\.has_value\(\)', '!project.boards.empty()', content)

    # Fix project.board -> project.boards[0] (direct member access)
    # Only when followed by a . or [ but not s (to avoid changing "boards")
    content = re.sub(r'\bproject_?\.board(?!s)(?=[\.\[])', lambda m: m.group(0).replace('.board', '.boards[0]').replace('project_.boards[0]', 'project_.boards[0]' if not 'project_.' in m.group(0) else 'project_.boards[0]'), content)
    
    # Simpler pattern - direct replacement
    content = content.replace('project_.board.', 'project_.boards.empty() ? ccad::Board{} . : project_.boards[0].')
    # That's too complex. Use simple direct approach.
    
    # Reset and use a simpler, more reliable approach
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    
    # Split into lines and fix each line
    lines = content.split('\n')
    new_lines = []
    for line in lines:
        # Fix .board. -> .boards[0]. (but not .boards. -> .boards[0]s.)
        if '.board.' in line and '.boards.' not in line and '.boards[' not in line:
            line = re.sub(r'\.board\.', '.boards[0].', line)
        if '.board->' in line and '.boards->' not in line:
            line = re.sub(r'\.board->', '.boards[0].', line)
        if '.board,' in line and '.boards,' not in line:
            line = re.sub(r'\.board,', '.boards[0],', line)
        if '.board)' in line and '.boards)' not in line:
            line = re.sub(r'\.board\)', '.boards[0])', line)
        if '.board ' in line and '.boards ' not in line:
            line = re.sub(r'\.board ', '.boards[0] ', line)
        if '.board;' in line and '.boards;' not in line:
            line = re.sub(r'\.board;', '.boards[0];', line)
        # Fix .board = (assignment) - should be push_back
        # But this is complex - we need context. Skip for now.
        
        # Fix project.constraints -> project.schematics[0].constraints
        if 'project.constraints' in line:
            line = line.replace('project.constraints', 'project.schematics.empty() ? ccad::Schematic::Constraints{} : project.schematics[0].constraints')
        
        new_lines.append(line)
    content = '\n'.join(new_lines)

    if orig != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    return False

files_to_fix = (
    glob.glob('src/ccad_gui/*.cpp') +
    glob.glob('src/ccad_gui/*.hpp') +
    glob.glob('src/ccad_core/*.cpp') +
    glob.glob('src/ccad_core/*.hpp') +
    glob.glob('src/ccad_cli/*.cpp') +
    glob.glob('src/ccad_cli/*.hpp') +
    glob.glob('tests/*.cpp')
)

fixed = []
for path in files_to_fix:
    if fix_file(path):
        fixed.append(path)

print(f'Fixed {len(fixed)} files:')
for f in fixed:
    print(' ', f)
