"""No-network contract test for CCad's OpenAI-compatible provider boundary."""

import json
import os
import subprocess
import sys
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
requests = []


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):  # noqa: N802 - stdlib HTTPServer hook
        length = int(self.headers.get("Content-Length", "0"))
        requests.append(json.loads(self.rfile.read(length)))
        body = requests[-1]
        if len(requests) == 1:
            self.send_response(503)
            self.end_headers()
            return
        system = " ".join(
            str(message.get("content", ""))
            for message in body.get("messages", [])
            if message.get("role") == "system"
        )
        if "supervisor managing" in system.lower():
            message = {"role": "assistant", "content": "router"}
            finish = "stop"
        else:
            message = {
                "role": "assistant",
                "content": None,
                "tool_calls": [{
                    "id": "stub-call-1",
                    "type": "function",
                    "function": {
                        "name": "ui_place_via",
                        "arguments": json.dumps({"x_mm": 10.0, "y_mm": 10.0, "dry_run": True}),
                    },
                }],
            }
            finish = "tool_calls"
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

    def log_message(self, *_args):
        return


def main():
    server = HTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    env = os.environ.copy()
    env.update({
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
        process.stdin.write(json.dumps({"method": "human_message", "params": {
            "text": "Routing Expert: place one via using approved route.",
            "context": "blank board with F.Cu",
        }}) + "\n")
        process.stdin.flush()
        lines = []
        for _ in range(30):
            line = process.stdout.readline()
            if not line:
                break
            lines.append(json.loads(line))
            if any(item.get("method") == "tool_call" for item in lines):
                break
        assert len(requests) >= 3, "transient failure or router request was not retried"
        router_request = next(
            request for request in requests
            if any(tool.get("function", {}).get("name") == "ui_place_via"
                   for tool in request.get("tools", [])))
        assert router_request["model"] == "ccad-local-stub"
        tool_names = {
            tool["function"]["name"]
            for tool in router_request.get("tools", [])
            if tool.get("type") == "function"
        }
        assert "ui_place_via" in tool_names, "router tool schema missing"
        assert any(item.get("method") == "tool_call" for item in lines), \
            "provider tool call did not reach CCad protocol"
        context_events = [item for item in lines if item.get("method") == "context_state"]
        assert context_events, "context revision event missing"
        assert context_events[0]["params"]["content_emitted"] is False
        assert len(context_events[0]["params"]["revision"]) == 16
        assert context_events[0]["params"]["original_content_size"] >= context_events[0]["params"]["content_size"]
        assert context_events[0]["params"]["context_limit"] >= context_events[0]["params"]["content_size"]
        assert context_events[0]["params"]["truncated"] is False
        print("PASS local OpenAI-compatible provider tool boundary")
    finally:
        process.kill()
        process.wait(timeout=5)
        server.shutdown()


if __name__ == "__main__":
    main()
