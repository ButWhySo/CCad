"""Conservative provider-catalog-based project-context allocation."""

from __future__ import annotations

from typing import Any, Iterable


DEFAULT_CONTEXT_CHARS = 32_768
MAX_CONTEXT_CHARS = 131_072
MAX_CONTEXT_TOKENS = 8_192
MAX_CATALOG_CONTEXT_TOKENS = 100_000_000


def _valid_context_length(value: Any) -> int | None:
    if (isinstance(value, bool) or not isinstance(value, int)
            or value <= 0 or value > MAX_CATALOG_CONTEXT_TOKENS):
        return None
    return value


def catalog_context_limits(provider: str,
                           models: Iterable[dict]) -> dict[tuple[str, str], int]:
    """Extract only provider-reported model IDs and positive context limits."""
    provider_id = provider.strip().casefold() if isinstance(provider, str) else ""
    if not provider_id:
        return {}
    limits = {}
    for item in models:
        if not isinstance(item, dict):
            continue
        model_id = item.get("id")
        limit = _valid_context_length(item.get("context_length"))
        if isinstance(model_id, str) and model_id.strip() and limit is not None:
            limits[(provider_id, model_id.strip())] = limit
    return limits


def lookup_context_limit(limits: dict[tuple[str, str], int],
                         provider: str, model: str) -> int | None:
    """Return metadata only for exact active provider/model identity."""
    if not isinstance(provider, str) or not isinstance(model, str):
        return None
    return limits.get((provider.strip().casefold(), model.strip()))


class ModelContextLimitRegistry:
    """Process-local context limits from the most recent explicit refresh."""

    def __init__(self, max_entries: int = 4096):
        self._limits: dict[tuple[str, str], int] = {}
        self._max_entries = min(16_384, max(1, int(max_entries)))

    def replace_provider_catalog(self, provider: str, catalog: Any) -> None:
        provider_id = provider.strip().casefold() if isinstance(provider, str) else ""
        if not provider_id:
            return
        self._limits = {key: value for key, value in self._limits.items()
                        if key[0] != provider_id}
        if not isinstance(catalog, dict) or not catalog.get("ok"):
            return
        models = catalog.get("models")
        if not isinstance(models, list):
            return
        self._limits.update(catalog_context_limits(provider_id, models))
        while len(self._limits) > self._max_entries:
            self._limits.pop(next(iter(self._limits)))

    def get(self, provider: str, model: str) -> int | None:
        return lookup_context_limit(self._limits, provider, model)


def context_package_char_budget(model_context_limit: Any,
                                *, fallback_chars: int = DEFAULT_CONTEXT_CHARS) -> int:
    """Allocate at most 25% of a known window, capped at 8,192 tokens.

    The 4-character conversion is a conservative sizing heuristic, not a
    tokenizer result. Unknown or malformed model metadata keeps the established
    fixed character budget. The rest of the model window remains available for
    instructions, conversation, tool schemas, provider framing, and output.
    """
    try:
        fallback = int(fallback_chars)
    except (TypeError, ValueError):
        fallback = DEFAULT_CONTEXT_CHARS
    fallback = min(MAX_CONTEXT_CHARS, max(1024, fallback))
    limit = _valid_context_length(model_context_limit)
    if limit is None:
        return fallback
    allocated_tokens = min(MAX_CONTEXT_TOKENS, max(256, limit // 4))
    return min(MAX_CONTEXT_CHARS, max(1024, allocated_tokens * 4))
