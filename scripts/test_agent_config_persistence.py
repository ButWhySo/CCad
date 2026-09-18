"""Verify agent.set_config survives a fresh orchestrator process."""
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORCHESTRATOR = ROOT / "src" / "ccad_agent" / "orchestrator.py"


def run_agent(appdata, requests):
    env = os.environ.copy()
    env["APPDATA"] = str(appdata)
    env["CCAD_PROVIDER"] = "mock"
    # On Windows, APPDATA also controls Python's user-site location. Keep
    # isolated config storage without hiding the installed agent dependencies.
    python_paths = [str(ROOT / "src" / "ccad_agent")]
    python_paths.extend(path for path in sys.path if path and "site-packages" in path.lower())
    env["PYTHONPATH"] = os.pathsep.join(dict.fromkeys(python_paths))
    result = subprocess.run(
        [sys.executable, str(ORCHESTRATOR)],
        input="".join(json.dumps(request) + "\n" for request in requests),
        text=True,
        capture_output=True,
        env=env,
        timeout=20,
        check=False,
    )
    if result.returncode != 0:
        raise AssertionError(f"orchestrator failed: {result.stderr}")
    return [json.loads(line) for line in result.stdout.splitlines()
            if line.strip().startswith("{")]


with tempfile.TemporaryDirectory() as temp:
    appdata = Path(temp)
    run_agent(appdata, [{"method": "agent.set_config", "params": {
        "provider": "cerebras", "model": "gpt-oss-120b", "grid": "2.5 mm"}}])
    responses = run_agent(appdata, [{"method": "agent.get_config", "params": {}}])
    config = next(item["params"] for item in responses
                  if item.get("method") == "config_state")
    assert config["provider"] == "cerebras"
    assert config["model"] == "gpt-oss-120b"
    assert config["grid"] == "2.5 mm"
print("PASS agent config persists across orchestrator restart")
