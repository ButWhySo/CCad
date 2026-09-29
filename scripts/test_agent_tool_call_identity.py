"""Provider tool correlation IDs must be real; missing IDs fail closed."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from orchestrator import model_tool_call_event, tool_result_call_id


assert model_tool_call_event({
    "id": "call-123", "name": "ui.draw_graphic", "args": {"x": 1},
}) == {
    "jsonrpc": "2.0",
    "method": "tool_call",
    "params": {"tool": "ui.draw_graphic", "args": {"x": 1}, "call_id": "call-123"},
}
for invalid in (
    {"name": "ui.draw_graphic", "args": {}},
    {"id": "  ", "name": "ui.draw_graphic", "args": {}},
    {"id": "call-123", "name": "", "args": {}},
    {"id": "call-123", "name": "ui.draw_graphic", "args": []},
):
    assert model_tool_call_event(invalid) is None

assert tool_result_call_id({"id": "call-123"}) == "call-123"
assert tool_result_call_id({"params": {"call_id": "call-456"}}) == "call-456"
assert tool_result_call_id({"id": "", "params": {"call_id": "call-456"}}) == "call-456"
assert tool_result_call_id({}) is None
assert tool_result_call_id({"id": "  ", "params": {"call_id": "  "}}) is None

print("PASS provider tool calls and results require authentic correlation IDs")
