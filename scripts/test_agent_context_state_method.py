"""No-network contract for opaque read-only context-state discovery."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '{"name": "agent.context_state", "read_only": True' in text
assert 'elif method == "agent.context_state":' in text
assert '"context_state_snapshot"' in text
assert '"revision": context_revisions.get(thread_id, "")' in text
assert '"content_emitted": False' in text
assert '"secret_value_visible": False' in text
print("PASS opaque agent.context_state discovery contract; no network")
