"""Contracts for deriving bounded context allocations from catalog metadata."""

import importlib.util
from pathlib import Path


MODULE = Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "model_context_budget.py"
SPEC = importlib.util.spec_from_file_location("ccad_model_context_budget", MODULE)
assert SPEC is not None and SPEC.loader is not None
BUDGET = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUDGET)


def test_catalog_context_lengths_are_validated_and_keyed_by_exact_model():
    models = [
        {"id": "gemini-2.5-flash", "context_length": 1_000_000},
        {"id": "model-bool", "context_length": True},
        {"id": "model-string", "context_length": "8192"},
        {"id": "model-negative", "context_length": -1},
        {"id": "model-too-large", "context_length": 100_000_001},
        {"id": "model-missing"},
        {"id": "", "context_length": 4096},
    ]
    assert BUDGET.catalog_context_limits("google_gemini", models) == {
        ("google_gemini", "gemini-2.5-flash"): 1_000_000}


def test_context_package_budget_scales_down_for_small_models_and_is_capped():
    assert BUDGET.context_package_char_budget(8192) == 8192
    assert BUDGET.context_package_char_budget(32768) == 32768
    assert BUDGET.context_package_char_budget(200_000) == 32768
    assert BUDGET.context_package_char_budget(1024) == 1024


def test_unknown_or_invalid_limit_uses_existing_conservative_fallback():
    for value in (None, True, 0, -20, "8192", 100_000_001):
        assert BUDGET.context_package_char_budget(value) == 32768


def test_model_limit_lookup_does_not_cross_provider_or_model_identity():
    limits = BUDGET.catalog_context_limits("openrouter", [
        {"id": "provider/model-a", "context_length": 128_000},
        {"id": "provider/model-b", "context_length": 64_000},
    ])
    assert BUDGET.lookup_context_limit(limits, "openrouter", "provider/model-a") == 128_000
    assert BUDGET.lookup_context_limit(limits, "openrouter", "provider/model-c") is None
    assert BUDGET.lookup_context_limit(limits, "openai", "provider/model-a") is None


def test_registry_uses_only_latest_explicit_successful_catalog_per_provider():
    registry = BUDGET.ModelContextLimitRegistry(max_entries=2)
    registry.replace_provider_catalog("google_gemini", {
        "ok": True, "models": [
            {"id": "gemini-a", "context_length": 128_000},
            {"id": "gemini-b", "context_length": 64_000}]})
    registry.replace_provider_catalog("openrouter", {
        "ok": True, "models": [{"id": "route/model", "context_length": 32_000}]})
    assert registry.get("google_gemini", "gemini-a") == 128_000
    assert registry.get("openrouter", "route/model") == 32_000
    registry.replace_provider_catalog("google_gemini", {
        "ok": True, "models": [{"id": "gemini-c", "context_length": 16_000}]})
    assert registry.get("google_gemini", "gemini-a") is None
    assert registry.get("google_gemini", "gemini-c") == 16_000
    registry.replace_provider_catalog("openrouter", {
        "ok": False, "models": [], "error": "catalog_unavailable"})
    assert registry.get("openrouter", "route/model") is None


def test_registry_bounds_cached_provider_metadata():
    registry = BUDGET.ModelContextLimitRegistry(max_entries=2)
    registry.replace_provider_catalog("ollama", {
        "ok": True, "models": [
            {"id": "a", "context_length": 4096},
            {"id": "b", "context_length": 8192},
            {"id": "c", "context_length": 16384}]})
    assert sum(registry.get("ollama", name) is not None
               for name in ("a", "b", "c")) == 2


print("PASS model-specific context-budget contracts; no provider/network access")
