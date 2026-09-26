"""Opt-in Gemini input-token preflight for the exact bound generation request."""

from __future__ import annotations

import re
from typing import Any, Callable


_MODEL_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$")


def count_gemini_input_tokens(
    bound_model: Any,
    messages: list[Any],
    provider: str,
    model: str,
    enabled: bool,
    timeout_seconds: int,
    classify_error: Callable[[Exception], str],
) -> dict[str, Any]:
    """Count a provider-shaped request without leaking prompt or SDK errors.

    Counting is explicitly opt-in because Gemini receives a second request
    containing the complete prompt and tool declarations. Only Gemini has a
    supported exact path; other providers remain honestly estimated.
    """
    if not enabled:
        return {"status": "disabled", "source": "unavailable",
                "additional_request_sent": False}
    if provider != "google_gemini":
        return {"status": "unsupported_provider", "source": "unavailable",
                "additional_request_sent": False}
    if not isinstance(model, str) or not _MODEL_ID.fullmatch(model):
        return {"status": "failed", "source": "unavailable",
                "additional_request_sent": False,
                "error_category": "invalid_model_id"}

    try:
        model_client = getattr(bound_model, "bound", bound_model)
        prepare = getattr(model_client, "_prepare_request")
        kwargs = getattr(bound_model, "kwargs", {})
        supported = {"stop", "tools", "functions", "safety_settings", "tool_config",
                     "tool_choice", "generation_config", "cached_content"}
        prepared = prepare(messages, **{key: value for key, value in kwargs.items()
                                      if key in supported})
        provider_request = prepared[0] if isinstance(prepared, tuple) else prepared
        if provider_request is None:
            raise ValueError("provider request preparation returned no request")
        client = getattr(model_client, "client")
        timeout = min(120, max(1, int(timeout_seconds)))
    except Exception as error:
        return {"status": "failed", "source": "unavailable",
                "additional_request_sent": False,
                "error_category": classify_error(error)}

    try:
        response = client.count_tokens(
            request={"model": f"models/{model}",
                     "generate_content_request": provider_request},
            timeout=timeout, retry=None)
    except Exception as error:
        return {"status": "failed", "source": "unavailable",
                "additional_request_sent": True,
                "error_category": classify_error(error)}

    total = getattr(response, "total_tokens", None)
    if isinstance(total, bool) or not isinstance(total, int) or not 0 <= total <= 1_000_000_000:
        return {"status": "failed", "source": "unavailable",
                "additional_request_sent": True,
                "error_category": "invalid_count_response"}
    return {"status": "counted", "source": "gemini_count_tokens",
            "exact_input_tokens": total, "additional_request_sent": True}
