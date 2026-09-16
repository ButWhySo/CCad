"""
verified_tick.py — Integrity-enforced tick for kicad_exploration_tasks.md

Usage:
    python verified_tick.py <relative_path>
    e.g. python verified_tick.py pcbnew\\toolbars_footprint_viewer.h

Rules:
  1. Searches ALL files in F:\CCad\scratch\*.md for the file's basename AND normalized path.
  2. ONLY marks [x] in the tracker if at least one of those matches is found.
  3. If not found — prints a BLOCK message and exits with code 1.
  4. Logs every tick attempt (pass or fail) to F:\CCad\scratch\tick_audit_log.txt
"""

import sys
import os
from pathlib import Path
from datetime import datetime

TASK_FILE   = r'F:\CCad\docs\kicad_exploration_tasks.md'
SCRATCH_DIR = Path(r'F:\CCad\scratch')
AUDIT_LOG   = Path(r'F:\CCad\scratch\tick_audit_log.txt')

def log(msg):
    timestamp = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
    line = f"[{timestamp}] {msg}\n"
    with open(AUDIT_LOG, 'a', encoding='utf-8') as f:
        f.write(line)
    print(line.strip())

def load_scratch_content():
    content = ''
    for sf in sorted(SCRATCH_DIR.glob('*.md')):
        try:
            with open(sf, 'r', encoding='utf-8', errors='ignore') as f:
                content += f.read() + '\n'
        except Exception as e:
            log(f'WARNING: Could not read scratch file {sf.name}: {e}')
    return content

def is_documented(filepath: str, scratch_content: str) -> bool:
    basename = os.path.basename(filepath)
    fwd_path = filepath.replace('\\', '/')
    return (basename in scratch_content) or (fwd_path in scratch_content)

def tick_file(filepath: str) -> bool:
    """Mark a file as [x] in the tracker. Returns True if successful."""
    with open(TASK_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    found = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('- [ ]'):
            candidate = stripped.replace('- [ ]', '').strip()
            if candidate == filepath:
                new_lines.append(line.replace('- [ ]', '- [x]', 1))
                found = True
                continue
        new_lines.append(line)

    if not found:
        log(f'SKIP — "{filepath}" not found as unticked in tracker (already ticked or doesn\'t exist)')
        return False

    with open(TASK_FILE, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)
    return True

def run_integrity_check():
    """Quick spot-check: count any ticked files missing from scratch. Print summary."""
    with open(TASK_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    ticked = [l.strip().replace('- [x]', '').strip() for l in lines if l.strip().startswith('- [x]')]
    scratch = load_scratch_content()
    missing = [t for t in ticked if not is_documented(t, scratch)]
    log(f'INTEGRITY CHECK: {len(ticked)} ticked, {len(missing)} missing documentation')
    if missing:
        log(f'  First 5 missing: {missing[:5]}')
    return len(missing)

def main():
    if len(sys.argv) < 2:
        print('Usage: python verified_tick.py <relative_path>')
        print('   or: python verified_tick.py --integrity-check')
        sys.exit(1)

    if sys.argv[1] == '--integrity-check':
        missing_count = run_integrity_check()
        sys.exit(0 if missing_count == 0 else 1)

    filepath = sys.argv[1].strip()
    log(f'TICK REQUEST: {filepath}')

    scratch_content = load_scratch_content()

    if not is_documented(filepath, scratch_content):
        log(f'BLOCKED — "{filepath}" has NO documentation in scratch/*.md')
        log(f'  basename searched: {os.path.basename(filepath)}')
        log(f'  path searched:     {filepath.replace(chr(92), "/")}')
        log(f'  You MUST write scratch notes FIRST, then call this script.')
        sys.exit(1)

    success = tick_file(filepath)
    if success:
        log(f'TICKED — "{filepath}" is documented and has been marked [x]')
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
