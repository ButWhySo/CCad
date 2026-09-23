"""Static provider integration contract; network-free and documentation-backed."""

from pathlib import Path


source = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
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
                 "rate_limited", "timeout", "connection_error", "invalid_response"):
    assert f'"{category}"' in source
assert '"source_kind": "local_provider_api"' in source
assert '"network_access": "explicit_refresh"' in source
print("PASS provider integration contract for six documented adapters; no network")
