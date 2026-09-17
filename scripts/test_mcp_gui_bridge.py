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
    calls = []
    bridge.call_gui = lambda server, method, args, **kwargs: calls.append((method, args)) or {"ok": True}
    result = bridge.request_native_approval("test", "Approve route")
    assert result["approval_required"] is False
    assert "Native approval target unavailable" in result["human_action"]
    assert [call[0] for call in calls] == ["ui.type_text", "ui.click"]
    bridge_path = Path(__file__).with_name("ccad_mcp_gui_bridge.py")
    child = subprocess.Popen([sys.executable, str(bridge_path)], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, text=True)
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "id": 1, "method": "initialize"}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
    child.stdin.write(json.dumps({"jsonrpc": "2.0", "id": 2, "method": "ping"}) + "\n")
    child.stdin.close()
    output = child.stdout.read().splitlines()
    child.wait(timeout=5)
    assert [json.loads(line)["id"] for line in output] == [1, 2]
    print("PASS MCP GUI bridge read-only policy")


if __name__ == "__main__":
    main()
