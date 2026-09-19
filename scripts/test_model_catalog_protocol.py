"""Exercise model-catalog protocol branches without provider/network access."""

import json
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env.pop("OPENROUTER_API_KEY", None)
env["CCAD_PROVIDER"] = "mock"
env["PYTHONNOUSERSITE"] = "1"
result = subprocess.run(
    [sys.executable, str(root / "src" / "ccad_agent" / "orchestrator.py")],
    input=(json.dumps({"method": "agent.methods", "params": {}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "openrouter"}})
           + "\n"
           + json.dumps({"method": "agent.list_models", "params": {"provider": "cerebras"}})
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
assert catalog_method["network_access_by_provider"] == {"openrouter": "explicit_refresh", "cerebras": "none"}
assert catalog_method["params"]["provider"]["enum"] == ["openrouter", "cerebras"]
assert {"source", "source_kind", "source_url"}.issubset(catalog_method["response"]["fields"])
catalogs = [item["params"] for item in responses if item.get("method") == "provider_models"]
openrouter = next(item for item in catalogs if item["provider"] == "openrouter")
cerebras = next(item for item in catalogs if item["provider"] == "cerebras")
unknown = next(item for item in catalogs if item["provider"] == "unknown")
assert openrouter["ok"] is False
assert openrouter["error"] == "missing_api_key"
assert openrouter["source_kind"] == "provider_api"
assert openrouter["source_url"] == "https://openrouter.ai/api/v1/models"
assert "OPENROUTER_API_KEY" not in json.dumps(openrouter)
assert cerebras["ok"] is True
assert cerebras["network_access"] == "none"
assert cerebras["source"] == "official_curated_snapshot"
assert cerebras["source_kind"] == "first_party_documentation"
assert cerebras["source_url"] == "https://inference-docs.cerebras.ai/models/overview"
assert {item["id"] for item in cerebras["models"]} == {"gpt-oss-120b", "qwen-3.8-27b"}
assert unknown["ok"] is False
assert unknown["error"] == "unsupported_provider"
print("PASS model catalog protocol branches; no network")
