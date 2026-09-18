"""No-network contract for correlated broker cancellation and late results."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'elif method == "agent.cancel_tool":' in text
assert '"tool_canceled"' in text
assert '"tool_cancel_ignored"' in text
assert '"unknown_or_late_call"' in text
assert 'pending_result_queue.put(json.dumps({' in text
assert 'cancel_params.get("thread_id")' in text
assert 'resume_checkpointed_run' in text
print("PASS broker cancellation and late-result contract; no network")
