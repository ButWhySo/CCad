"""Bounded, explicit provider model-catalog HTTP clients."""

from __future__ import annotations

import json
import os
import urllib.error
import urllib.parse
import urllib.request
from typing import Any, Callable

CatalogFailure = Callable[[str, Exception, str, str], dict[str, Any]]


def catalog_timeout_seconds() -> int:
    """Return the bounded timeout shared by explicit catalog requests."""
    try:
        timeout_value = int(os.environ.get("CCAD_MODEL_CATALOG_TIMEOUT_SECONDS", "8"))
    except ValueError:
        timeout_value = 8
    return min(20, max(2, timeout_value))


def _invalid_shape(source_url: str, network_access: str,
                   source_kind: str = "provider_api") -> dict[str, Any]:
    return {"ok": False, "error": "invalid_catalog_shape", "models": [],
            "network_access": network_access, "source_url": source_url,
            "source_kind": source_kind}


def _missing_key(source_url: str) -> dict[str, Any]:
    return {"ok": False, "error": "missing_api_key", "models": [],
            "network_access": "explicit_refresh", "source_url": source_url,
            "source_kind": "provider_api"}


def fetch_openrouter_models(failure: CatalogFailure) -> dict[str, Any]:
    """Explicit, bounded OpenRouter catalog refresh; never called at startup."""
    source_url = "https://openrouter.ai/api/v1/models"
    key = os.environ.get("OPENROUTER_API_KEY", "")
    if not key:
        return _missing_key(source_url)
    request = urllib.request.Request(
        source_url, headers={"Authorization": f"Bearer {key}", "Accept": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("data"), list):
            return _invalid_shape(source_url, "explicit_refresh")
        models = []
        for item in payload["data"]:
            if not isinstance(item, dict) or not isinstance(item.get("id"), str):
                continue
            model = {"id": item["id"], "name": item.get("name", item["id"]),
                     "context_length": item.get("context_length"),
                     "architecture": item.get("architecture", {})}
            supported = item.get("supported_parameters")
            if isinstance(supported, list):
                model["supported_parameters"] = [value for value in supported
                                                  if isinstance(value, str)]
            models.append(model)
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return failure("openrouter", error, source_url, "explicit_refresh")


def fetch_cerebras_models(failure: CatalogFailure) -> dict[str, Any]:
    """Fetch Cerebras' public catalog; no credential or inference quota used."""
    source_url = "https://api.cerebras.ai/public/v1/models"
    request = urllib.request.Request(source_url, headers={
        "Accept": "application/json",
        "User-Agent": "CCad/1.0 (+https://github.com/ButWhySo/CCad)",
    })
    try:
        with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("data"), list):
            return _invalid_shape(source_url, "explicit_refresh")
        models = []
        for item in payload["data"]:
            if not isinstance(item, dict) or not isinstance(item.get("id"), str):
                continue
            limits = item.get("limits")
            limits = limits if isinstance(limits, dict) else {}
            model = {"id": item["id"], "display_name": item.get("name", item["id"]),
                     "owned_by": item.get("owned_by"),
                     "context_length": item.get("context_length") or
                     limits.get("max_context_length")}
            capabilities = item.get("capabilities")
            if isinstance(capabilities, dict):
                model["capabilities"] = {
                    key: value for key, value in capabilities.items()
                    if key in {"function_calling", "tools", "tool_choice",
                               "parallel_tool_calls", "vision"}
                    and isinstance(value, bool)
                }
            models.append(model)
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return failure("cerebras", error, source_url, "explicit_refresh")


