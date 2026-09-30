"""Cerebras catalog is public, bounded, and does not attach credentials."""

import json
import os
import sys
from pathlib import Path
from unittest.mock import patch


root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
import orchestrator  # noqa: E402


class Response:
    def __init__(self, payload):
        self.payload = json.dumps(payload).encode("utf-8")

    def read(self):
        return self.payload

    def __enter__(self):
        return self

    def __exit__(self, *_args):
        return False


captured = []
payload = {"data": [{"id": "test-model", "owned_by": "cerebras",
                    "limits": {"max_context_length": 4096},
                    "capabilities": {"tools": True, "untrusted": "discard"}}]}
previous_key = os.environ.get("CEREBRAS_API_KEY")
os.environ["CEREBRAS_API_KEY"] = "must-not-be-sent"
try:
    def receive(request, timeout):
        captured.append((request, timeout))
        return Response(payload)

    with patch.object(orchestrator.urllib.request, "urlopen", side_effect=receive):
        result = orchestrator.fetch_cerebras_models()
finally:
    if previous_key is None:
        os.environ.pop("CEREBRAS_API_KEY", None)
    else:
        os.environ["CEREBRAS_API_KEY"] = previous_key

assert result["ok"] is True and result["network_access"] == "explicit_refresh"
assert result["source_kind"] == "provider_api"
assert result["models"][0]["context_length"] == 4096
assert result["models"][0]["capabilities"] == {"tools": True}
request, timeout = captured[0]
assert request.full_url == "https://api.cerebras.ai/public/v1/models"
assert request.get_header("Authorization") is None
assert request.get_header("User-agent") == "CCad/1.0 (+https://github.com/ButWhySo/CCad)"
assert 2 <= timeout <= 20
print("PASS Cerebras public catalog uses bounded unauthenticated refresh; no live network")
