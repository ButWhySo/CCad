"""No-network contract for opaque pending-call recovery metadata."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'def pending_call_snapshot(thread_id: str = "")' in text
assert 'def orchestrator_method_catalog()' in text
assert 'elif method == "agent.methods":' in text
assert 'elif method == "agent.pending_calls":' in text
assert '"pending_calls_state"' in text
assert '"secret_value_visible": False' in text
assert '"checkpoint_call_ids"' in text
print("PASS opaque pending-call recovery contract; no network")
