"""No-network OpenAI-compatible provider -> CCad approval integration test."""

import json
import os
import subprocess
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class Handler(BaseHTTPRequestHandler):
    calls = 0
    tool_sent = False

    def do_POST(self):  # noqa: N802
        size = int(self.headers.get("Content-Length", "0"))
        request = json.loads(self.rfile.read(size))
        Handler.calls += 1
        system = " ".join(str(m.get("content", "")) for m in request.get("messages", [])
                           if m.get("role") == "system")
        if "supervisor managing" in system.lower():
            message, finish = {"role": "assistant", "content": "router"}, "stop"
        elif not Handler.tool_sent:
            Handler.tool_sent = True
            message, finish = {
                "role": "assistant", "content": None,
                "tool_calls": [{"id": "gui-stub-call-1", "type": "function",
                                "function": {"name": "ui_place_via", "arguments": json.dumps(
                                    {"x_mm": 10.0, "y_mm": 10.0, "dry_run": False})}}],
            }, "tool_calls"
        else:
            message, finish = {"role": "assistant", "content": "Routing change approved and applied."}, "stop"
        payload = json.dumps({"id": "gui-stub", "object": "chat.completion", "created": 1,
                              "model": request.get("model", "ccad-gui-stub"),
                              "choices": [{"index": 0, "message": message,
                                            "finish_reason": finish}]}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, *_args):
        return


def main():
    server = HTTPServer(("127.0.0.1", 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    env = os.environ.copy()
    env.update({
        "CCAD_PROVIDER": "openai_compatible",
        "CCAD_OPENAI_COMPATIBLE_API_KEY": "sk-ccad-gui-stub",
        "CCAD_OPENAI_COMPATIBLE_BASE_URL": f"http://127.0.0.1:{server.server_port}/v1",
        "CCAD_OPENAI_COMPATIBLE_MODEL": "ccad-gui-stub",
    })
    command = ["python", str(ROOT / "scripts" / "live_agent_route_demo.py"),
               "--provider", "openai_compatible", "--mock-tool-approval-check",
               "--hold-seconds", "1", "--delay", "0.01"]
    log_path = ROOT / "artifacts" / "provider-gui-approval.log"
    log_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        # The GUI child inherits stdout while alive; pipe capture would wait
        # for EOF after the harness completed. File capture retains evidence.
        with log_path.open("w", encoding="utf-8") as log:
            result = subprocess.run(command, cwd=ROOT, env=env, text=True,
                                    stdout=log, stderr=subprocess.STDOUT, timeout=90)
        output = log_path.read_text(encoding="utf-8")
        if result.returncode:
            raise RuntimeError(output + f"stub_calls={Handler.calls}")
        assert "LIVE APPROVED MUTATION" in output
        assert '"pending_visible":true' in output
        assert '"via_count":1' in output
        assert Handler.calls >= 2, "stub did not receive supervisor and router calls"
        print("PASS local provider GUI approval roundtrip")
    finally:
        server.shutdown()


if __name__ == "__main__":
    main()
