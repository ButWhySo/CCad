"""Exercise the actual local LangChain memory-search tool with isolated storage."""

# Runtime path insertion below intentionally resolves modules from src/ccad_agent.
# pyright: reportMissingImports=false

import importlib.util
import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
AGENT = ROOT / "src" / "ccad_agent"
sys.path.insert(0, str(AGENT))
SPEC = importlib.util.spec_from_file_location("ccad_memory_search_orchestrator",
                                               AGENT / "orchestrator.py")
assert SPEC is not None and SPEC.loader is not None
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)

from context_broker import ContextBroker
from memory_manager import MemoryManager
from memory_store import MemoryStore

with tempfile.TemporaryDirectory() as temp:
    manager = MemoryManager(MemoryStore(Path(temp) / "memory.json"),
                            thread_id="search-thread", project_id="search-project")
    manager.configure({"stm": True, "ltm": True, "episodic": True})
    kept = manager.add("Keep connector clearance above one millimeter",
                       tier="ltm", title="Connector design constraint")
    project_fact = manager.add("This board keeps USB connector clearance above two millimeters",
                               tier="ltm", scope="project", title="Board clearance")
    manager.add("Prefer dark blue presentation headings", tier="episodic",
                title="Slide preference")
    setattr(module, "memory_manager", manager)
    setattr(module, "context_broker", ContextBroker(memory_token_budget=1000))
    initial = module.context_broker.prepare(
        manager, thread_id="search-thread", project_revision="r1",
        user_request="Review connector clearance")
    setattr(module, "active_turn_contexts", {"search-thread": initial})
    module.install_native_tool_catalog([{
        "method": "project.context", "description": "Inspect current project.",
        "read_only": True,
        "inputSchema": {"type": "object", "properties": {}, "required": []},
    }])
    search = next(tool for tool in module.agent_tools if tool.name == "ccad_search_memory")
    result = json.loads(search.invoke({"query": "connector clearance tolerance"}))
    assert result["ok"] is True
    assert result["context_version"] > initial["version"]
    assert any(row["id"] == kept["id"] for row in result["results"])
    assert any(row["id"] == project_fact["id"] for row in result["results"])
    assert all("Slide preference" not in row["title"] for row in result["results"])
    assert result["secret_value_visible"] is False
    assert module.agent_tools[0].name == "ccad_project_context"
print("PASS Agent can perform targeted local memory retrieval during its active turn")
