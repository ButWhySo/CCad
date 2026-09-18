"""No-network contract for pending-call approval metadata."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"approval_required": bool(process_calls or checkpoint_calls)' in text
assert '"approval_reason": "project_mutation" if (process_calls or checkpoint_calls) else ""' in text
assert '"name": "agent.pending_calls"' in text
assert '"approval_reason", "secret_value_visible"]' in text
print("PASS pending-call approval metadata contract; no network")
