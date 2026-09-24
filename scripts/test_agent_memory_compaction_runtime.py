"""Exercise durable compaction through real agent JSON-RPC; never contacts a provider."""

import json
import os
from pathlib import Path
import re
import queue
import subprocess
import sys
import threading
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ORCHESTRATOR = ROOT / "src" / "ccad_agent" / "orchestrator.py"
agent_python = (ROOT / "src" / "ccad_agent" / "venv" / "Scripts" / "python.exe"
                if os.name == "nt" else
                ROOT / "src" / "ccad_agent" / "venv" / "bin" / "python")
PYTHON = str(agent_python) if agent_python.is_file() else sys.executable

with tempfile.TemporaryDirectory(prefix="ccad-memory-compact-runtime-") as temporary:
    root = Path(temporary)
    appdata = root / "appdata"
    (appdata / "CCad").mkdir(parents=True)
    memory_path = root / "memory.json"
    original = [
        {"id": "fixture-memory-a", "tier": "ltm", "namespace": "compact-thread",
         "scope": "conversation", "title": "PCB constraint A",
         "content": "Preserve J3 placement and its existing ground return path. "
                    "Keep minimum copper clearance at 0.25 mm on F.Cu; do not move U3. "
                    "This durable record is fixture data used only in this isolated test.",
         "tags": ["pcb", "clearance"],
         "created_at": "2026-09-20T10:00:00+00:00", "expires_at": ""},
        {"id": "fixture-memory-b", "tier": "ltm", "namespace": "compact-thread",
         "scope": "conversation", "title": "PCB constraint B",
         "content": "Preserve the current board outline and all existing via locations. "
                    "Keep J3 placement fixed; maintain 0.25 mm copper clearance on F.Cu "
                    "and preserve U3 ground return. This is isolated fixture data only.",
         "tags": ["pcb", "outline"],
         "created_at": "2026-09-21T10:00:00+00:00", "expires_at": ""},
    ]
    memory_path.write_text(json.dumps(original, indent=2) + "\n", encoding="utf-8")
    original_bytes = memory_path.read_bytes()
    (appdata / "CCad" / "agent_config.json").write_text(json.dumps({
        "provider": "openai", "model": "gpt-5.1",
        "memory": {"stm": False, "ltm": True, "episodic": False},
        "observability": {"enabled": False, "backend": "langfuse"},
    }), encoding="utf-8")
    env = os.environ.copy()
    env.update({
        "APPDATA": str(appdata),
        "CCAD_AGENT_MEMORY_PATH": str(memory_path),
        "CCAD_AGENT_DEFER_PROVIDER_INIT": "1",
        "CCAD_AGENT_THREAD_ID": "compact-thread",
        "PYTHONNOUSERSITE": "1",
    })
    env.pop("OPENAI_API_KEY", None)
    process = subprocess.Popen(
        [PYTHON, str(ORCHESTRATOR)], stdin=subprocess.PIPE,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        bufsize=1, cwd=ROOT / "src" / "ccad_agent", env=env)
    output_lines = queue.Queue()

    def collect_stdout():
        for line in process.stdout:
            output_lines.put(line)
        output_lines.put(None)

    reader = threading.Thread(target=collect_stdout, daemon=True)
    reader.start()

    def send(request):
        process.stdin.write(json.dumps(request) + "\n")
        process.stdin.flush()

    def wait_for(method, *, kind="", stage=""):
        collected = []
        while True:
            try:
                line = output_lines.get(timeout=10)
            except queue.Empty as error:
                raise AssertionError(
                    f"agent did not emit {method}/{kind}/{stage}; "
                    f"process_status={process.poll()}, events={collected[-20:]}") from error
            if line is None:
                raise AssertionError(f"agent exited before {method}: {collected[-20:]}")
            if not line.startswith("{"):
                continue
            event = json.loads(line)
            collected.append(event)
            params = event.get("params", {})
            if (event.get("method") == method
                    and (not kind or params.get("kind") == kind)
                    and (not stage or params.get("stage") == stage)):
                return params, collected

    send({"jsonrpc": "2.0", "method": "human_message", "params": {
        "text": "/memory compact plan tier:ltm scope:conversation",
        "thread_id": "compact-thread", "session_id": "compact-session",
        "context": "{}"}})
    plan, events = wait_for("message", kind="memory_compaction_plan")
    match = re.search(r"/memory compact send:([0-9a-f]{32})", plan["text"])
    assert match, plan
    state = next(event["params"] for event in events
                 if event.get("method") == "memory_compaction_state"
                 and event.get("params", {}).get("stage") == "planned")
    assert state["source_record_count"] == 2
    assert state["source_contents_emitted"] is False
    assert not any(event.get("method") == "tool_call" for event in events)
    send({"jsonrpc": "2.0", "method": "human_message", "params": {
        "text": f"/memory compact send:{match.group(1)}",
        "thread_id": "compact-thread", "session_id": "compact-session",
        "context": "{}"}})
    refused, send_events = wait_for("message", kind="memory_compaction_refused")
    assert "provider_request_sent" not in refused or refused["provider_request_sent"] is False
    assert any(event.get("params", {}).get("provider_request_sent") is False
               for event in send_events if event.get("method") == "message")
    send({"jsonrpc": "2.0", "method": "human_message", "params": {
        "text": f"/memory compact cancel:{match.group(1)}",
        "thread_id": "compact-thread", "session_id": "compact-session",
        "context": "{}"}})
    _, cancel_events = wait_for("message", kind="memory_compaction_cancelled")
    assert any(event.get("method") == "memory_compaction_state"
               and event.get("params", {}).get("stage") == "cancelled"
               for event in cancel_events)
    process.stdin.close()
    assert process.wait(timeout=15) == 0
    reader.join(timeout=5)
    stderr = process.stderr.read()
    stdout = "\n".join([json.dumps(event)
                         for event in events + send_events + cancel_events])
    assert memory_path.read_bytes() == original_bytes
    assert "fixture-memory-a" not in stdout
    assert "This durable record is fixture data" not in stdout
    assert "This durable record is fixture data" not in stderr

print("PASS real JSON-RPC compaction planning, process-local plan safety, and no provider call")
