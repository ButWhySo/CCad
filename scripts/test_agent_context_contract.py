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

env = os.environ.copy()
env.update({"CCAD_PROVIDER": "mock", "PYTHONNOUSERSITE": "1",
            "PYTHONPATH": str(SOURCE.parent)})
payload = "\n".join(json.dumps({"method": "human_message", "params": {
    "text": "summarize", "context": context}})
    for context in ("board A", "board B")) + "\n"
run = subprocess.run([sys.executable, str(SOURCE)], input=payload, text=True,
                     capture_output=True, env=env, check=True)
events = [json.loads(line) for line in run.stdout.splitlines()
          if line.strip().startswith("{") and
          '"method": "context_state"' in line]
assert len(events) == 2
assert events[0]["params"]["previous_revision"] == ""
assert events[1]["params"]["previous_revision"] == events[0]["params"]["revision"]
print("PASS agent context history boundary contract; no network")
