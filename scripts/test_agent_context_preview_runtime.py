"""Exercise local context preview through the real Python agent process."""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ORCHESTRATOR = ROOT / "src" / "ccad_agent" / "orchestrator.py"
AGENT_PYTHON = (ROOT / "src" / "ccad_agent" / "venv" / "Scripts" / "python.exe"
                if os.name == "nt" else
                ROOT / "src" / "ccad_agent" / "venv" / "bin" / "python")
PYTHON = str(AGENT_PYTHON) if AGENT_PYTHON.is_file() else sys.executable

with tempfile.TemporaryDirectory(prefix="ccad-context-preview-") as temporary:
    temp = Path(temporary)
    appdata = temp / "appdata"
    config_dir = appdata / "CCad"
    config_dir.mkdir(parents=True)
    env = os.environ.copy()
    env.update({
        "APPDATA": str(appdata),
        "CCAD_AGENT_MEMORY_PATH": str(temp / "memory.json"),
        "CCAD_AGENT_DEFER_PROVIDER_INIT": "1",
        "CCAD_AGENT_LARGE_CONTEXT_TOKENS": "512",
        "CCAD_TRACE_DEBUG": "1",
        "CCAD_PROVIDER": "",
        "CCAD_MODEL": "",
        "CCAD_OPENROUTER_MODEL": "",
        "PYTHONNOUSERSITE": "1",
    })
    # The large real project snapshot makes the normal accounting path cross
    # its configured threshold; this process has no initialized model.
    private_marker = "CONTEXT_PREVIEW_PRIVATE_DESIGN_SENTINEL"
    (config_dir / "agent_config.json").write_text(json.dumps({
        "provider": "openrouter",
        "model": "openrouter/free",
        "personalisation": {"custom_instructions":
                            (private_marker + " private-instruction ") * 1600},
    }), encoding="utf-8")
    context = json.dumps({
        "schema_version": 1,
        "context_kind": "ccad_agent_context",
        "revision": "preview-revision-1",
        "project": {"notes": (private_marker + " design-state ") * 1400},
    })
    requests = [
        {"jsonrpc": "2.0", "method": "human_message", "params": {
            "text": "/context preview Preserve the connector clearance around U3",
            "context": context,
            "session_id": "preview-session",
            "thread_id": "preview-thread",
            "project_id": "preview-project",
        }},
        {"jsonrpc": "2.0", "method": "human_message", "params": {
            "text": "/commands", "context": "{}",
            "session_id": "preview-session", "thread_id": "preview-thread",
            "project_id": "preview-project",
        }},
        {"jsonrpc": "2.0", "method": "human_message", "params": {
            "text": "hello", "context": "{}",
            "session_id": "preview-session", "thread_id": "preview-thread",
            "project_id": "preview-project",
        }},
    ]
    wire = "\n".join(json.dumps(request) for request in requests) + "\n"
    result = subprocess.run(
        [PYTHON, str(ORCHESTRATOR)], input=wire, text=True,
        capture_output=True, timeout=45, cwd=ROOT / "src" / "ccad_agent",
        env=env, check=False)
    if result.returncode != 0:
        raise AssertionError(
            f"agent process exit={result.returncode}; stderr={result.stderr[-4000:]}")

    events = [json.loads(line) for line in result.stdout.splitlines()
              if line.startswith("{")]
    previews = [event["params"] for event in events
                if event.get("method") == "provider_request_context"
                and event.get("params", {}).get("request_mode") == "local_preview"]
    assert len(previews) == 1, result.stdout[-4000:]
    report = previews[0]
    assert report["large_context"]
    assert report["provider"] == "openrouter"
    assert report["model"] == "openrouter/free"
    assert report["model_context_limit_source"] == "unavailable"
    assert report["provider_request_sent"] is False
    assert report["preview_prompt_included"] is True
    assert report["preview_prompt_chars"] == len(
        "Preserve the connector clearance around U3")
    preview_messages = [event["params"] for event in events
                        if event.get("method") == "message"
                        and event.get("params", {}).get("kind") == "context_preview"]
    assert len(preview_messages) == 1
    assert preview_messages[0]["provider_request_sent"] is False
    assert preview_messages[0]["tool_executed"] is False
    assert "Local context preview (large)" in preview_messages[0]["text"]
    assert "no provider request was sent" in preview_messages[0]["text"]
    assert "Preserve the connector clearance" not in preview_messages[0]["text"]
    assert private_marker not in result.stdout
    assert private_marker not in result.stderr
    assert "[ccad-context-preview]" in result.stderr
    assert not any(event.get("method") == "tool_call" for event in events)
    assert any(event.get("method") == "backend_state"
               and not event["params"]["provider_initialized"] for event in events)
    command_help = [event["params"]["text"] for event in events
                    if event.get("method") == "message"
                    and "/context [draft]" in event.get("params", {}).get("text", "")]
    assert command_help, "context command missing from canonical help output"
    states = [event["params"] for event in events
              if event.get("method") == "context_state"]
    assert len(states) == 3 and all(state["history_message_count"] == 0
                                    for state in states)

print("PASS live local context preview uses real bounded assembly; no provider/tool call")
