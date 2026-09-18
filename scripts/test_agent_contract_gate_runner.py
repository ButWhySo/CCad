"""Static, no-GUI contract for the offline agent gate selector."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
runner = (root / "scripts" / "run_agent_contract_gate.ps1").read_text(encoding="utf-8")

assert "src\\ccad_agent\\venv\\Scripts\\python.exe" in runner
assert '"_gui_"' in runner
assert '"test_gui_"' in runner
assert '"visual"' in runner
assert '"live"' in runner
assert '"provider_real"' in runner
assert '"test_mcp_gui_bridge.py"' in runner
assert "IncludeCheckpointRestart" in runner
print("PASS offline agent gate excludes GUI/live tests and retains headless MCP")
