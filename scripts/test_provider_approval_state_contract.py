"""No-network contract for truthful provider tool-run state."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"awaiting_tool_approval"' in text
assert 'has_tool_calls = bool(getattr(last_msg, "tool_calls", None))' in text
assert '"awaiting_tool_result"' in text
assert 'tool_calls_require_approval(getattr(last_msg, "tool_calls", None))' in text
assert '"run_state": "running"' in text
assert '"run_state": resumed_run_state' in text
assert 'def emit_tool_approval_state()' in text
assert 'if approval["required"] and emit_call:\n            emit_tool_approval_state()' in text
panel = (Path(__file__).parents[1] / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
assert 'run_state_ = params["run_state"].toString();' in panel
print("PASS truthful provider approval telemetry contract; no network")
