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
results = [item["params"] for item in events if item.get("method") == "provider_test_result"]
assert any(item.get("provider") == "mock" and item.get("execution_enabled") is True for item in states)
assert len(results) == 1
assert results[0].get("provider") == "cerebras"
assert results[0].get("network_access") == "not_probed"
assert results[0].get("secret_value_visible") is False
assert results[0].get("error_category") == "missing_api_key"
assert events.index(next(item for item in events if item.get("method") == "provider_test_result")) > \
       max(index for index, item in enumerate(events)
           if item.get("method") == "provider_state" and item["params"].get("provider") == "mock")
assert "[mock provider]" in run.stdout
print("PASS transient provider validation restores active adapter and emits a terminal result; no network")
