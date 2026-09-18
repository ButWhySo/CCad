"""No-network contract: acknowledge tool results only after correlation validation."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
checkpoint_branch = text[text.index('if checkpoint_saver is not None:', text.index('elif method == "tool_result":')):]
ack = '"method": "tool_result_ack"'
assert checkpoint_branch.index('if not snapshot.next or not expected_call_id:') < checkpoint_branch.index(ack)
assert checkpoint_branch.index('reason": "call_id_mismatch"') < checkpoint_branch.index(ack)
assert 'pending_result_queue.put(json.dumps({' in checkpoint_branch
assert '"method": "tool_result_ignored"' in checkpoint_branch
print("PASS tool-result ack follows pending/correlation validation; no network")
