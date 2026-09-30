"""Runtime contract for safe pending-call metadata; performs no provider call."""

import queue
import sys
from pathlib import Path


root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
import orchestrator  # noqa: E402
from method_catalog import orchestrator_method_catalog  # noqa: E402

call_id = "opaque-test-call"
thread_id = "pending-test-thread"
with orchestrator.pending_calls_lock:
    original_calls = dict(orchestrator.pending_calls)
    original_threads = dict(orchestrator.pending_call_threads)
    orchestrator.pending_calls.clear()
    orchestrator.pending_call_threads.clear()
    orchestrator.pending_calls[call_id] = queue.Queue()
    orchestrator.pending_call_threads[call_id] = thread_id
try:
    pending = orchestrator.pending_call_snapshot(thread_id)
    empty = orchestrator.pending_call_snapshot("unrelated-thread")
finally:
    with orchestrator.pending_calls_lock:
        orchestrator.pending_calls.clear()
        orchestrator.pending_calls.update(original_calls)
        orchestrator.pending_call_threads.clear()
        orchestrator.pending_call_threads.update(original_threads)

assert pending["process_call_ids"] == [call_id]
assert pending["count"] == 1 and pending["approval_required"] is True
assert pending["approval_reason"] == "project_mutation"
assert pending["secret_value_visible"] is False
assert empty["count"] == 0 and empty["approval_required"] is False
method = next(entry for entry in orchestrator_method_catalog()["methods"]
              if entry["name"] == "agent.pending_calls")
assert method["response"]["fields"] == [
    "thread_id", "process_call_ids", "checkpoint_call_ids", "count",
    "approval_required", "approval_reason", "secret_value_visible"]
print("PASS pending-call approval metadata contract; no network")
