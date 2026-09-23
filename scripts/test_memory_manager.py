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

print("PASS STM/LTM/episodic memory lifecycle; no network")
