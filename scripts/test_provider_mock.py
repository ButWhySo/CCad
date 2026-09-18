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
assert '"provider": "mock"' in run.stdout
assert '"method": "tool_call"' in run.stdout
assert '"tool": "ui.place_via"' in run.stdout
assert '"run_state": "completed"' in run.stdout
assert "[mock provider] Request understood" in run.stdout
print("PASS mock provider chat and tool-loop execution")
