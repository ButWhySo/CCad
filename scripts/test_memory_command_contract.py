"""No-network behavior checks for tier-aware memory commands."""

import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from memory_commands import execute_memory_command
from memory_manager import MemoryManager
from memory_store import MemoryStore


with tempfile.TemporaryDirectory() as temp:
    manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                            thread_id="thread-a", run_id="task-a")
    manager.configure({"stm": True, "ltm": True, "episodic": True})
    added, result = execute_memory_command(
        manager, 'add tier:ltm scope:conversation title:"route rule" Keep ground return short')
    assert added == "memory_added" and result["tier"] == "ltm"
    listed, result = execute_memory_command(manager, "list tier:ltm scope:conversation")
    assert listed == "memory_state" and len(result["entries"]) == 1
    entry_id = result["entries"][0]["id"]
    _, update = execute_memory_command(manager, f"update {entry_id} Keep return path short")
    assert update["updated"]
    assert manager.list(tier="ltm")[0]["namespace"] == "thread-a"
    _, deleted = execute_memory_command(manager, f"delete {entry_id}")
    assert deleted["removed"]
    manager.add("episodic fact", tier="episodic", scope="user")
    _, cleared = execute_memory_command(manager, "clear scope:user tier:episodic")
    assert cleared["removed"] == 1
    try:
        execute_memory_command(manager, "add tier:ltm api_key:sk-testabcdef1234567890")
    except ValueError:
        pass
    else:
        raise AssertionError("secret-bearing memory command succeeded")

print("PASS tier-aware memory slash command CRUD and filtering; no network")
