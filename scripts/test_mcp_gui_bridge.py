"""Offline contract test for read-only MCP GUI bridge policy."""
from ccad_mcp_gui_bridge import READ_ONLY_METHODS


def main():
    assert "ui.map_compact" in READ_ONLY_METHODS
    assert "project.drc" in READ_ONLY_METHODS
    assert "ui.click" not in READ_ONLY_METHODS
    assert "ui.route_track" not in READ_ONLY_METHODS
    print("PASS MCP GUI bridge read-only policy")


if __name__ == "__main__":
    main()
