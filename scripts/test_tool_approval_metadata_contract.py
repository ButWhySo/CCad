"""No-network contract: mutating tool calls expose approval requirement."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"approval_required": bool(await_result and approval["required"])' in text
assert 'tool_approval_decision("ui.place_via", args)["required"]' in text
assert '"approval_required": True' in text
assert '"tool", "args", "call_id", "approval_required"' in text
print("PASS tool approval metadata is explicit and dry-run aware; no network")
