"""No-network contract checks for bounded agent conversation context."""

from pathlib import Path
import json
import os
import subprocess
import sys


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert 'CCAD_AGENT_HISTORY_LIMIT' in text
assert 'def bound_session_history(messages):' in text
assert 'min(64, max(4, limit))' in text
assert 'session_messages = bound_session_history(final_state["messages"])' in text
assert 'memory_content_emitted' in text
assert 'local_project_memory' in text
assert 'request_context_present = bool(raw_context.strip())' in text
assert '"previous_revision": previous_context_revision' in text
assert '"change_kind": context_change_kind' in text
assert '"response_contracts": {' in text
assert '"intake_state": {"fields": [' in text

env = os.environ.copy()
env.update({"CCAD_PROVIDER": "mock", "PYTHONNOUSERSITE": "1",
            "PYTHONPATH": str(SOURCE.parent)})
requests = [{"method": "agent.methods", "params": {}}]
requests.extend({"method": "human_message", "params": {
    "text": "summarize", "context": context}}
    for context in ("board A", "board B", "board B"))
payload = "\n".join(json.dumps(request) for request in requests) + "\n"
run = subprocess.run([sys.executable, str(SOURCE)], input=payload, text=True,
                     capture_output=True, env=env, check=True)
lines = [json.loads(line) for line in run.stdout.splitlines()
         if line.strip().startswith("{")]
methods = next(item["params"] for item in lines
               if item.get("method") == "agent_methods")
human_contract = next(item for item in methods["methods"]
                      if item["name"] == "human_message")
assert "context_state" in human_contract["response_contracts"]
assert "change_kind" in human_contract["response_contracts"]["context_state"]["fields"]
assert "intake_state" in human_contract["response_contracts"]
assert "accepted" in human_contract["response_contracts"]["intake_state"]["fields"]
assert "provider_state" in human_contract["response_contracts"]
assert "execution_ready" in human_contract["response_contracts"]["provider_state"]["fields"]
events = [item for item in lines if item.get("method") == "context_state"]
assert len(events) == 3
assert events[0]["params"]["previous_revision"] == ""
assert events[0]["params"]["change_kind"] == "initial"
assert events[1]["params"]["previous_revision"] == events[0]["params"]["revision"]
assert events[1]["params"]["change_kind"] == "changed"
assert events[2]["params"]["change_kind"] == "unchanged"
print("PASS agent context history boundary contract; no network")
