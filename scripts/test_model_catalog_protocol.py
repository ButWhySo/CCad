"""Exercise model-catalog protocol branches without provider/network access."""

import json
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env.pop("OPENROUTER_API_KEY", None)
env.pop("CEREBRAS_API_KEY", None)
env.pop("OPENAI_API_KEY", None)
env.pop("ANTHROPIC_API_KEY", None)
env.pop("GEMINI_API_KEY", None)
env.pop("GOOGLE_API_KEY", None)
env["CCAD_PROVIDER"] = "mock"
env["PYTHONNOUSERSITE"] = "1"
result = subprocess.run(
    [sys.executable, str(root / "src" / "ccad_agent" / "orchestrator.py")],
    input=(json.dumps({"method": "agent.methods", "params": {}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "openrouter"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "openai"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "anthropic"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "google_gemini"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "cerebras"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "  CereBras  "}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": 42}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "unknown"}})
           + "\n"),
    text=True,
    capture_output=True,
    env=env,
    timeout=20,
    check=False,
)
assert result.returncode == 0, result.stderr
responses = [json.loads(line) for line in result.stdout.splitlines()
             if line.strip().startswith("{")]
methods = next(item["params"] for item in responses if item.get("method") == "agent_methods")
catalog_method = next(item for item in methods["methods"] if item["name"] == "agent.list_models")
assert catalog_method["network_access"] == "provider_specific"
assert catalog_method["network_access_by_provider"] == {
    "openai": "explicit_refresh", "anthropic": "explicit_refresh",
    "google_gemini": "explicit_refresh", "openrouter": "explicit_refresh",
    "cerebras": "explicit_refresh"}
assert catalog_method["provider_normalization"] == "trim_lowercase"
assert catalog_method["params"]["provider"]["enum"] == [
    "openai", "anthropic", "google_gemini", "openrouter", "cerebras"]
assert {"source", "source_kind", "source_url"}.issubset(catalog_method["response"]["fields"])
assert {"context_window_free", "context_window_paid", "speed_tokens_per_second"}.issubset(
    catalog_method["response"]["model_fields"]
)
assert "reasoning_effort" in catalog_method["response"]["model_fields"]
catalogs = [item["params"] for item in responses if item.get("method") == "provider_models"]
openrouter = next(item for item in catalogs if item["provider"] == "openrouter")
openai = next(item for item in catalogs if item["provider"] == "openai")
anthropic = next(item for item in catalogs if item["provider"] == "anthropic")
gemini = next(item for item in catalogs if item["provider"] == "google_gemini")
cerebras = next(item for item in catalogs if item["provider"] == "cerebras")
canonical_cerebras = [item for item in catalogs
                      if item["provider"] == "cerebras"]
unknown = next(item for item in catalogs if item["provider"] == "unknown")
invalid = next(item for item in catalogs if item.get("error") == "invalid_params")
assert openrouter["ok"] is False
assert openrouter["error"] == "missing_api_key"
assert openrouter["source_kind"] == "provider_api"
assert openrouter["source_url"] == "https://openrouter.ai/api/v1/models"
assert "OPENROUTER_API_KEY" not in json.dumps(openrouter)
for catalog, key_name, source_url in (
        (openai, "OPENAI_API_KEY", "https://api.openai.com/v1/models"),
        (anthropic, "ANTHROPIC_API_KEY", "https://api.anthropic.com/v1/models"),
        (gemini, "GEMINI_API_KEY", "https://generativelanguage.googleapis.com/v1beta/models")):
    assert catalog["ok"] is False
    assert catalog["error"] == "missing_api_key"
    assert catalog["network_access"] == "explicit_refresh"
    assert catalog["source_kind"] == "provider_api"
    assert catalog["source_url"] == source_url
    assert key_name not in json.dumps(catalog)
assert cerebras["ok"] is False
assert cerebras["error"] == "missing_api_key"
assert cerebras["network_access"] == "explicit_refresh"
assert cerebras["source_kind"] == "provider_api"
assert cerebras["source_url"] == "https://api.cerebras.ai/v1/models"
assert "CEREBRAS_API_KEY" not in json.dumps(cerebras)
assert len(canonical_cerebras) == 2
assert unknown["ok"] is False
assert unknown["error"] == "unsupported_provider"
assert invalid["error_detail"] == "provider must be a string"
print("PASS model catalog protocol branches; no network")
