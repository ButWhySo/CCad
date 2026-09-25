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
                            thread_id="thread-a", task_id="task-a")
    manager.configure({"stm": True, "ltm": True, "episodic": True})
    try:
        execute_memory_command(manager,
                               "add tier:ltm scope:project Missing project identity")
    except ValueError:
        pass
    else:
        raise AssertionError("project memory was accepted without an active project ID")
    manager.set_identities(task_id="turn-only", thread_id="thread-a",
                           project_id="project-a", retain_stm_task=False)
    try:
        execute_memory_command(manager, "add tier:stm title:stm Keep only this task constraint")
    except RuntimeError as error:
        assert "active task" in str(error)
    else:
        raise AssertionError("non-retained STM memory write succeeded")
    added, result = execute_memory_command(
        manager, 'add tier:ltm scope:conversation kind:preference title:"route rule" Keep ground return short')
    assert added == "memory_added" and result["tier"] == "ltm"
    assert result["kind"] == "preference"
    listed, result = execute_memory_command(manager, "list tier:ltm scope:conversation")
    assert listed == "memory_state" and len(result["entries"]) == 1
    entry_id = result["entries"][0]["id"]
    _, update = execute_memory_command(
        manager, f"update {entry_id} kind:correction Keep return path short")
    assert update["updated"]
    assert manager.list(tier="ltm")[0]["kind"] == "correction"
    assert manager.list(tier="ltm")[0]["namespace"] == "thread-a"
    project_added, project_result = execute_memory_command(
        manager, 'add tier:ltm scope:project title:"board rule" Keep analog ground return clear')
    assert project_added == "memory_added"
    project_id = project_result["id"]
    project_record = manager.store.list(
        tier="ltm", namespace=manager.namespace_for("ltm", "project"))[0]
    assert project_record["id"] == project_id
    _, project_update = execute_memory_command(
        manager, f"update {project_id} Keep analog ground return isolated")
    assert project_update["updated"]
    assert manager.store.list(tier="ltm", namespace=manager.namespace_for(
        "ltm", "project"))[0]["scope"] == "project"
    _, moved_to_thread = execute_memory_command(
        manager, f"update {project_id} scope:conversation Keep analog return isolated")
    assert moved_to_thread["updated"]
    assert manager.list(tier="ltm", scope="project") == []
    assert manager.list(tier="ltm", scope="conversation")[0]["namespace"] == "thread-a"
    _, moved_to_project = execute_memory_command(
        manager, f"update {project_id} scope:project Keep analog return isolated")
    assert moved_to_project["updated"]
    manager.set_identities(task_id="task-b", thread_id="thread-b",
                           project_id="project-b")
    assert manager.list(tier="ltm", scope="project") == []
    other_project = manager.add("Preserve USB shield near J4",
                                tier="ltm", scope="project")
    manager.set_identities(task_id="task-a", thread_id="thread-a",
                           project_id="project-a")
    assert manager.list(tier="ltm", scope="project")[0]["id"] == project_id
    _, deleted = execute_memory_command(manager, f"delete {entry_id}")
    assert deleted["removed"]
    manager.reset("ltm")
    assert manager.list(tier="ltm", scope="project") == []
    assert manager.store.list(tier="ltm", namespace=other_project["namespace"]) == []
    manager.add("episodic fact", tier="episodic", scope="user")
    _, cleared = execute_memory_command(manager, "clear scope:user tier:episodic")
    assert cleared["removed"] == 1
    try:
        execute_memory_command(manager, "add tier:ltm api_key:sk-testabcdef1234567890")
    except ValueError:
        pass
    else:
        raise AssertionError("secret-bearing memory command succeeded")

print("PASS tier-aware thread/project/episodic memory slash CRUD; no network")
