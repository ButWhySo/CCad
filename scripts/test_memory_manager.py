"""No-network lifecycle proof for STM/LTM/episodic memory tiers."""

import tempfile
from pathlib import Path
import sys

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from memory_manager import MemoryManager
from memory_store import MemoryStore


with tempfile.TemporaryDirectory() as temp:
    manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                            run_id="run-1", thread_id="thread-1",
                            project_id="project-1")
    manager.configure({"stm": True, "ltm": True, "episodic": False})
    manager.add("Keep the return path short", tier="ltm", title="routing")
    manager.add("Current placement target is U3", tier="stm", title="goal")
    assert [item["title"] for item in manager.retrieve("return path")] == ["routing"]
    assert manager.state("ltm")["runtime_entries"] == 1
    manager.disable("ltm")
    assert manager.state("ltm")["runtime_entries"] == 0
    assert manager.state("ltm")["persistent_entries"] == 1
    try:
        manager.update(manager.store.list(tier="ltm", namespace="thread-1")[0]["id"],
                       "must not update while disabled")
    except RuntimeError:
        pass
    else:
        raise AssertionError("disabled memory tier allowed mutation")
    manager.enable("ltm")
    assert manager.state("ltm")["runtime_entries"] == 1
    assert manager.reset("ltm") == 1
    assert manager.state("ltm")["persistent_entries"] == 0
    manager.enable("episodic")
    manager.add("User prefers conservative clearances", tier="episodic")
    assert manager.state("episodic")["persistent_entries"] == 1
    assert manager.reset() == 2
    assert all(manager.state(tier)["persistent_entries"] == 0
               for tier in manager.TIERS)

    # STM exists only for its task lifetime; LTM and episodic identities stay
    # isolated when a chat/project is switched in the running process.
    manager.set_identities(run_id="task-a", thread_id="thread-a",
                           project_id="project-a", user_id="local-user")
    manager.configure({"stm": True, "ltm": True, "episodic": True})
    transient = manager.add("Current task constraint", tier="stm", scope="task")
    assert manager.state("stm")["persistent_entries"] == 0
    assert manager.list(tier="stm")[0]["id"] == transient["id"]
    manager.add("Conversation preference", tier="ltm", scope="conversation")
    expired = manager.store.add("expired record", tier="ltm", namespace="thread-a",
                                scope="conversation", expires_at="2000-01-01T00:00:00Z")
    manager.enable("ltm")
    assert all(item["id"] != expired["id"] for item in manager.list(tier="ltm"))
    expired_live = manager.add("expires during process", tier="ltm", scope="conversation",
                               expires_at="2000-01-01T00:00:00Z")
    assert all(item["id"] != expired_live["id"]
               for item in manager.retrieve("expires during process"))
    assert all(item["id"] != expired_live["id"]
               for item in manager.store.list(tier="ltm", namespace="thread-a"))
    manager.add("Cross-project preference", tier="episodic", scope="user")
    manager.set_identities(run_id="task-b", thread_id="thread-b",
                           project_id="project-b", user_id="local-user")
    assert manager.state("stm")["runtime_entries"] == 0
    assert manager.list(tier="ltm") == []
    assert manager.list(tier="episodic")[0]["content"] == "Cross-project preference"
    manager.set_identities(run_id="task-a", thread_id="thread-a",
                           project_id="project-a", user_id="local-user")
    manager.enable("ltm")
    memory = manager.list(tier="ltm")[0]
    updated = manager.update(memory["id"], "Updated conversation preference")
    assert updated["tier"] == "ltm" and updated["namespace"] == "thread-a"
    assert manager.delete(updated["id"])
    assert not manager.delete(updated["id"])

    manager.add("same memory text", tier="ltm", scope="conversation")
    duplicate = manager.add(" SAME   memory TEXT ", tier="ltm", scope="conversation")
    assert duplicate["id"] == manager.list(tier="ltm")[0]["id"]
    assert manager.clear_scope("conversation", tier="ltm") == 1
    for index in range(70):
        manager.add(f"retention rule {index}", tier="ltm", scope="retention")
    assert len(manager.list(tier="ltm", scope="retention")) == 64
    assert manager.state("ltm")["persistent_entries"] == 64

print("PASS STM/LTM/episodic memory lifecycle; no network")