def fetch_ollama_models(failure: CatalogFailure) -> dict[str, Any]:
    """List models from configured local Ollama; never starts or changes it."""
    base_url = os.environ.get("CCAD_OLLAMA_BASE_URL", "http://127.0.0.1:11434/v1").rstrip("/")
    if base_url.endswith("/v1"):
        base_url = base_url[:-3]
    source_url = base_url + "/api/tags"
    request = urllib.request.Request(source_url, headers={"Accept": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("models"), list):
            return _invalid_shape(source_url, "explicit_local_refresh", "local_provider_api")
        models = []
        for item in payload["models"]:
            if not isinstance(item, dict):
                continue
            model_id = item.get("model") or item.get("name")
            if not isinstance(model_id, str) or not model_id:
                continue
            models.append({"id": model_id, "display_name": item.get("name", model_id),
                           "details": item.get("details", {}), "size": item.get("size")})
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_local_refresh", "source_url": source_url,
                "source_kind": "local_provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        result = failure("ollama", error, source_url, "explicit_local_refresh")
        result["source_kind"] = "local_provider_api"
        return result


def fetch_openai_models(failure: CatalogFailure) -> dict[str, Any]:
    """Explicit OpenAI /v1/models refresh; never called at startup."""
    source_url = "https://api.openai.com/v1/models"
    key = os.environ.get("OPENAI_API_KEY", "")
    if not key:
        return _missing_key(source_url)
    request = urllib.request.Request(source_url, headers={
        "Authorization": f"Bearer {key}", "Accept": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("data"), list):
            return _invalid_shape(source_url, "explicit_refresh")
        models = [{"id": item["id"], "display_name": item.get("id"),
                   "owned_by": item.get("owned_by")}
                  for item in payload["data"]
                  if isinstance(item, dict) and isinstance(item.get("id"), str)]
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return failure("openai", error, source_url, "explicit_refresh")


def fetch_anthropic_models(failure: CatalogFailure) -> dict[str, Any]:
    """Explicit, bounded and paginated Anthropic /v1/models refresh."""
    source_url = "https://api.anthropic.com/v1/models"
    key = os.environ.get("ANTHROPIC_API_KEY", "")
    if not key:
        return _missing_key(source_url)
    try:
        models, after_id = [], ""
        for _ in range(20):
            query = {"limit": "1000"}
            if after_id:
                query["after_id"] = after_id
            request = urllib.request.Request(
                source_url + "?" + urllib.parse.urlencode(query), headers={
                    "x-api-key": key, "anthropic-version": "2023-06-01",
                    "Accept": "application/json"})
            with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
                payload = json.loads(response.read().decode("utf-8"))
            if not isinstance(payload, dict) or not isinstance(payload.get("data"), list):
                return _invalid_shape(source_url, "explicit_refresh")
            models.extend({"id": item["id"],
                           "display_name": item.get("display_name", item["id"]),
                           "created_at": item.get("created_at"),
                           "capabilities": item.get("capabilities", {})}
                          for item in payload["data"]
                          if isinstance(item, dict) and isinstance(item.get("id"), str))
            next_after = payload.get("last_id", "")
            if (not payload.get("has_more") or not isinstance(next_after, str)
                    or not next_after or next_after == after_id):
                break
            after_id = next_after
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return failure("anthropic", error, source_url, "explicit_refresh")


def fetch_gemini_models(failure: CatalogFailure) -> dict[str, Any]:
    """Explicit and paginated Gemini models.list refresh."""
    source_url = "https://generativelanguage.googleapis.com/v1beta/models"
    key = os.environ.get("GEMINI_API_KEY", "") or os.environ.get("GOOGLE_API_KEY", "")
    if not key:
        return _missing_key(source_url)
    try:
        models, page_token = [], ""
        for _ in range(20):
            query = {"pageSize": "1000"}
            if page_token:
                query["pageToken"] = page_token
            request = urllib.request.Request(
                source_url + "?" + urllib.parse.urlencode(query), headers={
                    "x-goog-api-key": key, "Accept": "application/json"})
            with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
                payload = json.loads(response.read().decode("utf-8"))
            if not isinstance(payload, dict) or not isinstance(payload.get("models"), list):
                return _invalid_shape(source_url, "explicit_refresh")
            for item in payload["models"]:
                if not isinstance(item, dict) or not isinstance(item.get("name"), str):
                    continue
                methods = item.get("supportedGenerationMethods", [])
                if methods and "generateContent" not in methods:
                    continue
                model_id = item["name"].removeprefix("models/")
                models.append({"id": model_id,
                               "display_name": item.get("displayName", model_id),
                               "context_length": item.get("inputTokenLimit"),
                               "supported_generation_methods": methods})
            next_token = payload.get("nextPageToken", "")
            if not isinstance(next_token, str) or not next_token or next_token == page_token:
                break
            page_token = next_token
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return failure("google_gemini", error, source_url, "explicit_refresh")
