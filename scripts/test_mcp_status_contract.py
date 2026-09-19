"""No-network MCP status discovery contract."""

from pathlib import Path
import json
import os
import subprocess
import sys

root = Path(__file__).parents[1]
source = root / "src" / "ccad_agent" / "orchestrator.py"
env = os.environ.copy()
env.update({"CCAD_PROVIDER": "mock", "PYTHONNOUSERSITE": "1",
            "PYTHONPATH": str(source.parent)})
request = (json.dumps({"method": "agent.methods", "params": {}}) + "\n" +
           json.dumps({"method": "agent.mcp_status", "params": {}}) + "\n")
run = subprocess.run([sys.executable, str(source)], input=request, text=True,
                     capture_output=True, env=env, check=True)
events = [json.loads(line) for line in run.stdout.splitlines() if line.startswith("{")]
methods = next(event for event in events if event.get("method") == "agent_methods")
contract = next(item for item in methods["params"]["methods"]
                if item["name"] == "agent.mcp_status")
assert contract["read_only"] is True
status = next(event["params"] for event in events if event.get("method") == "mcp_status")
assert status == {"servers": [], "configured": False,
                  "runtime": "not_started", "process_execution": False}
assert run.stderr == ""
print("PASS MCP status boundary; no network or server launch")
