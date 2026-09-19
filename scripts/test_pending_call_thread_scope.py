"""No-network proof: process pending calls stay scoped to their agent thread."""

from pathlib import Path
import queue
import sys


ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))
import orchestrator as ccad  # noqa: E402


with ccad.pending_calls_lock:
    ccad.pending_calls.clear()
    ccad.pending_call_threads.clear()
    ccad.pending_calls.update({"call-a": queue.Queue(), "call-b": queue.Queue()})
    ccad.pending_call_threads.update({"call-a": "thread-a", "call-b": "thread-b"})

old_saver = getattr(ccad, "checkpoint_saver", None)
old_executor = getattr(ccad, "executor", None)
ccad.checkpoint_saver = None
ccad.executor = None
try:
    a = ccad.pending_call_snapshot("thread-a")
    b = ccad.pending_call_snapshot("thread-b")
    assert a["process_call_ids"] == ["call-a"], a
    assert b["process_call_ids"] == ["call-b"], b
    assert a["count"] == b["count"] == 1
finally:
    with ccad.pending_calls_lock:
        ccad.pending_calls.clear()
        ccad.pending_call_threads.clear()
    ccad.checkpoint_saver, ccad.executor = old_saver, old_executor

print("PASS process pending-call thread scope; no network")
