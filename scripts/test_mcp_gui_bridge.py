"""Offline contract test for read-only MCP GUI bridge policy."""
import ccad_mcp_gui_bridge as bridge
import json
import subprocess
import sys
from pathlib import Path


def main():
    assert "ui.map_compact" in bridge.READ_ONLY_METHODS
    assert "project.drc" in bridge.READ_ONLY_METHODS
    assert "ui.click" not in bridge.READ_ONLY_METHODS
    assert "ui.route_track" not in bridge.READ_ONLY_METHODS
    bridge_source = Path(__file__).with_name("ccad_mcp_gui_bridge.py").read_text(encoding="utf-8")
    assert '"name": "ccad_gui_approval_status"' in bridge_source
    calls = []
    bridge.call_gui = lambda server, method, args, **kwargs: calls.append((method, args)) or {"ok": True}
    result = bridge.request_native_approval("test", "Approve route")
    assert result["approval_required"] is False
    assert "Native approval target unavailable" in result["human_action"]
    assert [call[0] for call in calls] == ["ui.type_text", "ui.click"]
    bridge.call_gui = lambda *args, **kwargs: {"result": {"nodes": [
        {"id": "panel:agent_approval_preview", "visible": True},
        {"id": "label:agent_approval_status", "text": "Approval pending"},
        {"id": "unrelated", "text": "omit"}]}}
    status = bridge.approval_status("test")
    assert status["visible"] is True
    assert status["status"] == "Approval pending"
    assert "unrelated" not in status["nodes"]
    bridge_path = Path(__file__).with_name("ccad_mcp_gui_bridge.py")
    child = subprocess.Popen([sys.executable, str(bridge_path)], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, text=True)
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "id": 1, "method": "initialize"}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/cancelled",
                                  "params": {}}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "id": 2, "method": "ping"}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "id": 3, "method": "tools/call",
                                  "params": {"name": "no.such.tool"}}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "id": 4, "method": "tools/list"}) + "\n")
    child.stdin.close()
    output = child.stdout.read().splitlines()
    child.wait(timeout=5)
    messages = [json.loads(line) for line in output]
    assert [message["id"] for message in messages] == [1, 2, 3, 4]
    assert messages[2]["error"]["code"] == -32601
    tool_names = {tool["name"] for tool in messages[3]["result"]["tools"]}
    assert {"ccad_gui_query", "ccad_gui_request_approval",
            "ccad_gui_approval_status"}.issubset(tool_names)
    print("PASS MCP GUI bridge read-only policy")


if __name__ == "__main__":
    main()
