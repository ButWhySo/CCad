"""Verify Gemini BYOK uses the adapter key name without leaking the secret."""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
secret = "gemini-test-secret-not-real"
env = os.environ.copy()
env["CCAD_AGENT_DEFER_PROVIDER_INIT"] = "1"
env["PYTHONPATH"] = str(ROOT / "src" / "ccad_agent")
env["CCAD_GEMINI_MODEL"] = "gemini-test-model"
request = {"method": "agent.set_provider_secret", "params": {
    "provider": "google_gemini", "secret": secret}}
run = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps(request) + "\n", text=True, capture_output=True, env=env,
    timeout=20, check=True)
events = [json.loads(line) for line in run.stdout.splitlines() if line.startswith("{")]
secret_result = next(item["params"] for item in events
                     if item.get("method") == "provider_secret_result")
assert secret_result["provider"] == "google_gemini"
assert secret_result["secret_value_visible"] is False
assert secret_result["network_access"] == "not_probed"
assert secret not in run.stdout
assert secret not in run.stderr

terminal = subprocess.run(
    [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
    input=json.dumps({"method": "agent.test_provider", "params": {
        "provider": "unsupported_test_provider", "model": "test-model",
        "secret": ""}}) + "\n", text=True, capture_output=True, env=env,
    timeout=20, check=True)
terminal_events = [json.loads(line) for line in terminal.stdout.splitlines()
                   if line.startswith("{")]
terminal_result = next(item["params"] for item in terminal_events
                       if item.get("method") == "provider_test_result")
assert terminal_result["error"] == "missing_api_key"
assert terminal_result["error_category"] == "missing_api_key"
assert terminal_result["network_access"] == "not_probed"
assert secret not in terminal.stdout
assert secret not in terminal.stderr
source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text()
assert '"google_gemini": "CCAD_GEMINI_MODEL"' in source
assert 'if model_env: os.environ[model_env] = model' in source
assert 'method in ("agent.test_provider", "agent.test_provider_connection")' in source
test_start = source.index('method in ("agent.test_provider", "agent.test_provider_connection")')
secret_start = source.index('method == "agent.set_provider_secret"')
assert "config_manager.update" not in source[test_start:secret_start]
assert 'clear_session_provider_env()' in source[test_start:secret_start]
assert 'set_session_provider_env("GOOGLE_API_KEY", secret)' in source[test_start:secret_start]
assert '"method": "provider_test_result"' in source[test_start:secret_start]
assert '"error_category": test_category' in source[test_start:secret_start]
assert 'failure_category == "missing_api_key"' in source
assert 'is not configured. ' in source
print("PASS Gemini BYOK secret alias and redaction")
