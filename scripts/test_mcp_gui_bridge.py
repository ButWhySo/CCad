"""Offline contract test for read-only MCP GUI bridge policy."""
import ccad_mcp_gui_bridge as bridge


def main():
    assert "ui.map_compact" in bridge.READ_ONLY_METHODS
    assert "project.drc" in bridge.READ_ONLY_METHODS
    assert "ui.click" not in bridge.READ_ONLY_METHODS
    assert "ui.route_track" not in bridge.READ_ONLY_METHODS
    calls = []
    bridge.call_gui = lambda server, method, args, **kwargs: calls.append((method, args)) or {"ok": True}
    result = bridge.request_native_approval("test", "Approve route")
    assert result["approval_required"] is True
    assert [call[0] for call in calls] == ["ui.type_text", "ui.click"]
    print("PASS MCP GUI bridge read-only policy")


if __name__ == "__main__":
    main()
