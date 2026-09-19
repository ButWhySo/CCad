"""Verify MCP plan is inspectable without launching configured commands."""
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="ccad-mcp-plan-") as appdata:
    env = os.environ.copy()
    env["APPDATA"] = appdata
    requests = [
        {"method": "agent.set_config", "params": {"mcp_servers": [
            {"name": "demo", "command": "ccad-mcp", "args": ["--stdio"],
             "port": 0, "enabled": True}]}},
        {"method": "agent.mcp_plan", "params": {}},
    ]
    run = subprocess.run(
        [sys.executable, str(root / "src" / "ccad_agent" / "orchestrator.py")],
        input="\n".join(json.dumps(item) for item in requests) + "\n",
        text=True, capture_output=True, env=env, timeout=20, check=True)
events = [json.loads(line) for line in run.stdout.splitlines() if line.strip()]
plan = next(item["params"] for item in events if item.get("method") == "mcp_plan")
assert plan["servers"][0]["command"] == "ccad-mcp"
assert plan["launch_allowed"] is False
assert "explicit approval" in plan["reason"]
assert "ccad-mcp" not in run.stderr
print("PASS MCP execution plan is inspectable and non-launching; no network")
