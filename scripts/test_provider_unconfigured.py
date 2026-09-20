"""A missing credential must not fabricate chat, tools, or PCB changes."""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env["CCAD_PROVIDER"] = "openai"
env["CCAD_AGENT_DEFER_PROVIDER_INIT"] = "0"
env.pop("OPENAI_API_KEY", None)
env["PYTHONPATH"] = str(ROOT / "src" / "ccad_agent")
request = {"method": "human_message", "params": {
    "text": "/route", "context": "board has 2 layers"}}
run = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps(request) + "\n", text=True, capture_output=True, env=env, check=True)
events = [json.loads(line) for line in run.stdout.splitlines()
          if line.strip().startswith("{")]
tool_events = [event for event in events if event.get("method") == "tool_call"]
assert not tool_events
assert any(event.get("method") == "provider_state"
           and event["params"].get("provider") == "openai"
           and event["params"].get("configured") is False for event in events)
assert "Request understood. Use approved CCad tools" not in run.stdout
assert "Local CCad agent received" in run.stdout
print("PASS unconfigured provider creates neither fake chat nor tool calls")
