"""Verify Gemini BYOK uses the adapter key name without leaking the secret."""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
secret = "gemini-test-secret-not-real"
env = os.environ.copy()
env["CCAD_PROVIDER"] = "mock"
env["PYTHONPATH"] = str(ROOT / "src" / "ccad_agent")
env["CCAD_GEMINI_MODEL"] = "gemini-test-model"
request = {"method": "agent.set_provider_secret", "params": {
    "provider": "google_gemini", "secret": secret}}
run = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps(request) + "\n", text=True, capture_output=True, env=env,
    timeout=20, check=True)
assert '"provider": "google_gemini"' in run.stdout
assert '"secret_value_visible": false' in run.stdout
assert secret not in run.stdout
assert secret not in run.stderr

terminal = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps({"method": "agent.test_provider", "params": {
        "provider": "unsupported_test_provider", "model": "test-model",
        "secret": ""}}) + "\n", text=True, capture_output=True, env=env,
    timeout=20, check=True)
assert '"method": "provider_state"' in terminal.stdout
assert '"error": "missing_api_key"' in terminal.stdout
assert secret not in terminal.stdout
assert secret not in terminal.stderr
source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text()
assert 'os.environ.get("CCAD_GEMINI_MODEL") or model_name' in source
assert 'method == "agent.test_provider"' in source
test_start = source.index('method == "agent.test_provider"')
secret_start = source.index('method == "agent.set_provider_secret"')
assert "config_manager.update" not in source[test_start:secret_start]
assert 'os.environ.pop("GOOGLE_API_KEY", None)' in source[test_start:secret_start]
assert 'error": "provider_unavailable" if secret else "missing_api_key"' in source[test_start:secret_start]
print("PASS Gemini BYOK secret alias and redaction")
