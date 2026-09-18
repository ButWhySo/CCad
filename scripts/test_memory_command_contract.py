"""No-network contract for orchestrator local memory commands."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "from memory_store import MemoryStore" in text
assert "def parse_memory_add_args(arguments):" in text
assert "parse_memory_add_args(memory_args[4:].strip())" in text
assert 'memory_args.startswith("list scope:")' in text
assert 'memory_store.list(scope=scope)' in text
assert "memory_store = MemoryStore()" in text
assert 'cmd_base == "/memory"' in text
assert 'memory_store.add(content, title=title, scope=scope)' in text
assert 'memory_store.delete(memory_args[7:].strip())' in text
assert 'memory_args == "clear all"' in text
assert 'memory_args.startswith("clear scope:")' in text
assert 'Bare clear does nothing.' in text
print("PASS local memory command boundary; no network")
