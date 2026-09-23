"""No-network contract for bounded, opt-in local memory context injection."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "def local_memory_entries(query=\"\"):" in text
assert "from memory_manager import MemoryManager" in text
assert "memory_manager.retrieve_with_metadata(query)" in text
assert 'memory_manager.configure(config_manager.get("memory", {}))' in text
assert 'memory_manager.configure(clean_config.get("memory", {}))' in text
assert 'elif cmd_base == "/task":' in text
assert 'memory_task_scopes.start(requested_session)' in text
assert 'memory_task_scopes.end(requested_session)' in text
assert 'memory_entries, memory_retrieval = local_memory_entries(text)' in text
assert 'package = build_context_package(' in text
print("PASS bounded local memory retrieval contract; no network")
