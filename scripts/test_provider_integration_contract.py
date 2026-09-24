"""Static provider integration contract; network-free and documentation-backed."""

from pathlib import Path


source = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
compatibility = (Path(__file__).parents[1] / "docs" / "research" / "provider-model-compatibility.md").read_text(encoding="utf-8")
compatibility_text = " ".join(compatibility.split())
contracts = {
    "openai": ("https://api.openai.com/v1/models", "OPENAI_API_KEY", "ChatOpenAI"),
    "anthropic": ("https://api.anthropic.com/v1/models", "ANTHROPIC_API_KEY", "ChatAnthropic"),
    "google_gemini": ("https://generativelanguage.googleapis.com/v1beta/models", "GEMINI_API_KEY", "ChatGoogleGenerativeAI"),
    "openrouter": ("https://openrouter.ai/api/v1/models", "OPENROUTER_API_KEY", "ChatOpenAI"),
    "cerebras": ("https://api.cerebras.ai/public/v1/models", "CEREBRAS_API_KEY", "ChatOpenAI"),
    "ollama": ("/api/tags", "CCAD_OLLAMA_BASE_URL", "ChatOpenAI"),
}
for provider, (endpoint, credential, adapter) in contracts.items():
    assert endpoint in source, provider
    assert adapter in source, provider
    assert credential in source, provider
    assert f'catalog_failure("{provider}"' in source or provider == "ollama"
for category in ("authentication", "permission_denied", "payment_required",
                 "quota_or_rate_limit", "rate_limited", "timeout", "connection_error", "invalid_response"):
    assert f'"{category}"' in source
assert '"source_kind": "local_provider_api"' in source
assert '"network_access": "explicit_refresh"' in source
assert 'model_name = model_name or "openrouter/free"' in source
assert "defaults to `openrouter/free`" in compatibility_text
assert "openrouter/auto` is not the runtime default" in compatibility_text
assert "https://api.cerebras.ai/public/v1/models" in source
assert "https://api.cerebras.ai/public/v1/models" in compatibility
assert "generic gRPC `RESOURCE_EXHAUSTED` can mean either quota exhaustion or" in compatibility_text
assert "server/model capability remains unknown" in compatibility_text
print("PASS provider integration contract for six documented adapters; no network")
