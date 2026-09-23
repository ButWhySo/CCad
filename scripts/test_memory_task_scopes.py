"""No-network tests for task-scoped STM lifetime and session isolation."""

import tempfile
from pathlib import Path
import sys

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from memory_manager import MemoryManager, MemoryTaskScopes
from memory_store import MemoryStore


with tempfile.TemporaryDirectory() as directory:
    manager = MemoryManager(MemoryStore(Path(directory) / "memory.json"))
    manager.configure({"stm": True})
    scopes = MemoryTaskScopes(manager, max_sessions=2)

    first_task, cleared = scopes.start("session-one")
    assert not cleared and scopes.current("session-one") == first_task
    assert scopes.is_active(first_task)
    assert not scopes.is_active("unknown-task")
    manager.set_identities(task_id=first_task, thread_id="thread-one",
                           project_id="project-one")
    manager.add("Keep this task's ground reference", tier="stm")

    second_task, _ = scopes.start("session-two")
    manager.set_identities(task_id=second_task, thread_id="thread-two",
                           project_id="project-two")
    manager.add("Keep second task's board constraint", tier="stm")
    assert scopes.current("session-one") == first_task
    assert scopes.current("session-two") == second_task

    replacement_task, cleared = scopes.start("session-one")
    assert replacement_task != first_task and cleared == 1
    manager.set_identities(task_id=replacement_task, thread_id="thread-one",
                           project_id="project-one")
    assert manager.list(tier="stm") == []

    ended_task, removed = scopes.end("session-two")
    assert ended_task == second_task and removed == 1
    assert scopes.current("session-two") is None
    assert not scopes.is_active(second_task)
    assert scopes.end("session-two") == (None, 0)

    third_task, _ = scopes.start("session-three")
    manager.set_identities(task_id=third_task, thread_id="thread-three",
                           project_id="project-three")
    manager.add("Temporary task-three constraint", tier="stm")
    scopes.start("session-four")
    scopes.start("session-five")
    assert scopes.current("session-three") is None
    assert manager.clear_task(third_task) == 0

print("PASS task-scoped STM identity, replacement, end, and bounded session map")
