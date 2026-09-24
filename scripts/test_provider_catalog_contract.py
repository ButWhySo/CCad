"""No-network contract for official provider catalog refresh endpoints."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
source = "\n".join((root / "src" / "ccad_agent" / name).read_text(encoding="utf-8")
                   for name in ("orchestrator.py", "model_catalog.py", "method_catalog.py"))
contracts = {
    "openai": ("fetch_openai_models", "https://api.openai.com/v1/models", "OPENAI_API_KEY"),
    "anthropic": ("fetch_anthropic_models", "https://api.anthropic.com/v1/models", "ANTHROPIC_API_KEY"),
    "google_gemini": ("fetch_gemini_models", "https://generativelanguage.googleapis.com/v1beta/models", "GEMINI_API_KEY"),
}
for provider, (function, endpoint, key) in contracts.items():
    assert f"def {function}():" in source
    assert endpoint in source
    assert key in source
    assert f'provider_id == "{provider}"' in source
assert '"anthropic-version": "2023-06-01"' in source
assert '"x-goog-api-key": key' in source
assert '"generateContent"' in source
assert '"error": "invalid_catalog_shape"' in source
assert "never called at startup" in source
assert "https://api.cerebras.ai/public/v1/models" in source
assert '"User-Agent": "CCad/1.0 (+https://github.com/ButWhySo/CCad)"' in source
print("PASS OpenAI, Anthropic, and Gemini catalog contracts; no network")
