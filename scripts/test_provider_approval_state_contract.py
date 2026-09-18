"""No-network contract for truthful provider tool-run state."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"awaiting_tool_approval"' in text
assert 'has_tool_calls = bool(getattr(last_msg, "tool_calls", None))' in text
assert '"run_state": "awaiting_tool_approval" if (has_tool_calls or has_legacy_tool) else "completed"' in text
assert 'def emit_tool_approval_state()' in text
assert text.count('emit_tool_approval_state()') >= 4
print("PASS truthful provider approval telemetry contract; no network")
