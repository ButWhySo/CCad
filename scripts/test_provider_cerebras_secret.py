"""Verify Cerebras BYOK enters the child session without secret leakage."""
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
secret = "cerebras-test-secret-not-real"
env = os.environ.copy()
env["CCAD_PROVIDER"] = "mock"
env["PYTHONPATH"] = str(ROOT / "src" / "ccad_agent")
env["CCAD_CEREBRAS_MODEL"] = "qwen-3.8-27b"
request = {"method": "agent.set_provider_secret", "params": {
    "provider": "cerebras", "secret": secret}}
run = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps(request) + "\n", text=True, capture_output=True, env=env,
    timeout=20, check=True)
assert '"provider": "cerebras"' in run.stdout
assert '"configured": true' in run.stdout
assert '"secret_value_visible": false' in run.stdout
assert secret not in run.stdout
assert secret not in run.stderr

source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text()
assert '"cerebras": "CEREBRAS_API_KEY"' in source
assert 'set_session_provider_env(env_name, secret)' in source
assert 'init_provider()' in source
print("PASS Cerebras BYOK session injection and redaction; no request sent")
