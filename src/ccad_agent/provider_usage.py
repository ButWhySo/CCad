"""Normalize token counts reported by provider responses, without payloads."""

from collections.abc import Iterable
from typing import Any


_ALIASES = {
    "input_tokens": ("input_tokens", "prompt_tokens", "prompt_token_count",
                     "promptTokenCount"),
    "output_tokens": ("output_tokens", "completion_tokens",
                      "candidates_token_count", "candidatesTokenCount"),
    "total_tokens": ("total_tokens", "total_token_count", "totalTokenCount"),
}
_MAX_COUNT = 1_000_000_000


def _count(mapping: dict, aliases: tuple[str, ...]) -> int | None:
    for name in aliases:
        value = mapping.get(name)
        if (isinstance(value, int) and not isinstance(value, bool)
                and 0 <= value <= _MAX_COUNT):
            return value
    return None


def normalize_usage(value: Any) -> dict[str, int]:
    """Return only valid input/output/total integers from response usage fields."""
    candidates: list[dict] = []
    if isinstance(value, dict):
        candidates.append(value)
        for key in ("usage_metadata", "usageMetadata", "token_usage", "usage"):
            nested = value.get(key)
            if isinstance(nested, dict):
                candidates.append(nested)
    else:
        direct = getattr(value, "usage_metadata", None)
        if isinstance(direct, dict):
            candidates.append(direct)
        response_metadata = getattr(value, "response_metadata", None)
        if isinstance(response_metadata, dict):
            candidates.append(response_metadata)
            for key in ("usage_metadata", "usageMetadata", "token_usage", "usage"):
                nested = response_metadata.get(key)
                if isinstance(nested, dict):
                    candidates.append(nested)

    counts: dict[str, int] = {}
    for candidate in candidates:
        for normalized, aliases in _ALIASES.items():
            if normalized not in counts:
                result = _count(candidate, aliases)
                if result is not None:
                    counts[normalized] = result
        if len(counts) == len(_ALIASES):
            break
    if "total_tokens" not in counts and {
            "input_tokens", "output_tokens"} <= counts.keys():
        counts["total_tokens"] = counts["input_tokens"] + counts["output_tokens"]
    return counts


def _message_type(message: Any) -> str:
    kind = getattr(message, "type", "")
    if isinstance(kind, str) and kind:
        return kind.casefold()
    return type(message).__name__.casefold()


def collect_turn_usage(messages: Iterable[Any]) -> dict[str, int]:
    """Aggregate response-reported counts from AI messages after latest user message."""
    items = list(messages)
    latest_user = -1
    for index, message in enumerate(items):
        if _message_type(message) in {"human", "user", "humanmessage"}:
            latest_user = index
    if latest_user < 0:
        return {}

    totals: dict[str, int] = {}
    for message in items[latest_user + 1:]:
        if _message_type(message) not in {"ai", "assistant", "aimessage"}:
            continue
        usage = normalize_usage(message)
        for key, count in usage.items():
            totals[key] = min(_MAX_COUNT, totals.get(key, 0) + count)
    if "total_tokens" not in totals and {
            "input_tokens", "output_tokens"} <= totals.keys():
        totals["total_tokens"] = min(
            _MAX_COUNT, totals["input_tokens"] + totals["output_tokens"])
    return totals


def with_provider_usage(report: dict, usage: dict[str, int]) -> dict:
    """Return safe request accounting enriched with actual provider response counts."""
    result = dict(report)
    safe_usage = normalize_usage(usage)
    result["provider_usage"] = safe_usage
    result["provider_usage_source"] = (
        "provider_response" if safe_usage else "unavailable")
    input_tokens = safe_usage.get("input_tokens")
    estimate = report.get("estimated_input_tokens")
    if (input_tokens is not None and isinstance(estimate, int)
            and not isinstance(estimate, bool) and estimate >= 0):
        result["input_token_estimate_delta"] = input_tokens - estimate
    return result


def account_response(report: dict, response: Any) -> dict:
    """Attach only authoritative numeric usage from one provider response."""
    return with_provider_usage(report, normalize_usage(response))


def langfuse_usage_details(usage: dict[str, int]) -> dict[str, int]:
    """Map allowlisted provider counts to Langfuse's usage_details field names."""
    normalized = normalize_usage(usage)
    return {key.removesuffix("_tokens"): count
            for key, count in normalized.items()}
