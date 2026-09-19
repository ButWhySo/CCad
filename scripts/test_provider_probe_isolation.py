"""Prove transient provider probe restores active adapter; no network."""

import json
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env["CCAD_PROVIDER"] = "mock"
env["PYTHONNOUSERSITE"] = "1"
requests = [
    {"method": "agent.test_provider", "params": {"provider": "cerebras", "model": "gpt-oss-120b"}},
    {"method": "human_message", "params": {"text": "probe isolation"}},
]
run = subprocess.run(
    [sys.executable, str(root / "src" / "ccad_agent" / "orchestrator.py")],
    input="\n".join(json.dumps(item) for item in requests) + "\n",
    text=True, capture_output=True, env=env, timeout=20, check=True,
)
events = [json.loads(line) for line in run.stdout.splitlines() if line.startswith("{")]
states = [item["params"] for item in events if item.get("method") == "provider_state"]
assert any(item.get("provider") == "mock" and item.get("execution_enabled") is True for item in states)
assert "[mock provider]" in run.stdout
print("PASS transient provider probe restores active adapter; no network")
