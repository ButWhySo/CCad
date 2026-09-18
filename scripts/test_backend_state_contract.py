"""Prove backend readiness is emitted separately from provider readiness."""

import json
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env["CCAD_PROVIDER"] = "mock"
env["PYTHONNOUSERSITE"] = "1"
result = subprocess.run(
    [sys.executable, str(root / "src" / "ccad_agent" / "orchestrator.py")],
    input='{"method":"agent.methods"}\n',
    text=True,
    capture_output=True,
    env=env,
    timeout=20,
    check=False,
)
assert result.returncode == 0, result.stderr
events = [json.loads(line) for line in result.stdout.splitlines() if line.strip().startswith("{")]
backend = next(item["params"] for item in events if item.get("method") == "backend_state")
assert backend["runtime"] == "python"
assert backend["ready"] is True
assert backend["provider_initialized"] is True
assert backend["network_access"] == "not_probed"
assert backend["secret_value_visible"] is False
print("PASS backend readiness emitted separately; no network")
