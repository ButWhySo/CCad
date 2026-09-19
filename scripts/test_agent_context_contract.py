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
requests.append({"method": "agent.context_state", "params": {}})
requests.append({"method": "agent.pending_calls", "params": {}})
requests.extend({"method": "human_message", "params": {
    "text": "summarize", "context": context, "thread_id": thread_id}}
    for context, thread_id in (("board A", "thread-a"),
                               ("board B", "thread-b"),
                               ("board B", "thread-b")))
requests.append({"method": "human_message", "params": {
    "text": "/clear", "context": "board B"}})
requests.append({"method": "agent.context_state", "params": {}})
payload = "\n".join(json.dumps(request) for request in requests) + "\n"
run = subprocess.run([sys.executable, str(SOURCE)], input=payload, text=True,
                     capture_output=True, env=env, check=True)
lines = [json.loads(line) for line in run.stdout.splitlines()
         if line.strip().startswith("{")]
methods = next(item["params"] for item in lines
               if item.get("method") == "agent_methods")
human_contract = next(item for item in methods["methods"]
                      if item["name"] == "human_message")
assert human_contract["params"]["thread_id"]["optional"] is True
assert human_contract["params"]["thread_id"]["type"] == "string"
pending_contract = next(item for item in methods["methods"]
                        if item["name"] == "agent.pending_calls")
assert pending_contract["params"]["thread_id"]["optional"] is True
assert pending_contract["params"]["thread_id"]["type"] == "string"
assert pending_contract["response"]["method"] == "pending_calls_state"
context_contract = next(item for item in methods["methods"]
                        if item["name"] == "agent.context_state")
assert context_contract["params"]["thread_id"]["optional"] is True
assert context_contract["params"]["thread_id"]["type"] == "string"
set_thread_contract = next(item for item in methods["methods"]
                           if item["name"] == "agent.set_thread_id")
assert set_thread_contract["params"]["thread_id"]["optional"] is False
resume_contract = next(item for item in methods["methods"]
                       if item["name"] == "agent.resume_thread")
assert resume_contract["params"]["resume"]["type"] == "object"
assert resume_contract["params"]["resume"]["optional"] is True
assert "thread_resumed" in resume_contract["responses"]
assert "checkpoint_id" in resume_contract["response"]["fields"]
assert "message_count" in resume_contract["response_contracts"]["thread_resumed"]["fields"]
secret_contract = next(item for item in methods["methods"]
                       if item["name"] == "agent.set_provider_secret")
assert secret_contract["secrets"] is True
assert secret_contract["params"]["secret"]["secret"] is True
assert "secret_value_visible" in secret_contract["response"]["fields"]
for field in ("thread_id", "process_call_ids", "checkpoint_call_ids", "count", "approval_required",
              "approval_reason", "secret_value_visible"):
    assert field in pending_contract["response"]["fields"]
assert "context_state" in human_contract["response_contracts"]
assert "provider_state" in human_contract["responses"]
assert "backend_state" in human_contract["responses"]
assert "change_kind" in human_contract["response_contracts"]["context_state"]["fields"]
assert "thread_id" in human_contract["response_contracts"]["context_state"]["fields"]
assert "intake_state" in human_contract["response_contracts"]
assert "accepted" in human_contract["response_contracts"]["intake_state"]["fields"]
assert "provider_state" in human_contract["response_contracts"]
provider_fields = human_contract["response_contracts"]["provider_state"]["fields"]
assert "execution_enabled" in provider_fields
assert "error" in provider_fields
assert "error_category" in provider_fields
assert "execution_ready" not in provider_fields
assert "backend_state" in human_contract["response_contracts"]
assert "ready" in human_contract["response_contracts"]["backend_state"]["fields"]
assert "message" in human_contract["response_contracts"]
assert "text" in human_contract["response_contracts"]["message"]["fields"]
assert "tool_canceled" in human_contract["responses"]
assert "reason" in human_contract["response_contracts"]["tool_canceled"]["fields"]
assert "tool_call" in human_contract["response_contracts"]
tool_fields = human_contract["response_contracts"]["tool_call"]["fields"]
assert "tool" in tool_fields
assert "call_id" in tool_fields
assert "name" not in tool_fields
assert "approval_required" in tool_fields
assert "side_effect" not in tool_fields
snapshots = [item for item in lines if item.get("method") == "context_state_snapshot"]
assert snapshots[0]["params"]["thread_id"] == "ccad-local"
assert snapshots[0]["params"]["revision"] == ""
assert snapshots[-1]["params"]["revision"] == ""
assert snapshots[-1]["params"]["content_emitted"] is False
assert snapshots[-1]["params"]["secret_value_visible"] is False
pending_snapshot = next(item for item in lines
                        if item.get("method") == "pending_calls_state")
assert pending_snapshot["params"]["count"] == 0
assert pending_snapshot["params"]["thread_id"] == "ccad-local"
assert pending_snapshot["params"]["approval_required"] is False
assert pending_snapshot["params"]["approval_reason"] == ""
events = [item for item in lines if item.get("method") == "context_state"]
assert len(events) == 4
assert events[0]["params"]["previous_revision"] == ""
assert events[0]["params"]["change_kind"] == "initial"
assert events[0]["params"]["thread_id"] == "thread-a"
assert events[1]["params"]["previous_revision"] == ""
assert events[1]["params"]["change_kind"] == "initial"
assert events[1]["params"]["thread_id"] == "thread-b"
assert events[2]["params"]["previous_revision"] == events[1]["params"]["revision"]
assert events[2]["params"]["change_kind"] == "unchanged"
assert events[2]["params"]["thread_id"] == "thread-b"
print("PASS agent context history boundary contract; no network")
