"""Exercise every provider catalog parser against controlled API responses."""

import importlib.util
import json
import os
import sys
from pathlib import Path
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))
spec = importlib.util.spec_from_file_location("ccad_orchestrator_catalog", ROOT / "src" / "ccad_agent" / "orchestrator.py")
orchestrator = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(orchestrator)


class Response:
    def __init__(self, payload):
        self.payload = json.dumps(payload).encode("utf-8")

    def read(self):
        return self.payload

    def __enter__(self):
        return self

    def __exit__(self, *_args):
        return False


def invoke(function, key_name, key, payload):
    captured = []
    previous = os.environ.get(key_name)
    os.environ[key_name] = key
    try:
        with patch.object(orchestrator.urllib.request, "urlopen", side_effect=lambda request, timeout: (captured.append((request, timeout)) or Response(payload))):
            result = function()
    finally:
        if previous is None:
            os.environ.pop(key_name, None)
        else:
            os.environ[key_name] = previous
    assert len(captured) == 1
    return result, captured[0][0]


openai, request = invoke(orchestrator.fetch_openai_models, "OPENAI_API_KEY", "openai-test", {
    "data": [{"id": "gpt-test", "owned_by": "openai"}]})
assert openai["ok"] and openai["models"][0]["id"] == "gpt-test"
assert request.get_header("Authorization") == "Bearer openai-test"

anthropic, request = invoke(orchestrator.fetch_anthropic_models, "ANTHROPIC_API_KEY", "anthropic-test", {
    "data": [{"id": "claude-test", "display_name": "Claude Test"}]})
assert anthropic["ok"] and anthropic["models"][0]["display_name"] == "Claude Test"
assert request.get_header("X-api-key") == "anthropic-test"
assert request.get_header("Anthropic-version") == "2023-06-01"

gemini, request = invoke(orchestrator.fetch_gemini_models, "GEMINI_API_KEY", "gemini-test", {
    "models": [{"name": "models/gemini-test", "displayName": "Gemini Test",
                "supportedGenerationMethods": ["generateContent"]},
               {"name": "models/embed-test", "supportedGenerationMethods": ["embedContent"]}]})
assert gemini["ok"] and [model["id"] for model in gemini["models"]] == ["gemini-test"]
assert request.get_header("X-goog-api-key") == "gemini-test"

openrouter, request = invoke(orchestrator.fetch_openrouter_models, "OPENROUTER_API_KEY", "router-test", {
    "data": [{"id": "provider/model", "name": "Model"}]})
assert openrouter["ok"] and openrouter["models"][0]["id"] == "provider/model"
assert request.get_header("Authorization") == "Bearer router-test"

cerebras, request = invoke(orchestrator.fetch_cerebras_models, "CEREBRAS_API_KEY", "cerebras-test", {
    "data": [{"id": "qwen-3.8-27b", "owned_by": "cerebras"}]})
assert cerebras["ok"] and cerebras["models"][0]["id"] == "qwen-3.8-27b"
assert request.get_header("Authorization") == "Bearer cerebras-test"

for function, key_name in ((orchestrator.fetch_openai_models, "OPENAI_API_KEY"),
                           (orchestrator.fetch_anthropic_models, "ANTHROPIC_API_KEY"),
                           (orchestrator.fetch_gemini_models, "GEMINI_API_KEY")):
    result, _ = invoke(function, key_name, "shape-test", [])
    assert result["ok"] is False and result["error"] == "invalid_catalog_shape"

print("PASS provider catalog parsers and auth headers; controlled local responses only")
