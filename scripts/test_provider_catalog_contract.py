"""Supported explicit model refreshes dispatch through their provider clients."""

import sys
from pathlib import Path
from unittest.mock import patch


root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
import orchestrator  # noqa: E402

providers = {
    "openai": "fetch_openai_models",
    "anthropic": "fetch_anthropic_models",
    "google_gemini": "fetch_gemini_models",
    "openrouter": "fetch_openrouter_models",
    "cerebras": "fetch_cerebras_models",
    "ollama": "fetch_ollama_models",
}
for provider, function_name in providers.items():
    events = []
    result = {"ok": True, "models": [], "count": 0,
              "network_access": "explicit_refresh"}
    with patch.object(orchestrator, function_name, return_value=result) as fetch, \
            patch.object(orchestrator, "emit", side_effect=events.append):
        handled = orchestrator.handle_provider_and_state_request(
            {"method": "agent.list_models", "params": {"provider": provider}},
            executor=None)
    assert handled is True
    fetch.assert_called_once_with()
    assert events[0]["method"] == "provider_models"
    assert events[0]["params"]["provider"] == provider
    assert events[0]["params"]["network_access"] == "explicit_refresh"

events = []
with patch.object(orchestrator, "emit", side_effect=events.append):
    handled = orchestrator.handle_provider_and_state_request(
        {"method": "agent.list_models", "params": {"provider": "unknown"}},
        executor=None)
assert handled is True
assert events[0]["params"]["error"] == "unsupported_provider"
print("PASS provider catalog dispatch is explicit and supported; no network")
