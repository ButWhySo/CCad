"""No-network contract test for CCad's OpenAI-compatible provider boundary."""

import json
import os
import re
import subprocess
import sys
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
requests = []
tool_responses_sent = 0


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):  # noqa: N802 - stdlib HTTPServer hook
        global tool_responses_sent
        length = int(self.headers.get("Content-Length", "0"))
        requests.append(json.loads(self.rfile.read(length)))
        body = requests[-1]
        system = " ".join(
            str(message.get("content", ""))
            for message in body.get("messages", [])
            if message.get("role") == "system"
        )
        if "supervisor managing" in system.lower():
            message = {"role": "assistant", "content": "router"}
            finish = "stop"
        elif tool_responses_sent == 0:
            tool_responses_sent += 1
            message = {
                "role": "assistant",
                "content": None,
                "tool_calls": [{
                    "id": "stub-call-1",
                    "type": "function",
                    "function": {
                        "name": "ccad_ui_place_via",
                        "arguments": json.dumps({"x_mm": 10.0, "y_mm": 10.0, "dry_run": True}),
                    },
                }],
            }
            finish = "tool_calls"
        else:
            # Let the graph finish after the protocol emitted its tool request;
            # this test has no native broker and must not fabricate execution.
            message = {"role": "assistant", "content": "Tool request was forwarded to CCad."}
            finish = "stop"
        response = {"id": "stub-chat", "object": "chat.completion", "created": 1,
                    "model": body.get("model", "stub"),
                    "choices": [{"index": 0, "message": message, "finish_reason": finish}],
                    "usage": {"prompt_tokens": 1, "completion_tokens": 1, "total_tokens": 2}}
        encoded = json.dumps(response).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def log_message(self, format: str, *args: object) -> None:
        return


def main():
    temporary_data = tempfile.TemporaryDirectory(prefix="ccad-openai-provider-contract-")
    isolated_root = Path(temporary_data.name)
    server = HTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    env = {name: os.environ[name] for name in
           ("PATH", "SYSTEMROOT", "WINDIR", "TEMP", "TMP", "HOME")
           if name in os.environ}
    env.update({
        "APPDATA": str(isolated_root),
        "CCAD_AGENT_CONVERSATION_DB": str(isolated_root / "conversations.sqlite3"),
        "CCAD_AGENT_MEMORY_PATH": str(isolated_root / "memory.json"),
        "CCAD_PROVIDER": "openai_compatible",
        # Unrelated credentials must not override explicit provider choice.
        "ANTHROPIC_API_KEY": "wrong-provider-anthropic-key",
        "GEMINI_API_KEY": "wrong-provider-gemini-key",
        "CCAD_OPENAI_COMPATIBLE_API_KEY": "sk-ccad-local-stub",
        "CCAD_OPENAI_COMPATIBLE_BASE_URL": f"http://127.0.0.1:{server.server_port}/v1",
        "CCAD_OPENAI_COMPATIBLE_MODEL": "ccad-local-stub",
        "PYTHONPATH": str(ROOT / "src" / "ccad_agent"),
    })
    process = subprocess.Popen(
        [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, env=env,
    )
    try:
        try:
            tool_catalog = [{
                "method": "ui.place_via",
                "description": "Place a via at a board coordinate.",
                "read_only": False,
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "x_mm": {"type": "number", "description": "X coordinate in mm."},
                        "y_mm": {"type": "number", "description": "Y coordinate in mm."},
                        "dry_run": {"type": "boolean", "description": "Preview only."},
                    },
                    "required": ["x_mm", "y_mm"],
                },
            }]
            stdout, stderr = process.communicate(
                input=(json.dumps({"method": "agent.set_tool_catalog",
                                   "params": {"catalog": tool_catalog}}) + "\n" +
                       json.dumps({"method": "human_message", "params": {
                           "text": "Routing Expert: place one via using approved route.",
                           "context": "blank board with F.Cu",
                       }}) + "\n"),
                timeout=45,
            )
        except subprocess.TimeoutExpired as error:
            process.kill()
            stdout, stderr = process.communicate()
            raise AssertionError(
                f"local provider turn did not terminate after stdin EOF; stderr={stderr!r}"
            ) from error
        assert process.returncode == 0, f"orchestrator failed: {stderr}"
        lines = [json.loads(line) for line in stdout.splitlines()
                 if line.strip().startswith("{")]
        assert requests, f"provider endpoint received no request; stdout={stdout!r}"
        router_request = next((
            request for request in requests
            if any(tool.get("function", {}).get("name") == "ccad_ui_place_via"
                   for tool in request.get("tools", []))), None)
        assert router_request is not None, (
            f"router tool schema missing; requests={json.dumps(requests, sort_keys=True)}; "
            f"stdout={stdout!r}"
        )
        assert router_request["model"] == "ccad-local-stub"
        tool_names = {
            tool["function"]["name"]
            for tool in router_request.get("tools", [])
            if tool.get("type") == "function"
        }
        assert "ccad_ui_place_via" in tool_names, "router tool schema missing"
        assert any(item.get("method") == "tool_call" for item in lines), \
            "provider tool call did not reach CCad protocol"
        assert not any(item.get("params", {}).get("error_type") == "GraphRecursionError"
                       for item in lines), "local provider conversation did not converge"
        context_events = [item for item in lines if item.get("method") == "context_state"]
        assert context_events, "context revision event missing"
        assert context_events[0]["params"]["content_emitted"] is False
        assert re.fullmatch(r"[0-9a-f]{24}", context_events[0]["params"]["revision"])
        assert context_events[0]["params"]["content_size"] >= context_events[0]["params"]["original_content_size"]
        assert context_events[0]["params"]["context_limit"] >= context_events[0]["params"]["content_size"]
        assert context_events[0]["params"]["truncated"] is False
        print("PASS local OpenAI-compatible provider tool boundary")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=5)
        server.shutdown()
        server.server_close()
        temporary_data.cleanup()


if __name__ == "__main__":
    main()
