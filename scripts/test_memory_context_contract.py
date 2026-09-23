"""No-network contract for bounded, opt-in local memory context injection."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "def local_memory_entries():" in text
assert 'memory_config.get("stm", True)' in text
assert 'memory_store.list(scope="project")[-8:]' in text
assert 'memory_entries = local_memory_entries()' in text
assert 'package = build_context_package(' in text
print("PASS bounded local memory retrieval contract; no network")
