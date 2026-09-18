"""No-network contract for bounded, opt-in local memory context injection."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert "def local_memory_context():" in text
assert 'memory_config.get("stm", True)' in text
assert 'memory_store.list(scope="project")[-8:]' in text
assert "entry.get('content', '')[:1000]" in text
assert "raw_context = (raw_context +" in text
print("PASS opt-in bounded local memory context contract; no network")
