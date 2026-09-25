"""No-network contract for bounded, opt-in local memory context injection."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "from context_broker import ContextBroker, extract_context_signals" in text
assert "context_broker.prepare(" in text
assert 'memory_entries = turn_context["memories"]' in text
assert 'memory_entries, memory_retrieval = local_memory_entries(memory_query)' not in text
assert "from memory_manager import MemoryManager" in text
assert 'memory_manager.configure(config_manager.get("memory", {}))' in text
assert 'memory_manager.configure(clean_config.get("memory", {}))' in text
assert 'elif cmd_base == "/task":' in text
assert 'memory_task_scopes.start(requested_session)' in text
assert 'memory_task_scopes.end(requested_session)' in text
assert 'memory_query = text' in text
assert 'if text.partition(" ")[0].casefold() == "/context":' in text
assert 'package = build_context_package(' in text
assert 'memory_summary=turn_context["memory_summary"]' in text
assert 'memory_manifest=turn_context["manifest"]' in text
assert 'active_conversation_project_id = str(project_id or "")' in text
assert 'memory_manager.compaction_identities()' in text
print("PASS bounded local memory retrieval contract; no network")
