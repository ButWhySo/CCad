"""Provider-free end-to-end chat smoke for the LangGraph adapter path."""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env["CCAD_PROVIDER"] = "mock"
env["PYTHONPATH"] = str(ROOT / "src" / "ccad_agent")
request = {"method": "human_message", "params": {
    "text": "/route", "context": "board has 2 layers"}}
run = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps(request) + "\n", text=True, capture_output=True, env=env, check=True)
events = [json.loads(line) for line in run.stdout.splitlines()
          if line.strip().startswith("{")]
tool_events = [event for event in events if event.get("method") == "tool_call"]
assert tool_events
tool_params = tool_events[0]["params"]
assert tool_params["tool"] == "ui.place_via"
assert set(tool_params) == {"tool", "args", "call_id", "approval_required"}
assert tool_params["approval_required"] is False
assert tool_params["args"]["dry_run"] is True
assert any(event.get("method") == "provider_state"
           and event["params"].get("provider") == "mock" for event in events)
assert any(event.get("params", {}).get("run_state") == "completed"
           for event in events)
assert "[mock provider] Request understood" in run.stdout
print("PASS mock provider chat and tool-loop execution")
