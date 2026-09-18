"""Exercise model-catalog protocol branches without provider/network access."""

import json
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env.pop("OPENROUTER_API_KEY", None)
env["CCAD_PROVIDER"] = "mock"
env["PYTHONNOUSERSITE"] = "1"
result = subprocess.run(
    [sys.executable, str(root / "src" / "ccad_agent" / "orchestrator.py")],
    input=(json.dumps({"method": "agent.list_models", "params": {"provider": "openrouter"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "cerebras"}})
           + "\n"),
    text=True,
    capture_output=True,
    env=env,
    timeout=20,
    check=False,
)
assert result.returncode == 0, result.stderr
responses = [json.loads(line) for line in result.stdout.splitlines()
             if line.strip().startswith("{")]
catalogs = [item["params"] for item in responses if item.get("method") == "provider_models"]
openrouter = next(item for item in catalogs if item["provider"] == "openrouter")
cerebras = next(item for item in catalogs if item["provider"] == "cerebras")
assert openrouter["ok"] is False
assert openrouter["error"] == "missing_api_key"
assert "OPENROUTER_API_KEY" not in json.dumps(openrouter)
assert cerebras["ok"] is False
assert cerebras["error"] == "catalog_not_implemented"
assert cerebras["error_detail"]
assert cerebras["network_access"] == "explicit_refresh"
print("PASS model catalog protocol branches; no network")
