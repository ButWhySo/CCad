import sys
import json
import operator
import os
import re
import queue
import sys
import threading
import time
import uuid
import atexit
import hashlib
import warnings
import urllib.request
import urllib.error
import urllib.parse
from email.utils import parsedate_to_datetime
from datetime import datetime, timezone
from collections import deque
from typing import Annotated, Any, Dict, Iterable, List, Literal, TypedDict, cast
from langchain_core.tools import StructuredTool
from langchain_core.runnables import RunnableConfig
from pydantic import Field, create_model
from langchain_core.messages import AIMessage, BaseMessage, HumanMessage, SystemMessage
# langgraph-checkpoint currently emits this known pending-deprecation warning
# during import; install filter after langchain_core imports, which may reset
# warning filters. Keep all other warnings visible.
warnings.filterwarnings(
    "ignore",
    message=r"The default value of `allowed_objects` will change.*",
    category=Warning,
)
# langchain-google-genai currently imports Google's legacy caching module,
# which emits a dependency FutureWarning to stderr. It neither describes a
# failed provider request nor needs user action, so do not leak it into chat.
warnings.filterwarnings(
    "ignore",
    category=FutureWarning,
    module=r"langchain_google_genai\\.chat_models",
)
warnings.filterwarnings(
    "ignore",
    message=r"All support for the `google\.generativeai` package has ended\..*",
    category=FutureWarning,
)
from langgraph.graph import StateGraph, END
from langgraph.types import Command, interrupt

from config import AgentConfigManager, ConfigPersistenceError
from telemetry import runtime as telemetry_runtime, trace_function
import hooks
from memory_store import MemoryStore, MemoryStoreError
from memory_manager import MemoryManager, MemoryTaskScopes
from memory_commands import execute_memory_command
from history_compaction import (HistoryCompactionError,
                                compact_history,
                                prepare_history_compaction,
                                replace_checkpoint_history)
from context_package import (build_context_package, build_provider_request_report,
                             format_large_context_explanation)

def emit(payload: dict):
    print(json.dumps(payload), flush=True)

def catalog_failure(provider: str, error: Exception, source_url: str,
                    network_access: str):
    """Return a safe, provider-specific catalog failure without raw bodies."""
    status = provider_http_status(error)
    text = str(error).lower()
    metadata = provider_error_metadata(error)
    codes = metadata["codes"]
    if status == 401 or any(code in {"authentication_error", "invalid_api_key"} for code in codes) or "unauthorized" in text or "invalid api key" in text:
        category = "authentication"
    elif status == 403 or any(code in {"permission_error", "permission_denied"} for code in codes) or "forbidden" in text or "permission" in text:
        category = "permission_denied"
    elif status == 402 or any(code in {"billing_error", "payment_required", "credit_balance_exhausted"} for code in codes) or any(marker in text for marker in
                              ("payment required", "insufficient credit", "billing")):
        category = "payment_required"
    elif any(code in {"insufficient_quota", "quota_exceeded", "organization_spend_limit_exceeded", "project_spend_limit_exceeded"} for code in codes) or "quota exceeded" in text:
        category = "quota_exhausted"
    elif status == 429 or any(marker in text for marker in
                               ("rate limit", "rate_limit", "too many requests", "quota")):
        category = "rate_limited"
    elif isinstance(error, TimeoutError):
        category = "timeout"
    elif isinstance(error, urllib.error.URLError):
        category = "connection_error"
    elif isinstance(error, ValueError):
        category = "invalid_response"
    else:
        category = "provider_unavailable"
    result = {"ok": False, "error": category, "error_type": type(error).__name__,
            "provider": provider, "models": [], "network_access": network_access,
            "source_url": source_url, "source_kind": "provider_api"}
    if status is not None:
        result["http_status"] = status
    if metadata["retry_after_seconds"] is not None:
        result["retry_after_seconds"] = metadata["retry_after_seconds"]
    return result

def fetch_openrouter_models():
    """Explicit, bounded OpenRouter catalog refresh; never called at startup."""
    source_url = "https://openrouter.ai/api/v1/models"
    key = os.environ.get("OPENROUTER_API_KEY", "")
    if not key:
        return {"ok": False, "error": "missing_api_key", "models": [],
                "source_url": source_url, "source_kind": "provider_api"}
    request = urllib.request.Request(
        "https://openrouter.ai/api/v1/models",
        headers={"Authorization": f"Bearer {key}", "Accept": "application/json"},
    )
    try:
        timeout_value = int(os.environ.get("CCAD_MODEL_CATALOG_TIMEOUT_SECONDS", "8"))
    except ValueError:
        timeout_value = 8
    timeout = min(20, max(2, timeout_value))
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict):
            return {"ok": False, "error": "invalid_catalog_shape", "models": [],
                    "network_access": "explicit_refresh", "source_url": source_url,
                    "source_kind": "provider_api"}
        models = []
        for item in payload.get("data", []):
            if not isinstance(item, dict) or not item.get("id"):
                continue
            models.append({
                "id": item["id"], "name": item.get("name", item["id"]),
                "context_length": item.get("context_length"),
                "architecture": item.get("architecture", {}),
            })
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return catalog_failure("openrouter", error, source_url, "explicit_refresh")

def fetch_cerebras_models():
    """Explicit, bounded public Cerebras catalog refresh; never at startup.

    Cerebras documents this endpoint as public.  It deliberately does not use
    the saved inference credential, so model discovery remains available while
    a project is awaiting billing activation and cannot spend inference quota.
    """
    source_url = "https://api.cerebras.ai/public/v1/models"
    request = urllib.request.Request(source_url, headers={
        "Accept": "application/json",
        # Cerebras fronts the public catalog with Cloudflare, which rejects
        # Python's anonymous default user agent even though this endpoint is
        # intentionally unauthenticated.
        "User-Agent": "CCad/1.0 (+https://github.com/ButWhySo/CCad)",
    })
    try:
        timeout_value = int(os.environ.get("CCAD_MODEL_CATALOG_TIMEOUT_SECONDS", "8"))
    except ValueError:
        timeout_value = 8
    timeout = min(20, max(2, timeout_value))
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict):
            return {"ok": False, "error": "invalid_catalog_shape", "models": [],
                    "network_access": "explicit_refresh", "source_url": source_url,
                    "source_kind": "provider_api"}
        models = []
        for item in payload.get("data", []):
            if not isinstance(item, dict) or not item.get("id"):
                continue
            models.append({"id": item["id"], "display_name": item.get("name", item["id"]),
                           "owned_by": item.get("owned_by"),
                           "context_length": item.get("context_length"),
                           "capabilities": item.get("capabilities", {})})
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return catalog_failure("cerebras", error, source_url, "explicit_refresh")

def fetch_ollama_models():
    """Explicit local Ollama inventory; never starts, pulls, or changes Ollama."""
    base_url = os.environ.get("CCAD_OLLAMA_BASE_URL", "http://127.0.0.1:11434/v1")
    base_url = base_url.rstrip("/")
    if base_url.endswith("/v1"):
        base_url = base_url[:-3]
    source_url = base_url + "/api/tags"
    request = urllib.request.Request(source_url, headers={"Accept": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("models"), list):
            return {"ok": False, "error": "invalid_catalog_shape", "models": [],
                    "network_access": "explicit_local_refresh", "source_url": source_url,
                    "source_kind": "local_provider_api"}
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
        result = catalog_failure("ollama", error, source_url, "explicit_local_refresh")
        result["source_kind"] = "local_provider_api"
        return result

def catalog_timeout_seconds():
    """Return a bounded timeout shared by explicit catalog requests."""
    try:
        timeout_value = int(os.environ.get("CCAD_MODEL_CATALOG_TIMEOUT_SECONDS", "8"))
    except ValueError:
        timeout_value = 8
    return min(20, max(2, timeout_value))

def fetch_openai_models():
    """Explicit OpenAI `/v1/models` refresh; never called at startup."""
    source_url = "https://api.openai.com/v1/models"
    key = os.environ.get("OPENAI_API_KEY", "")
    if not key:
        return {"ok": False, "error": "missing_api_key", "models": [],
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    request = urllib.request.Request(source_url, headers={
        "Authorization": f"Bearer {key}", "Accept": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not isinstance(payload, dict) or not isinstance(payload.get("data"), list):
            return {"ok": False, "error": "invalid_catalog_shape", "models": [],
                    "network_access": "explicit_refresh", "source_url": source_url,
                    "source_kind": "provider_api"}
        models = [{"id": item["id"], "display_name": item.get("id"),
                   "owned_by": item.get("owned_by")}
                  for item in payload.get("data", [])
                  if isinstance(item, dict) and isinstance(item.get("id"), str)]
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return catalog_failure("openai", error, source_url, "explicit_refresh")

def fetch_anthropic_models():
    """Explicit Anthropic `/v1/models` refresh; never called at startup."""
    source_url = "https://api.anthropic.com/v1/models"
    key = os.environ.get("ANTHROPIC_API_KEY", "")
    if not key:
        return {"ok": False, "error": "missing_api_key", "models": [],
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    try:
        models, after_id = [], ""
        # Anthropic returns at most 1,000 entries per page and pages with the
        # opaque final model id. Cap continuation defensively in case a server
        # repeats a cursor; normal accounts complete in the first request.
        for _ in range(20):
            query = {"limit": "1000"}
            if after_id: query["after_id"] = after_id
            request = urllib.request.Request(
                source_url + "?" + urllib.parse.urlencode(query), headers={
                    "x-api-key": key, "anthropic-version": "2023-06-01",
                    "Accept": "application/json"})
            with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
                payload = json.loads(response.read().decode("utf-8"))
            if not isinstance(payload, dict) or not isinstance(payload.get("data"), list):
                return {"ok": False, "error": "invalid_catalog_shape", "models": [],
                        "network_access": "explicit_refresh", "source_url": source_url,
                        "source_kind": "provider_api"}
            models.extend({"id": item["id"],
                           "display_name": item.get("display_name", item["id"]),
                           "created_at": item.get("created_at"),
                           "capabilities": item.get("capabilities", {})}
                          for item in payload["data"]
                          if isinstance(item, dict) and isinstance(item.get("id"), str))
            next_after = payload.get("last_id", "")
            if not payload.get("has_more") or not isinstance(next_after, str) or not next_after or next_after == after_id:
                break
            after_id = next_after
        return {"ok": True, "models": models, "count": len(models),
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return catalog_failure("anthropic", error, source_url, "explicit_refresh")

def fetch_gemini_models():
    """Explicit Gemini `models.list` refresh; never called at startup."""
    source_url = "https://generativelanguage.googleapis.com/v1beta/models"
    key = os.environ.get("GEMINI_API_KEY", "") or os.environ.get("GOOGLE_API_KEY", "")
    if not key:
        return {"ok": False, "error": "missing_api_key", "models": [],
                "network_access": "explicit_refresh", "source_url": source_url,
                "source_kind": "provider_api"}
    try:
        models = []
        page_token = ""
        # The Gemini REST catalog is paginated.  Preserve every model the
        # current key can use rather than silently offering only page one.
        for _ in range(20):
            query = {"pageSize": "1000"}
            if page_token: query["pageToken"] = page_token
            request = urllib.request.Request(
                source_url + "?" + urllib.parse.urlencode(query), headers={
                    "x-goog-api-key": key, "Accept": "application/json"})
            with urllib.request.urlopen(request, timeout=catalog_timeout_seconds()) as response:
                payload = json.loads(response.read().decode("utf-8"))
            if not isinstance(payload, dict) or not isinstance(payload.get("models"), list):
                return {"ok": False, "error": "invalid_catalog_shape", "models": [],
                        "network_access": "explicit_refresh", "source_url": source_url,
                        "source_kind": "provider_api"}
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
        return catalog_failure("google_gemini", error, source_url, "explicit_refresh")

def cerebras_model_snapshot():
    """Return documented public presets without a network call.

    Source: https://inference-docs.cerebras.ai/models/overview
    Refresh this snapshot when provider docs change; live model listing remains
    an explicit public operation and is never performed at startup.
    """
    models = [
        {"id": "gpt-oss-120b", "display_name": "OpenAI GPT OSS 120B", "tier": "production",
         "context_window_free": 65000, "context_window_paid": 131000,
         "speed_tokens_per_second": 3000,
         "reasoning_effort": ["low", "medium", "high"]},
        {"id": "qwen-3.8-27b", "display_name": "Qwen 3.8 27B", "tier": "production",
         "context_window_free": 64000, "context_window_paid": 128000,
         "speed_tokens_per_second": 1850,
         "reasoning_effort": ["none", "low", "medium", "high"]},
    ]
    return {"ok": True, "models": models, "count": len(models),
            "network_access": "none", "source": "official_curated_snapshot",
            "source_url": "https://inference-docs.cerebras.ai/models/overview",
            "source_kind": "first_party_documentation"}

def context_revision(context: str) -> str:
    """Return stable opaque context identity; never expose context contents."""
    return hashlib.sha256(context.encode("utf-8")).hexdigest()[:16]

broker_wait_enabled = False
current_run_trace_id = ""
current_run_span_id = ""
inbound_queue: Any = None
deferred_queue = queue.Queue()
pending_calls = {}
pending_call_threads = {}
pending_calls_lock = threading.Lock()

def route_protocol_line(protocol_line: str) -> bool:
    """Route a result to its waiting call; return whether it was consumed."""
    try:
        response = json.loads(protocol_line)
    except (TypeError, json.JSONDecodeError):
        return False
    if response.get("method") != "tool_result":
        return False
    call_id = response.get("id", "")
    with pending_calls_lock:
        result_queue = pending_calls.get(call_id)
    if result_queue is None:
        return False
    result_queue.put(protocol_line)
    return True

def wait_for_broker_result(call_id: str) -> str:
    """Synchronously receive matching C++ broker result for current tool call."""
    timeout = max(1.0, float(os.environ.get("CCAD_BROKER_TIMEOUT_SECONDS", "30")))
    deadline = time.monotonic() + timeout
    result_queue = queue.Queue()
    thread_id = os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local")
    with pending_calls_lock:
        pending_calls[call_id] = result_queue
        pending_call_threads[call_id] = thread_id
    try:
        while True:
            if inbound_queue is not None:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return json.dumps({"error": "broker_timeout", "call_id": call_id})
                try:
                    line = result_queue.get(timeout=remaining)
                except queue.Empty:
                    return json.dumps({"error": "broker_timeout", "call_id": call_id})
            else:
                line = sys.stdin.readline()
            if not line:
                return json.dumps({"error": "broker_closed", "call_id": call_id})
            try:
                response = json.loads(line)
            except json.JSONDecodeError:
                continue
            if response.get("method") != "tool_result" or response.get("id", "") != call_id:
                deferred_queue.put(line)
                continue
            if response.get("error") is not None:
                return json.dumps({"error": response["error"], "call_id": call_id})
            return json.dumps(response.get("result", {"error": "empty_broker_result"}))
    finally:
        with pending_calls_lock:
            pending_calls.pop(call_id, None)
            pending_call_threads.pop(call_id, None)

def new_tool_call_id(tool_name: str) -> str:
    """Create a per-invocation correlation ID; never reuse across retries."""
    return f"{tool_name}-{uuid.uuid4().hex}"

def emit_tool_approval_state():
    """Publish approval before a mutating tool blocks on the client result."""
    if not current_run_trace_id:
        return
    emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
        "run_state": "awaiting_tool_approval", "trace_id": current_run_trace_id,
        "span_id": current_run_span_id, "token_usage": "unavailable", "cost": "unavailable",
    }})

def checkpoint_tool_call_id(tool_name: str, args: dict) -> str:
    """Stable ID lets interrupted tool re-execution correlate after restart."""
    encoded = json.dumps({"tool": tool_name, "args": args}, sort_keys=True,
                         separators=(",", ":")).encode("utf-8")
    return f"{tool_name}-{hashlib.sha256(encoded).hexdigest()[:24]}"

def pending_call_snapshot(thread_id: str = ""):
    """Return opaque pending-call metadata without tool arguments or secrets."""
    requested_thread = thread_id or os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local")
    with pending_calls_lock:
        process_calls = sorted(call_id for call_id in pending_calls
                               if pending_call_threads.get(call_id) == requested_thread)
    checkpoint_calls = []
    if checkpoint_saver is not None and executor is not None:
        try:
            snapshot = executor.get_state({"configurable": {
                "thread_id": requested_thread
            }})
            for task in snapshot.tasks:
                for pending_interrupt in getattr(task, "interrupts", ()):
                    value = getattr(pending_interrupt, "value", {})
                    if isinstance(value, dict) and value.get("call_id"):
                        checkpoint_calls.append(str(value["call_id"]))
        except Exception:
            checkpoint_calls = []
    return {
        "thread_id": requested_thread,
        "process_call_ids": process_calls,
        "checkpoint_call_ids": sorted(set(checkpoint_calls)),
        "count": len(set(process_calls).union(checkpoint_calls)),
        "approval_required": bool(process_calls or checkpoint_calls),
        "approval_reason": "project_mutation" if (process_calls or checkpoint_calls) else "",
        "secret_value_visible": False,
    }

from method_catalog import orchestrator_method_catalog

def sanitize_persisted_config(value, secret_key_fragments, rejected_keys, path="") -> Any:
    """Remove secret-like keys before any config value reaches disk."""
    if isinstance(value, dict):
        clean = {}
        for key, child in value.items():
            key_text = str(key)
            full_key = f"{path}.{key_text}" if path else key_text
            if any(fragment in key_text.lower() for fragment in secret_key_fragments):
                rejected_keys.append(full_key)
                continue
            clean[key] = sanitize_persisted_config(child, secret_key_fragments,
                                                    rejected_keys, full_key)
        return clean
    if isinstance(value, list):
        return [sanitize_persisted_config(item, secret_key_fragments,
                                           rejected_keys, path) for item in value]
    return value

@trace_function("dispatch-native-tool", "tool")
def dispatch_checkpointed_tool(tool_name: str, args: dict):
    """Pause graph until C++ client returns authoritative tool result."""
    call_id = checkpoint_tool_call_id(tool_name, args)
    decision = interrupt({"kind": "ccad_tool_call", "tool": tool_name,
                          "args": args, "call_id": call_id,
                          "approval_required": True,
                          "approval_reason": "project_mutation"})
    if isinstance(decision, dict) and "error" in decision:
        return json.dumps(decision)
    return json.dumps(decision) if isinstance(decision, (dict, list)) else str(decision)

def tool_approval_decision(tool_name: str, args: dict):
    """Return one deterministic approval decision for a client tool call."""
    dry_run = bool(args.get("dry_run", False))
    required = tool_name.startswith("ui.") and not dry_run
    return {"required": required,
            "reason": "dry_run" if dry_run else "project_mutation"}

@trace_function("dispatch-native-tool", "tool")
def dispatch_client_tool(tool_name: str, args: dict) -> str:
    """Send one client tool call and await its authoritative broker result."""
    call_id = (checkpoint_tool_call_id(tool_name, args)
               if checkpoint_saver is not None and broker_wait_enabled
               else new_tool_call_id(tool_name))
    approval = tool_approval_decision(tool_name, args)
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {
        "tool": tool_name, "args": args, "call_id": call_id,
        "approval_required": approval["required"],
        "approval_reason": approval["reason"],
    }})
    if broker_wait_enabled:
        if approval["required"]:
            emit_tool_approval_state()
        if checkpoint_saver is not None:
            return dispatch_checkpointed_tool(tool_name, args)
        return wait_for_broker_result(call_id)
    return json.dumps({"error": "broker_wait_unavailable", "tool": tool_name,
                       "project_action": False})

config_manager = AgentConfigManager()
memory_store = MemoryStore()
memory_manager = MemoryManager(memory_store)
memory_task_scopes = MemoryTaskScopes(memory_manager)
memory_manager.configure(config_manager.get("memory", {}))

def local_memory_entries(query=""):
    """Return ranked, enabled, namespace-scoped memories for one turn."""
    entries, provenance = memory_manager.retrieve_with_metadata(query)
    return (entries if isinstance(entries, list) else [],
            provenance if isinstance(provenance, list) else [])


def emit_provider_request_context(system_text, messages, context_content,
                                 context_metadata, tools, provider, model):
    """Emit safe prompt-size facts; expose full breakdown only for large turns."""
    report = build_provider_request_report(
        system_text, messages, tools, provider=provider, model=model,
        context_content=context_content, context_metadata=context_metadata,
        large_context_threshold=os.environ.get(
            "CCAD_AGENT_LARGE_CONTEXT_TOKENS", "4096"))
    emit({"jsonrpc": "2.0", "method": "provider_request_context",
          "params": report})
    if os.environ.get("CCAD_TRACE_DEBUG", "").lower() in {"1", "true", "yes"}:
        print("[ccad-context] " + json.dumps(report, sort_keys=True),
              file=sys.stderr, flush=True)
    if report["large_context"]:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "kind": "large_context_breakdown",
            "text": format_large_context_explanation(report, context_metadata)}})
    return report


def provider_request_trace_metadata(report):
    """Convert safe request accounting to Langfuse-compatible string metadata."""
    result = {
        "prompt_chars": str(report["prompt_chars"]),
        "estimated_input_tokens": str(report["estimated_input_tokens"]),
        "tool_schema_count": str(report["tool_schema_count"]),
        "model_context_limit": str(report["model_context_limit"] or "unavailable"),
        "context_package_truncated": str(report["context_package_truncated"]).lower(),
        "project_snapshot_omitted": str(report["project_snapshot_omitted"]).lower(),
        "project_source_chars": str(report["project_source_chars"]),
        "memory_entry_count": str(report["memory_entry_count"]),
        "estimate_includes_all_payloads": str(report["estimate_includes_all_payloads"]).lower(),
    }
    for name, component in report["components"].items():
        result[f"input_{name}_estimated_tokens"] = str(component["estimated_tokens"])
    for tier, count in report["memory_tier_counts"].items():
        result[f"memory_{tier}_retrieved_count"] = str(count)
    for tier, runtime in report["memory_runtime"].items():
        result[f"memory_{tier}_enabled"] = str(runtime["enabled"]).lower()
        result[f"memory_{tier}_loaded_count"] = str(runtime["runtime_entries"])
        result[f"memory_{tier}_persistent_count"] = str(runtime["persistent_entries"])
        result[f"memory_{tier}_namespace_hash"] = runtime["namespace_hash"]
    for item in report["memory_retrieval"]:
        prefix = f"memory_match_{item['rank']}"
        result[f"{prefix}_tier"] = item["tier"]
        result[f"{prefix}_overlap_terms"] = str(item["query_overlap_terms"])
        result[f"{prefix}_namespace_hash"] = item["namespace_hash"]
    return result

def parse_memory_add_args(arguments):
    """Parse optional leading scope/title flags from a memory add command."""
    tokens = arguments.split()
    scope, title = "project", ""
    while tokens and (tokens[0].startswith("scope:") or tokens[0].startswith("title:")):
        key, value = tokens.pop(0).split(":", 1)
        if key == "scope":
            scope = value.strip() or "project"
        else:
            title = value.strip()
    return " ".join(tokens).strip(), title, scope

def parse_memory_update_args(arguments):
    """Parse memory ID plus optional metadata for an update command."""
    tokens = arguments.split()
    if not tokens:
        return "", "", "project", ""
    entry_id = tokens.pop(0)
    content, title, scope = parse_memory_add_args(" ".join(tokens))
    return entry_id, content, scope, title

class AgentState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]
    goal: str
    context: str
    context_metadata: Dict[str, Any]
    next_node: str
    thread_id: str

def tool_provider_name(method_name: str) -> str:
    """Create a provider-compatible stable name without losing native identity."""
    return "ccad_" + re.sub(r"[^A-Za-z0-9_-]", "_", method_name)


def json_schema_annotation(schema: dict):
    """Map CCad's JSON Schema subset to the pydantic types LangChain exports."""
    if not isinstance(schema, dict):
        return Any
    enum = schema.get("enum")
    if isinstance(enum, list) and enum and all(isinstance(value, str) for value in enum):
        return Literal[tuple(enum)]
    schema_type = schema.get("type")
    if schema_type == "string":
        return str
    if schema_type == "integer":
        return int
    if schema_type == "number":
        return float
    if schema_type == "boolean":
        return bool
    if schema_type == "array":
        return List[json_schema_annotation(schema.get("items", {}))]
    if schema_type == "object":
        return Dict[str, Any]
    return Any


def validate_native_tool_catalog(catalog: object) -> List[dict]:
    """Accept only the typed native method catalog forwarded by the GUI."""
    if not isinstance(catalog, list) or not catalog:
        raise ValueError("catalog must be a non-empty method array")
    accepted, seen_methods, seen_provider_names = [], set(), set()
    for entry in catalog:
        if not isinstance(entry, dict):
            raise ValueError("catalog entries must be objects")
        method = entry.get("method")
        description = entry.get("description")
        schema = entry.get("inputSchema")
        if (not isinstance(method, str) or not re.fullmatch(r"[A-Za-z][A-Za-z0-9_.-]{0,95}", method)
                or not isinstance(description, str) or not description.strip()
                or not isinstance(schema, dict) or schema.get("type") != "object"):
            raise ValueError("catalog entry has invalid method, description, or inputSchema")
        callable_method = entry.get("callable", True)
        if not isinstance(callable_method, bool):
            raise ValueError("catalog callable flag must be boolean")
        if not callable_method:
            continue
        provider_name = tool_provider_name(method)
        if method in seen_methods or provider_name in seen_provider_names:
            raise ValueError("catalog contains duplicate method identity")
        seen_methods.add(method)
        seen_provider_names.add(provider_name)
        accepted.append({"method": method, "description": description.strip(),
                         "inputSchema": schema,
                         "read_only": bool(entry.get("read_only", False))})
    if not accepted:
        raise ValueError("catalog has no callable native methods")
    return accepted


def build_native_tools(catalog: List[dict]) -> List[StructuredTool]:
    """Build real LangChain tools that call the authoritative C++ ToolBroker."""
    tools = []
    for entry in catalog:
        schema = entry["inputSchema"]
        required = set(schema.get("required", [])) if isinstance(schema.get("required", []), list) else set()
        properties = schema.get("properties", {})
        if not isinstance(properties, dict):
            raise ValueError("catalog inputSchema.properties must be an object")
        fields = {}
        for key, property_schema in properties.items():
            if not isinstance(key, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key):
                raise ValueError("catalog property name is not provider-safe")
            if not isinstance(property_schema, dict):
                raise ValueError("catalog property schema must be an object")
            default = ... if key in required else property_schema.get("default", None)
            fields[key] = (json_schema_annotation(property_schema),
                           Field(default, description=str(property_schema.get("description", ""))))
        args_schema = create_model("CCad_" + tool_provider_name(entry["method"]).replace("-", "_"),
                                   **fields)
        method = entry["method"]
        read_only = entry["read_only"]

        def invoke_native_tool(_method=method, _read_only=read_only, **kwargs):
            return dispatch_client_tool(_method, kwargs)

        tools.append(StructuredTool.from_function(
            invoke_native_tool, name=tool_provider_name(method),
            description=f"CCad native method `{method}`. {entry['description']}",
            args_schema=args_schema))
    return tools


native_tool_catalog: List[dict] = []
agent_tools: List[StructuredTool] = []


def bind_native_tools(model: Any):
    """Bind CCad StructuredTools at the dynamic LangChain adapter boundary.

    Provider packages expose narrower `bind_tools` annotations than the
    StructuredTool objects they accept at runtime, so keep the dynamic call in
    one checked boundary instead of repeating adapter-specific type escapes.
    """
    binder = getattr(model, "bind_tools", None)
    if not callable(binder):
        raise TypeError("provider adapter does not support tool binding")
    return binder(agent_tools)


def install_native_tool_catalog(catalog: object) -> dict:
    """Install the GUI's current native catalog and rebuild live graph bindings."""
    global native_tool_catalog, agent_tools, router_llm, librarian_llm, execute_tool_node, executor
    native_tool_catalog = validate_native_tool_catalog(catalog)
    agent_tools = build_native_tools(native_tool_catalog)
    if llm is not None:
        router_llm = bind_native_tools(llm)
        librarian_llm = router_llm
    execute_tool_node = ToolNode(agent_tools)
    if executor is not None:
        executor = create_orchestrator()
    return {"accepted": True, "method_count": len(native_tool_catalog),
            "tool_count": len(agent_tools), "secret_value_visible": False}

llm = None
router_llm = None
librarian_llm = None
checkpoint_saver: Any = None
checkpoint_context: Any = None
session_provider_env = set()
executor: Any = None

def set_session_provider_env(name, value):
    """Set provider credential only for this process and track its alias."""
    os.environ[name] = value
    session_provider_env.add(name)

def clear_session_provider_env():
    """Remove credential aliases created by the in-process settings flow."""
    for name in tuple(session_provider_env):
        os.environ.pop(name, None)
    session_provider_env.clear()

def init_checkpointer():
    """Enable durable LangGraph checkpoints only when an explicit DB path is set."""
    global checkpoint_saver, checkpoint_context
    db_path = os.environ.get("CCAD_AGENT_CHECKPOINT_DB", "").strip()
    if not db_path:
        return False
    try:
        from langgraph.checkpoint.sqlite import SqliteSaver
        checkpoint_context = SqliteSaver.from_conn_string(db_path)
        checkpoint_saver = checkpoint_context.__enter__()
        checkpoint_saver.setup()
        atexit.register(lambda: checkpoint_context.__exit__(None, None, None))
        emit({"jsonrpc": "2.0", "method": "checkpoint_state", "params": {
            "enabled": True, "backend": "sqlite", "path_visible": True,
        }})
        return True
    except (ImportError, OSError, RuntimeError) as error:
        checkpoint_saver = None
        checkpoint_context = None
        emit({"jsonrpc": "2.0", "method": "checkpoint_state", "params": {
            "enabled": False, "backend": "sqlite", "error": type(error).__name__,
        }})
        return False

langsmith_callbacks = []
# LangSmith is opt-in and provider-owned: no credentials means no exporter,
# no network. Langfuse callbacks are rebuilt by TelemetryRuntime after Settings
# changes and are collected at each invocation rather than module import.
if (os.environ.get("LANGCHAIN_TRACING_V2", "").lower() == "true"
        and os.environ.get("LANGCHAIN_API_KEY")):
    try:
        from langchain.callbacks.tracers import LangChainTracer
        langsmith_callbacks.append(LangChainTracer(
            project_name=os.environ.get("LANGCHAIN_PROJECT", "ccad")))
    except ImportError:
        pass

observability_secrets = {"public_key": "", "secret_key": ""}

def active_callbacks():
    return telemetry_runtime.callbacks() + list(langsmith_callbacks)

def observability_state():
    return telemetry_runtime.status()

def reconfigure_observability():
    return telemetry_runtime.configure(
        config_manager.get("observability", {}), observability_secrets)

def emit_provider_failure(provider: str, error: Exception):
    """Report adapter failure without exposing key, prompt, or endpoint data."""
    category = classify_provider_error(error)
    params = {
        "provider": provider, "configured": category != "missing_api_key",
        "error": type(error).__name__,
        "error_category": category, "execution_enabled": False,
        "secret_value_visible": False,
    }
    status = provider_http_status(error)
    retry_after = provider_retry_after_seconds(error)
    if status is not None:
        params["http_status"] = status
    if retry_after is not None:
        params["retry_after_seconds"] = retry_after
    emit({"jsonrpc": "2.0", "method": "provider_state", "params": params})
    return category

def provider_exception_chain(error: Exception) -> Iterable[Exception]:
    """Yield wrapped SDK exceptions once, including exception-group children."""
    pending = deque([error])
    seen = set()
    while pending:
        candidate = pending.popleft()
        if id(candidate) in seen:
            continue
        seen.add(id(candidate))
        yield candidate
        for nested in (getattr(candidate, "__cause__", None),
                       getattr(candidate, "__context__", None)):
            if isinstance(nested, Exception):
                pending.append(nested)
        nested_errors = getattr(candidate, "exceptions", ())
        if isinstance(nested_errors, (tuple, list)):
            pending.extend(item for item in nested_errors if isinstance(item, Exception))

def provider_http_status(error: Exception):
    """Extract a numeric HTTP status from nested SDK and gRPC wrappers."""
    for candidate in provider_exception_chain(error):
        for response in (candidate, getattr(candidate, "response", None)):
            for name in ("status_code", "status", "code"):
                status = getattr(response, name, None)
                if isinstance(status, int) and 100 <= status <= 599:
                    return status
    return None

def provider_error_metadata(error: Exception):
    """Return safe structured error codes and bounded retry delay, never body text."""
    codes = []
    retry_after = None
    for candidate in provider_exception_chain(error):
        response = getattr(candidate, "response", None)
        body = getattr(candidate, "body", None)
        response_json = getattr(response, "json", None) if response is not None else None
        if callable(response_json):
            try:
                body = response_json()
            except Exception:
                body = None
        if isinstance(body, dict):
            error_body = body.get("error", body)
            if isinstance(error_body, dict):
                code = error_body.get("code") or error_body.get("type") or error_body.get("status")
                if isinstance(code, str):
                    codes.append(code.lower())
        for source in (candidate, response):
            headers = getattr(source, "headers", None)
            if headers is None:
                continue
            try:
                raw_delay = headers.get("retry-after") or headers.get("Retry-After")
                try:
                    delay = float(raw_delay)
                except (TypeError, ValueError):
                    retry_time = parsedate_to_datetime(str(raw_delay))
                    if retry_time.tzinfo is None:
                        retry_time = retry_time.replace(tzinfo=timezone.utc)
                    delay = (retry_time - datetime.now(timezone.utc)).total_seconds()
            except (AttributeError, TypeError, ValueError, OverflowError):
                continue
            if delay >= 0:
                retry_after = max(0, min(3600, int(delay)))
                break
        if retry_after is not None:
            break
    return {"codes": tuple(codes), "retry_after_seconds": retry_after}

def provider_retry_after_seconds(error: Exception):
    """Expose a bounded Retry-After value for safe, actionable UI guidance."""
    return provider_error_metadata(error)["retry_after_seconds"]

def classify_provider_error(error: Exception):
    """Return safe, actionable category; never include secret-bearing text."""
    for candidate in provider_exception_chain(error):
        status = next((getattr(response, "status_code", None)
                       for response in (candidate, getattr(candidate, "response", None))
                       if isinstance(getattr(response, "status_code", None), int)), None)
        text = str(candidate).lower()
        error_name = type(candidate).__name__.lower()
        metadata = provider_error_metadata(candidate)
        codes = metadata["codes"]
        if any(code in {"insufficient_quota", "quota_exceeded", "credit_balance_exhausted",
                        "organization_spend_limit_exceeded", "project_spend_limit_exceeded",
                        "organization_usage_limit_exceeded", "billing_error"} for code in codes):
            return "quota_exhausted"
        if any(code in {"rate_limit_exceeded", "rate_limit_error", "too_many_requests"}
               for code in codes):
            return "rate_limited"
        grpc_code = getattr(candidate, "code", None)
        grpc_name = getattr(grpc_code, "name", "")
        if callable(grpc_code):
            try:
                grpc_name = getattr(grpc_code(), "name", grpc_name)
            except Exception:
                grpc_name = ""
        if str(grpc_name).upper() == "RESOURCE_EXHAUSTED":
            return "quota_exhausted"
        if any(marker in text for marker in ("api_key", "api key", "apikey")) and any(
            marker in text for marker in ("required", "must be set", "not provided", "missing", "none")
        ):
            return "missing_api_key"
        if status == 401 or any(marker in text for marker in ("unauthorized", "invalid api key", "authentication")):
            return "authentication"
        if status == 403 or any(marker in text for marker in ("forbidden", "permission denied", "not permitted")):
            return "permission_denied"
        if status == 404 or any(marker in text for marker in ("model not found", "does not exist", "unknown model")):
            return "model_not_found"
        # HTTP 402 is billing/payment, distinct from quota and rate limiting.
        if status == 402 or any(marker in text for marker in (
                "insufficient credits", "insufficient balance", "billing quota", "payment required")):
            return "payment_required"
        # Google may wrap ResourceExhausted without preserving its HTTP status.
        if any(marker in text or marker in error_name for marker in (
                "resourceexhausted", "resource exhausted", "exceeded your current quota",
                "quota exhausted", "quota exceeded")):
            return "quota_exhausted"
        if status == 429 or any(marker in text for marker in (
                "rate limit", "too many requests", "requests per minute")):
            return "rate_limited"
        if isinstance(candidate, TimeoutError) or "timeout" in text or "timeout" in error_name:
            return "timeout"
        if isinstance(candidate, (ImportError, ModuleNotFoundError)):
            return "dependency"
        if isinstance(candidate, ConnectionError) or any(marker in text or marker in error_name for marker in (
                "connectionerror", "connecterror", "connection refused", "failed to establish a new connection",
                "network is unreachable", "name or service not known")):
            return "connection_error"
    return "provider_unavailable"

def provider_error_user_message(error: Exception):
    """Explain a provider failure without exposing SDK text or secrets."""
    category = classify_provider_error(error)
    guidance = {
        "missing_api_key": "Add the selected provider's API key in Agent Settings.",
        "authentication": "The provider rejected this credential; verify the key for the selected provider.",
        "permission_denied": "The credential lacks access to this model or project; check provider permissions.",
        "model_not_found": "The provider does not recognize this model ID; verify the selected model.",
        "payment_required": "The provider reports billing or payment is required; this is distinct from a rate limit.",
        "quota_exhausted": "The provider reports quota exhausted or unavailable for this key, project, or model; check the provider quota and billing details.",
        "rate_limited": "The provider rate-limited requests; wait before retrying or reduce request frequency.",
        "timeout": "The provider request timed out; check connectivity and retry later.",
        "connection_error": "CCad could not connect to the provider; check network access and the provider endpoint.",
        "dependency": "The provider adapter dependency is unavailable; install the configured adapter.",
        "provider_unavailable": "The provider request failed for an unclassified reason; check provider status and network settings.",
    }
    status = provider_http_status(error)
    status_text = f" (HTTP {status})" if status is not None else ""
    retry_after = provider_retry_after_seconds(error)
    retry_text = f" Retry after {retry_after} seconds as requested by the provider." if retry_after else ""
    return ("Provider request stopped before a response was completed. "
            f"{guidance.get(category, guidance['provider_unavailable'])} "
            f"Cause: {category}{status_text}.{retry_text}")

def emit_dependency_warning(module_name: str):
    """Give users a safe, copyable remedy when an optional adapter is absent."""
    emit({"jsonrpc": "2.0", "method": "message", "params": {
        "text": (f"Provider adapter unavailable: {module_name}. "
                 "Install agent dependencies with "
                 "python -m pip install -r src/ccad_agent/requirements.txt.")}})

def emit_provider_ready(provider: str, model: str):
    """Report configured adapter readiness; do not imply network probe."""
    emit({"jsonrpc": "2.0", "method": "provider_state", "params": {
        "provider": provider, "model": model, "configured": True,
        "execution_enabled": True, "network_access": "not_probed",
        "secret_value_visible": False,
    }})

def provider_timeout_seconds():
    """Return bounded provider request timeout in seconds."""
    try:
        value = int(os.environ.get("CCAD_PROVIDER_TIMEOUT_SECONDS", "60"))
    except ValueError:
        value = 60
    return min(120, max(1, value))


def active_provider_model():
    """Resolve the selected adapter/model using the provider-startup rules."""
    provider = os.environ.get("CCAD_PROVIDER") or config_manager.get("provider", "openai")
    if not isinstance(provider, str) or not provider:
        provider = "openai"
    model_name = os.environ.get("CCAD_MODEL") or config_manager.get("model", "")
    if not isinstance(model_name, str):
        model_name = ""
    adapter_provider = "local_model" if provider == "local_model_server" else provider
    provider_model_env = {
        "openai_compatible": "CCAD_OPENAI_COMPATIBLE_MODEL",
        "openrouter": "CCAD_OPENROUTER_MODEL",
        "cerebras": "CCAD_CEREBRAS_MODEL",
        "local_model": "CCAD_LOCAL_MODEL_NAME",
        "ollama": "CCAD_OLLAMA_MODEL",
        "google_gemini": "CCAD_GEMINI_MODEL",
    }.get(adapter_provider)
    if provider_model_env:
        model_name = os.environ.get(provider_model_env) or model_name
    return adapter_provider, model_name

def init_provider():
    global llm, router_llm, librarian_llm, broker_wait_enabled
    failure_category = "provider_unavailable"

    # Reconfiguration must not retain a previously initialized adapter or its
    # credential-backed client after a key/provider is removed.
    llm = None
    router_llm = None
    librarian_llm = None
    broker_wait_enabled = False
    
    provider, model_name = active_provider_model()

    if provider == "anthropic":
        try:
            from langchain_anthropic import ChatAnthropic
            if not model_name: model_name = "claude-opus-5"
            llm = cast(Any, ChatAnthropic)(model=model_name, temperature=0, max_retries=0)
            router_llm = bind_native_tools(llm)
            librarian_llm = router_llm
            broker_wait_enabled = True
            emit_provider_ready(provider, model_name)
            return True
        except ImportError:
            emit_dependency_warning("langchain_anthropic")
        except Exception as error:
            failure_category = classify_provider_error(error)
            failure_category = emit_provider_failure(provider, error)
    if provider == "google_gemini":
        try:
            # This dependency currently imports the retired
            # google.generativeai cache module. Scope suppression to that
            # import only; genuine runtime/provider warnings remain visible.
            with warnings.catch_warnings():
                warnings.filterwarnings(
                    "ignore",
                    message=r"All support for the `google\.generativeai` package has ended\.",
                    category=FutureWarning,
                )
                from langchain_google_genai import ChatGoogleGenerativeAI
            if not model_name: model_name = "gemini-3.8-flash"
            # The UI/API uses the provider-neutral GEMINI_API_KEY name; the
            # LangChain Google adapter reads GOOGLE_API_KEY.
            google_api_key = os.environ.get("GEMINI_API_KEY") or os.environ.get("GOOGLE_API_KEY")
            if google_api_key:
                os.environ["GOOGLE_API_KEY"] = google_api_key
            llm = ChatGoogleGenerativeAI(model=model_name, temperature=0)
            router_llm = bind_native_tools(llm)
            librarian_llm = router_llm
            broker_wait_enabled = True
            emit_provider_ready(provider, model_name)
            return True
        except ImportError:
            emit_dependency_warning("langchain_google_genai")
        except Exception as error:
            failure_category = emit_provider_failure(provider, error)
    if provider in ("openai", "openai_compatible", "openrouter", "local_model", "cerebras", "ollama"):
        try:
            from langchain_openai import ChatOpenAI
            if provider == "openai_compatible":
                model_name = model_name or os.environ.get("CCAD_OPENAI_COMPATIBLE_MODEL", "") or "default"
                base_url = os.environ.get("CCAD_OPENAI_COMPATIBLE_BASE_URL", "")
            elif provider == "openrouter":
                model_name = model_name or "openrouter/free"
                base_url = "https://openrouter.ai/api/v1"
            elif provider == "cerebras":
                # Keep backend fallback on a current Cerebras production model.
                # Explicit CCAD_CEREBRAS_MODEL still wins; custom IDs remain
                # available for preview/dedicated endpoints.
                model_name = model_name or "gpt-oss-120b"
                base_url = "https://api.cerebras.ai/v1"
            elif provider == "local_model":
                model_name = model_name or os.environ.get("CCAD_LOCAL_MODEL_NAME", "") or "local-model"
                base_url = os.environ.get("CCAD_LOCAL_MODEL_BASE_URL", "http://127.0.0.1:1234/v1")
            elif provider == "ollama":
                model_name = model_name or "qwen3"
                base_url = os.environ.get("CCAD_OLLAMA_BASE_URL", "http://127.0.0.1:11434/v1")
            else:
                model_name = model_name or "gpt-5.1"
                base_url = ""
            kwargs = {"model": model_name, "temperature": 0,
                      "timeout": provider_timeout_seconds(), "max_retries": 0}
            if base_url:
                kwargs["base_url"] = base_url
            if provider == "cerebras":
                kwargs["default_headers"] = {
                    "X-Cerebras-3rd-Party-Integration": "langgraph"
                }
                effort_options = {
                    "gpt-oss-120b": {"low", "medium", "high"},
                    "qwen-3.8-27b": {"none", "low", "medium", "high"},
                }
                requested_effort = os.environ.get("CCAD_CEREBRAS_REASONING_EFFORT", "").strip().lower()
                default_effort = "none" if model_name == "qwen-3.8-27b" else "medium"
                if requested_effort not in effort_options.get(model_name, set()):
                    requested_effort = default_effort
                kwargs["reasoning_effort"] = requested_effort
            provider_keys = {
                "openai": "OPENAI_API_KEY",
                "openai_compatible": "CCAD_OPENAI_COMPATIBLE_API_KEY",
                "openrouter": "OPENROUTER_API_KEY",
                "cerebras": "CEREBRAS_API_KEY",
                "local_model": "CCAD_LOCAL_MODEL_API_KEY",
                "ollama": "CCAD_OLLAMA_API_KEY",
            }
            api_key_name = provider_keys.get(provider)
            if api_key_name and os.environ.get(api_key_name):
                kwargs["api_key"] = os.environ[api_key_name]
            elif provider == "ollama":
                # Ollama's OpenAI-compatible localhost endpoint ignores the
                # bearer token, but langchain-openai requires a nonempty value.
                kwargs["api_key"] = "ollama"
            llm = ChatOpenAI(**kwargs)
            router_llm = bind_native_tools(llm)
            librarian_llm = router_llm
            broker_wait_enabled = True
            emit_provider_ready(provider, model_name)
            return True
        except ImportError:
            emit_dependency_warning("langchain_openai")
        except Exception as error:
            failure_category = emit_provider_failure(provider, error)
    if failure_category == "missing_api_key":
        provider_message = (f"Agent provider '{provider}' is not configured. "
                            f"Add its API key in Agent Settings; local CCad tools remain available.")
    else:
        provider_message = (f"Agent provider '{provider}' unavailable. "
                            "Configure its environment in Agent Settings; local CCad tools remain available.")
    emit({"jsonrpc": "2.0", "method": "message", "params": {
        "text": provider_message,
        "kind": "provider_error", "category": failure_category,
        "secret_value_visible": False,
    }})
    return False

provider_initialized = False

def initialize_agent_process():
    """Initialize provider state only for a launched orchestration process."""
    global provider_initialized
    provider_initialized = False
    observability = reconfigure_observability()
    atexit.register(telemetry_runtime.shutdown)
    if os.environ.get("CCAD_AGENT_DEFER_PROVIDER_INIT", "").lower() not in {"1", "true", "yes"}:
        provider_initialized = init_provider()
    # Runtime readiness is independent from provider readiness. Harnesses can
    # distinguish "Python agent process is alive" from "selected API adapter
    # works" without making module import perform provider initialization.
    emit({"jsonrpc": "2.0", "method": "backend_state", "params": {
        "runtime": "python",
        "ready": True,
        "provider_initialized": bool(provider_initialized),
        "network_access": "not_probed",
        "secret_value_visible": False,
    }})
    emit({"jsonrpc": "2.0", "method": "observability_state", "params": observability})


def invoke_agent_run(state):
    """Invoke graph under one run span without exporting prompt contents."""
    thread_id = state.get("thread_id") or os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local")
    telemetry_runtime.begin_turn()
    with telemetry_runtime.session(thread_id), telemetry_runtime.observation("agent-turn", "agent", {
            "workflow": active_workflow,
            "provider_ready": llm is not None,
            "thread_id": thread_id,
            "context_chars": len(str(state.get("context", ""))),
            "context_package_digest": str(state.get("context_metadata", {}).get("package_digest", "")),
        }):
        run_config: Dict[str, Any] = {"run_name": "agent-turn"}
        run_config["configurable"] = {"thread_id": thread_id}
        # Metadata is deliberately non-content: prompt, context, tool args, and
        # credentials must not be exported by observability callbacks.
        run_config["metadata"] = {
            "ccad_provider": os.environ.get("CCAD_PROVIDER", "configured"),
            "ccad_workflow": active_workflow,
            # Langfuse propagated attributes accept strings. Keep these
            # metadata-only flags instead of emitting SDK warnings every turn.
            "ccad_context_present": "true" if state.get("context", "") else "false",
            "ccad_thread_id_present": "true" if thread_id else "false",
            "langfuse_session_id": thread_id,
            "ccad_context_package_digest": str(state.get("context_metadata", {}).get("package_digest", "")),
            "ccad_context_estimated_tokens": str(state.get("context_metadata", {}).get("estimated_token_count", "")),
        }
        run_config["tags"] = ["ccad", "agent", active_workflow,
                              os.environ.get("CCAD_PROVIDER", "configured")]
        # Bound supervisor -> specialist -> tool cycles.  This is a safety
        # limit, not a provider retry: a malformed tool call must not consume
        # quota forever while chaining is enabled.
        try:
            recursion_limit = int(os.environ.get("CCAD_AGENT_RECURSION_LIMIT", "12"))
        except ValueError:
            recursion_limit = 12
        run_config["recursion_limit"] = min(32, max(4, recursion_limit))
        callbacks = active_callbacks()
        if callbacks:
            run_config["callbacks"] = callbacks
        return executor.invoke(state, config=run_config)



def get_system_prompt(role_desc: str) -> str:
    base_prompt = config_manager.get("system_prompt", "")
    person_config = config_manager.get("personalisation", {})
    personality = person_config.get("agent_personality", "Default")
    custom_inst = person_config.get("custom_instructions", "")
    
    parts = [f"You are {role_desc}",
             "Use only tools in the native catalog. Never invent a tool, board object, layer, net, placement, preview, or successful mutation.",
             "Read the typed project context before design-specific work. Use project.state for complete live PCB/schematic state when needed.",
             "For a requested PCB layer or net, verify it exists in project context, then call ui.set_active_layer or ui.set_active_net before a dependent mutation.",
             "Treat tool results as authoritative: report a change only after performed=true; report the returned failure reason otherwise.",
             "Persistent mutations require the approval path. Use rendered proposal preview when available; never describe text-only context as a visual diff."]
    if base_prompt: parts.append(f"System Base: {base_prompt}")
    
    # Inject active workflow context
    if active_workflow == "routing_pass":
        parts.append("Current Phase: ROUTING. Focus on trace placement and available routing constraints. Use ui.route_track and ui.place_via only after validating layer/net state.")
    elif active_workflow == "placement_pass":
        parts.append("Current Phase: PLACEMENT. Focus on component alignment, signal flow, and thermal separation. Use ui.place_footprint only with an available typed footprint source.")
    elif active_workflow == "sch_to_pcb":
        parts.append("Current Phase: FORWARD ANNOTATION. Map schematic nets to board layout instances.")

    if personality == "Senior EE":
        parts.append("Personality: Senior Electrical Engineer. Prioritize signal integrity, EMI/EMC, and robust power delivery. Enforce strict DRC checks.")
    elif personality == "Super Power":
        parts.append("Personality: Super Power AI. Provide highly optimized, cutting-edge PCB routing strategies. Utilize exotic footprint placements.")
        
    if custom_inst:
        parts.append(f"Custom Instructions: {custom_inst}")
        
    return "\n".join(parts)

def invoke_provider_with_retry(client, messages, config=None):
    """Retry transient provider failures without retrying any tool execution."""
    try:
        retries = min(2, max(0, int(os.environ.get("CCAD_PROVIDER_RETRIES", "0"))))
    except ValueError:
        retries = 0

    def quota_or_rate_limited(error):
        return classify_provider_error(error) in {
            "quota_exhausted", "rate_limited", "authentication",
            "permission_denied", "model_not_found", "payment_required",
        }

    for attempt in range(retries + 1):
        try:
            return client.invoke(messages, config=config or {})
        except Exception as error:
            # Never multiply quota/credit failures. Surface immediately.
            if attempt >= retries or quota_or_rate_limited(error):
                raise
            emit({"jsonrpc": "2.0", "method": "provider_retry", "params": {
                "attempt": attempt + 1,
                "max_retries": retries,
                "error_type": type(error).__name__,
                "prompt_emitted": False,
                "secret_value_visible": False,
            }})

def provider_connection_probe(client):
    """Make exactly one explicit live request; never retry or call tools."""
    response = client.invoke([HumanMessage(content=(
        "Reply with exactly CCAD_CONNECTION_OK. Do not call tools."))])
    content = getattr(response, "content", "")
    if isinstance(content, list):
        content = " ".join(
            str(block.get("text", "")) if isinstance(block, dict) else str(block)
            for block in content)
    return re.sub(r"\s+", " ", str(content)).strip()[:240]

@trace_function("supervisor", "agent")
def supervisor_node(state: AgentState):
    if "pre node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("pre node", emit, "supervisor")
    # Enforce workflow routing
    if active_workflow == "routing_pass":
        return {"next_node": "router"}
    elif active_workflow == "placement_pass":
        return {"next_node": "librarian"}

    # Internal classifier calls doubled normal-chat cost and could leak a
    # literal FINISH token. One primary agent owns chat and all approved tools.
    next_node = "router"
    if "post node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post node", emit, f"supervisor -> {next_node}")
        
    # This is an internal classifier response, never a chat answer. Returning
    # it leaked literal FINISH/router/librarian into the user transcript when
    # chaining ended after the supervisor.
    return {"next_node": next_node}

@trace_function("router", "agent")
def router_node(state: AgentState):
    if "pre node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("pre node", emit, "router")
    if not router_llm:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Error: Router LLM not initialized."}})
        return {"messages": []}
    
    context_str = state.get("context", "")
    role_desc = "the CCad PCB Routing Expert."
    system_text = get_system_prompt(role_desc)
    system_text += f"\nContext: {context_str}"
    
    system_msg = SystemMessage(content=system_text)
    prompt = [system_msg] + state["messages"]
    callbacks = active_callbacks()
    request_context = emit_provider_request_context(
        system_text, state["messages"], context_str,
        state.get("context_metadata", {}), agent_tools,
        os.environ.get("CCAD_PROVIDER", "configured"),
        os.environ.get("CCAD_MODEL", "configured"))
    with telemetry_runtime.observation("generate-routing-response", "generation", {
            "provider": os.environ.get("CCAD_PROVIDER", "configured"),
            "model": os.environ.get("CCAD_MODEL", "configured"),
            "context_chars": str(len(context_str)),
            **provider_request_trace_metadata(request_context),
        }, model=os.environ.get("CCAD_MODEL", "configured")):
        response = invoke_provider_with_retry(
            router_llm, prompt, config={"callbacks": callbacks} if callbacks else {})
    if "post node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post node", emit, "router")
    return {"messages": [response]}

@trace_function("librarian", "agent")
def librarian_node(state: AgentState):
    if "pre node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("pre node", emit, "librarian")
    if not librarian_llm:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Error: Librarian LLM not initialized."}})
        return {"messages": []}
        
    context_str = state.get("context", "")
    role_desc = "the CCad Component Librarian."
    system_text = get_system_prompt(role_desc)
    system_text += f"\nContext: {context_str}"
    
    system_msg = SystemMessage(content=system_text)
    prompt = [system_msg] + state["messages"]
    callbacks = active_callbacks()
    request_context = emit_provider_request_context(
        system_text, state["messages"], context_str,
        state.get("context_metadata", {}), agent_tools,
        os.environ.get("CCAD_PROVIDER", "configured"),
        os.environ.get("CCAD_MODEL", "configured"))
    with telemetry_runtime.observation("generate-library-response", "generation", {
            "provider": os.environ.get("CCAD_PROVIDER", "configured"),
            "model": os.environ.get("CCAD_MODEL", "configured"),
            "context_chars": str(len(context_str)),
            **provider_request_trace_metadata(request_context),
        }, model=os.environ.get("CCAD_MODEL", "configured")):
        response = invoke_provider_with_retry(
            librarian_llm, prompt, config={"callbacks": callbacks} if callbacks else {})
    if "post node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post node", emit, "librarian")
    return {"messages": [response]}

from langgraph.prebuilt import ToolNode
execute_tool_node = ToolNode(agent_tools)

def should_route(state: AgentState):
    next_node = state.get("next_node", "FINISH")
    if next_node == "router":
        return "router"
    elif next_node == "librarian":
        return "librarian"
    return END

def should_continue(state: AgentState):
    messages = state.get("messages", [])
    if not messages:
        return END
    last_message = messages[-1]
    
    if getattr(last_message, "tool_calls", None):
        return "execute_tool"
        
    if "<TOOL>" in str(getattr(last_message, "content", "")):
        return "execute_tool"
        
    return END

def create_orchestrator():
    graph_builder = StateGraph(AgentState)
    graph_builder.add_node("supervisor", supervisor_node)
    graph_builder.add_node("router", router_node)
    graph_builder.add_node("librarian", librarian_node)
    graph_builder.add_node("execute_tool", execute_tool_node)

    graph_builder.set_entry_point("supervisor")
    graph_builder.add_conditional_edges("supervisor", should_route, {"router": "router", "librarian": "librarian", END: END})
    
    graph_builder.add_conditional_edges("router", should_continue, {"execute_tool": "execute_tool", END: END})
    graph_builder.add_conditional_edges("librarian", should_continue, {"execute_tool": "execute_tool", END: END})
    
    graph_builder.add_edge("execute_tool", "supervisor")

    if checkpoint_saver is not None:
        return graph_builder.compile(checkpointer=checkpoint_saver)
    return graph_builder.compile()

def resume_checkpointed_run(thread_id: str, resume_value):
    """Resume an interrupted graph using same durable thread identity."""
    if checkpoint_saver is None:
        return None
    config: Dict[str, Any] = {"configurable": {"thread_id": thread_id},
              "metadata": {"langfuse_session_id": thread_id},
              "tags": ["ccad", "agent", "resume"]}
    callbacks = active_callbacks()
    if callbacks:
        config["callbacks"] = callbacks
    return executor.invoke(Command(resume=resume_value), config=config)

session_messages = []
active_workflow = "default"
chaining_phase = "none"
chaining_state = True
active_hooks = []
schedules = []
context_revisions = {}
CONTEXT_REVISION_THREAD_LIMIT = 128

def bound_session_history(messages):
    """Keep interactive history bounded before it becomes provider input."""
    try:
        limit = int(os.environ.get("CCAD_AGENT_HISTORY_LIMIT", "24"))
    except ValueError:
        limit = 24
    limit = min(64, max(4, limit))
    return messages[-limit:]

def bound_context_text(context):
    """Cap provider-bound project context while preserving truncation signal."""
    if not isinstance(context, str):
        context = str(context or "")
    limit = agent_context_limit()
    if len(context) <= limit:
        return context
    marker = "\n[CCAD context truncated for provider safety]\n"
    return context[:max(0, limit - len(marker))] + marker

_INTAKE_RULES = (
    ("prompt_injection", re.compile(
        r"(?:ignore|disregard|override)\s+(?:all\s+)?(?:previous|prior|system|developer)\s+instructions"
        r"|reveal\s+(?:your|the)\s+system\s+prompt", re.IGNORECASE)),
    ("secret_bearing", re.compile(
        r"(?:api[_-]?key|secret|password|access[_-]?token)\s*[:=]\s*\S+"
        r"|\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b"
        r"|\bAIza[A-Za-z0-9_-]{20,}\b", re.IGNORECASE)),
)

def scan_intake(*values):
    """Reject known prompt-injection or inline-secret input patterns."""
    combined = "\n".join(str(value or "") for value in values)
    for category, pattern in _INTAKE_RULES:
        if pattern.search(combined):
            return {"accepted": False, "category": category,
                    "secret_value_visible": False}
    return {"accepted": True, "category": "none",
            "secret_value_visible": False}

def agent_context_limit():
    """Return clamped provider-bound context budget in characters."""
    try:
        limit = int(os.environ.get("CCAD_AGENT_CONTEXT_LIMIT", "32768"))
    except ValueError:
        limit = 32768
    return min(131072, max(4096, limit))

def compact_session_history(messages, thread_id):
    """Semantically compact older turns through the selected model, then verify."""
    telemetry_runtime.begin_turn()
    plan = prepare_history_compaction(messages)
    if not plan["ready"]:
        return None, {"applied": False, "reason": plan["reason"], **plan["report"]}
    model_client = llm
    if model_client is None:
        raise HistoryCompactionError("provider_unavailable")

    checkpoint_message_ids = None
    checkpoint_id = None
    graph_executor = executor
    if checkpoint_saver is not None and graph_executor is not None:
        try:
            checkpoint_state = graph_executor.get_state({"configurable": {
                "thread_id": str(thread_id)}})
        except Exception as error:
            raise HistoryCompactionError("checkpoint_state_unavailable") from error
        if getattr(checkpoint_state, "next", ()):
            raise HistoryCompactionError("thread_has_pending_graph_work")
        checkpoint_messages = list((getattr(checkpoint_state, "values", {}) or {}).get(
            "messages", []))
        if not checkpoint_messages:
            raise HistoryCompactionError("checkpoint_history_empty")
        checkpoint_message_ids = [str(getattr(message, "id", ""))
                                  for message in checkpoint_messages]
        checkpoint_config = getattr(checkpoint_state, "config", {}) or {}
        checkpoint_id = str((checkpoint_config.get("configurable", {}) or {}).get(
            "checkpoint_id", ""))
        source_message_ids = [str(getattr(message, "id", ""))
                              for message in messages]
        if (not checkpoint_id or not all(checkpoint_message_ids) or
                source_message_ids != checkpoint_message_ids):
            raise HistoryCompactionError("checkpoint_history_changed")

    provider, model_name = active_provider_model()
    report = dict(plan["report"])
    report.update(provider=provider, model=model_name,
                  tools_enabled=False, provider_request_sent=True)
    input_chars = len(plan["system_prompt"]) + len(plan["transcript"])
    report["estimated_input_tokens"] = (input_chars + 3) // 4 + 8
    emit({"jsonrpc": "2.0", "method": "context_compaction_state", "params": {
        **report, "stage": "prepared", "provider_request_sent": False,
        "secret_value_visible": False}})

    callbacks = active_callbacks()
    run_config: RunnableConfig = {
        "run_name": "ccad-history-compaction",
        "tags": ["ccad", "context-compaction", provider],
        "metadata": {"ccad_operation": "history_compaction",
                     "ccad_provider": provider, "ccad_model": model_name,
                     "ccad_thread_id_present": "true" if thread_id else "false",
                     "ccad_source_message_count": str(report["safe_source_message_count"]),
                     "ccad_input_chars": str(input_chars)},
    }
    if callbacks:
        run_config["callbacks"] = callbacks

    def summarize(compaction_plan):
        emit({"jsonrpc": "2.0", "method": "context_compaction_state", "params": {
            **report, "stage": "request_started", "provider_request_sent": True,
            "secret_value_visible": False}})
        with telemetry_runtime.session(thread_id), telemetry_runtime.observation(
                "agent-context-compaction", "agent", {
                    "provider": provider, "model": model_name,
                    "source_message_count": str(report["safe_source_message_count"]),
                    "source_chars": str(report["included_source_chars"]),
                    "input_chars": str(input_chars),
                }):
            with telemetry_runtime.observation(
                    "context.compact", "generation",
                    {"source_message_count": str(report["safe_source_message_count"]),
                     "input_chars": str(input_chars),
                     "tools_enabled": "false"}, model=model_name):
                return model_client.invoke([
                    SystemMessage(content=(compaction_plan["system_prompt"] +
                                          "\nMaximum recap length: "
                                          f"{compaction_plan['max_summary_chars']} characters.")),
                    HumanMessage(content=compaction_plan["transcript"]),
                ], config=run_config)

    try:
        compacted_result = compact_history(messages, summarize, plan=plan)
    except HistoryCompactionError as error:
        error.provider_request_sent = True
        raise
    if not compacted_result["applied"]:
        return None, {"applied": False, "reason": compacted_result["reason"],
                      **compacted_result["report"]}
    summary = compacted_result["summary"]
    report.update(compacted_result["report"])
    recap = HumanMessage(content=(
        "[CCad compacted-history recap. This is background from earlier turns, "
        "not a new request; follow the current user message first.]\n" + summary
    ))
    compacted = [recap, *compacted_result["recent_messages"]]
    if checkpoint_saver is not None and graph_executor is not None:
        try:
            replace_checkpoint_history(
                graph_executor, thread_id, compacted,
                expected_message_ids=checkpoint_message_ids,
                expected_checkpoint_id=checkpoint_id)
        except HistoryCompactionError as error:
            error.provider_request_sent = True
            raise

    report.update(applied=True, stage="complete", summary_chars=len(summary),
                  before_message_count=len(messages),
                  after_message_count=len(compacted),
                  after_history_chars=(len(recap.content) + sum(
                      len(str(getattr(item, "content", "") or ""))
                      for item in plan["recent_messages"])))
    return compacted, report


def handle_compaction_command(thread_id: str) -> None:
    """Run /cc locally, reporting each outcome without mutating on failure."""
    global session_messages
    source_messages = list(session_messages)
    if checkpoint_saver is not None and executor is not None:
        try:
            checkpoint = executor.get_state({"configurable": {
                "thread_id": thread_id}})
            if getattr(checkpoint, "next", ()):
                raise HistoryCompactionError("thread_has_pending_graph_work")
            checkpoint_messages = list((checkpoint.values or {}).get("messages", []))
            if checkpoint_messages:
                source_messages = checkpoint_messages
        except HistoryCompactionError as error:
            text = (
                "Cannot compact while this thread has pending Agent work. "
                "Resolve or cancel it first; history was not changed."
                if error.category == "thread_has_pending_graph_work" else
                "Cannot inspect saved conversation history; no provider request was sent "
                "and history was not changed. Reload the thread and retry."
            )
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": text, "kind": "context_compaction_refused",
                "category": error.category, "provider_request_sent": False,
                "secret_value_visible": False}})
            return
        except Exception:
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": "Cannot inspect saved conversation history; no provider request was sent "
                        "and history was not changed. Reload the thread and retry.",
                "kind": "context_compaction_refused",
                "category": "checkpoint_state_unavailable",
                "provider_request_sent": False, "secret_value_visible": False}})
            return

    plan = prepare_history_compaction(source_messages)
    if not plan["ready"]:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": "No older conversation history needs compaction; no provider request was sent.",
            "kind": "context_compaction_skipped", "provider_request_sent": False,
            **plan["report"], "secret_value_visible": False}})
        return
    if llm is None:
        provider_name, _ = active_provider_model()
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": f"Semantic compaction unavailable: provider '{provider_name}' is not ready. "
                    "History is unchanged; configure a working provider and retry.",
            "kind": "context_compaction_failed", "category": "provider_unavailable",
            "provider_request_sent": False, "secret_value_visible": False}})
        return

    provider_name, model_name = active_provider_model()
    emit({"jsonrpc": "2.0", "method": "message", "params": {
        "text": f"Compacting older conversation with {provider_name}/{model_name}. "
                "This sends bounded chat history to that provider and may use quota; "
                "no tools are enabled. Latest four messages stay unchanged.",
        "kind": "context_compaction_started", "provider_request_sent": False,
        "tools_enabled": False, "secret_value_visible": False}})
    try:
        compacted, report = compact_session_history(source_messages, thread_id)
        if compacted is None:
            raise HistoryCompactionError("insufficient_older_history")
        session_messages = compacted
        usage = report.get("provider_usage", {})
        emit({"jsonrpc": "2.0", "method": "observability_state",
              "params": telemetry_runtime.flush_turn()})
        emit({"jsonrpc": "2.0", "method": "context_compaction_state",
              "params": {**report, "stage": "complete",
                         "provider_request_sent": True, "tools_enabled": False,
                         "secret_value_visible": False}})
        token_detail = ""
        if isinstance(usage, dict):
            input_tokens = usage.get("input_tokens")
            output_tokens = usage.get("output_tokens")
            if isinstance(input_tokens, int) and isinstance(output_tokens, int):
                token_detail = (f" Provider usage: {input_tokens} input, "
                                f"{output_tokens} output tokens.")
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": (f"Conversation semantically compacted: "
                     f"{report['before_message_count']} → "
                     f"{report['after_message_count']} active messages; "
                     f"{report['summary_chars']} recap characters from "
                     f"{report['included_source_chars']} older-history characters. "
                     f"Latest {report['retained_message_count']} messages were preserved. "
                     f"Estimated input: ~{report['estimated_input_tokens']} tokens; "
                     f"provider request sent, no tools executed.{token_detail}"),
            "kind": "context_compaction_complete", "provider_request_sent": True,
            "tool_executed": False, "secret_value_visible": False}})
    except HistoryCompactionError as error:
        telemetry_runtime.flush_turn()
        emit({"jsonrpc": "2.0", "method": "context_compaction_state",
              "params": {"stage": "failed", "category": error.category,
                         "provider_request_sent": error.provider_request_sent,
                         "tools_enabled": False, "secret_value_visible": False}})
        changed = error.category == "checkpoint_restore_failed"
        text = ("Checkpoint recovery failed; do not continue this thread until "
                "its saved state is reloaded and inspected." if changed else
                f"Semantic compaction was not applied ({error.category}); "
                "conversation history remains unchanged.")
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": text, "kind": "context_compaction_failed",
            "category": error.category,
            "provider_request_sent": error.provider_request_sent,
            "secret_value_visible": False}})
    except Exception as error:
        telemetry_runtime.flush_turn()
        category = classify_provider_error(error)
        emit({"jsonrpc": "2.0", "method": "context_compaction_state",
              "params": {"stage": "failed", "category": category,
                         "provider_request_sent": True, "tools_enabled": False,
                         "secret_value_visible": False}})
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": ("Semantic compaction failed; conversation history was not changed. "
                     + provider_error_user_message(error)),
            "kind": "context_compaction_failed", "category": category,
            "provider_request_sent": True, "secret_value_visible": False}})

# --- Custom Workflows ---
def handle_marketplace(text: str):
    del text
    emit({"jsonrpc": "2.0", "method": "message", "params": {
        "text": "Marketplace installation is unavailable: CCad has no verified plugin installer or runtime loader. No plugin was changed."
    }})

def get_dynamic_marketplace_catalog():
    # Scan dynamic plugins if they exist, fallback to core + installed state
    installed = config_manager.get("installed_plugins", [])
    core_plugins: List[Dict[str, Any]] = [
        {"id": "plugin.autoplacer", "name": "AutoPlacer", "description": "AI-driven component placement using simulated annealing"},
        {"id": "plugin.autorouter", "name": "AutoRouter", "description": "Cloud-accelerated PCB autorouter"},
        {"id": "plugin.kicad_sync", "name": "KiCad Sync", "description": "Two-way synchronization with KiCad"},
        {"id": "plugin.otel_tracing", "name": "OTel Tracing", "description": "OpenTelemetry observability integration"},
        {"id": "plugin.freerouting", "name": "Freerouting Hook", "description": "Push-and-shove DSN router integration"},
        {"id": "plugin.ai_generator", "name": "AI Component Generator", "description": "Generate schematic symbols and footprints via LLM"}
    ]
    # Mark installed state based on actual config
    for p in core_plugins:
        p["installed"] = p["id"] in installed or p["name"] in installed
        
    return {
        "plugins": core_plugins,
        "workflows": [
            {"id": "workflow.validation", "name": "Validation Workflow", "description": "Runs full DRC/ERC checks before committing", "installed": True},
            {"id": "workflow.routing", "name": "Routing Workflow", "description": "Iterative routing and cleanup phases", "installed": active_workflow == "routing_pass"},
            {"id": "workflow.sch_to_pcb", "name": "Schematic to PCB Sync", "description": "Forward annotation of netlist and components", "installed": True}
        ]
    }

def handle_provider_and_state_request(req, executor):
    method = req.get("method")
    if method in ("agent.test_provider", "agent.test_provider_connection"):
        # Transient tests: never update config_manager or write config.
        params = req.get("params", {})
        provider_id = params.get("provider", "openai")
        model = params.get("model", "").strip()
        secret = params.get("secret", "")
        connection_requested = method == "agent.test_provider_connection"
        test_env_names = ("CCAD_PROVIDER", "CCAD_MODEL", "CCAD_GEMINI_MODEL",
                          "CCAD_OPENROUTER_MODEL", "CCAD_CEREBRAS_MODEL",
                          "CCAD_CEREBRAS_REASONING_EFFORT",
                          "CCAD_OPENAI_COMPATIBLE_MODEL", "CCAD_LOCAL_MODEL_NAME",
                          "CCAD_OLLAMA_MODEL", "CCAD_OLLAMA_BASE_URL",
                          "OPENAI_API_KEY", "ANTHROPIC_API_KEY", "GEMINI_API_KEY",
                          "GOOGLE_API_KEY", "OPENROUTER_API_KEY", "CEREBRAS_API_KEY",
                          "CCAD_OPENAI_COMPATIBLE_API_KEY", "CCAD_LOCAL_MODEL_API_KEY",
                          "CCAD_OLLAMA_API_KEY")
        saved_test_env = {name: os.environ.get(name) for name in test_env_names}
        saved_session_provider_env = set(session_provider_env)
        os.environ["CCAD_PROVIDER"] = provider_id
        if model:
            os.environ["CCAD_MODEL"] = model
            model_env = {"google_gemini": "CCAD_GEMINI_MODEL",
                         "openai_compatible": "CCAD_OPENAI_COMPATIBLE_MODEL",
                         "openrouter": "CCAD_OPENROUTER_MODEL",
                         "cerebras": "CCAD_CEREBRAS_MODEL",
                         "local_model": "CCAD_LOCAL_MODEL_NAME",
                         "ollama": "CCAD_OLLAMA_MODEL",
                         "local_model_server": "CCAD_LOCAL_MODEL_NAME"}.get(provider_id)
            if model_env: os.environ[model_env] = model
        env_names = {"openai": "OPENAI_API_KEY", "anthropic": "ANTHROPIC_API_KEY",
                     "google_gemini": "GEMINI_API_KEY",
                     "openai_compatible": "CCAD_OPENAI_COMPATIBLE_API_KEY",
                     "openrouter": "OPENROUTER_API_KEY",
                     "cerebras": "CEREBRAS_API_KEY",
                     "local_model": "CCAD_LOCAL_MODEL_API_KEY",
                     "ollama": "CCAD_OLLAMA_API_KEY",
                     "local_model_server": "CCAD_LOCAL_MODEL_API_KEY"}
        env_name = env_names.get(provider_id, "OPENAI_API_KEY")
        clear_session_provider_env()
        if secret:
            set_session_provider_env(env_name, secret)
            if provider_id == "google_gemini": set_session_provider_env("GOOGLE_API_KEY", secret)
        provider_ready = init_provider()
        # `init_provider` can emit ambient provider_state events for the
        # temporary selection.  The GUI must not use those to decide a
        # settings validation result because restoring the active
        # provider emits another ambient state afterwards.
        test_error = "" if provider_ready else (
            "provider_unavailable" if secret else "missing_api_key")
        test_category = "" if provider_ready else (
            "provider_unavailable" if secret else "missing_api_key")
        connection_preview = ""
        connection_category = test_category
        connection_status = None
        connection_attempted = False
        if connection_requested and provider_ready and llm is not None:
            try:
                # This is deliberately not invoke_provider_with_retry:
                # user clicked an explicit quota-spending connection test.
                connection_attempted = True
                connection_preview = provider_connection_probe(llm)
                connection_category = ""
            except Exception as error:
                connection_category = classify_provider_error(error)
                connection_status = provider_http_status(error)
        clear_session_provider_env()
        for name, value in saved_test_env.items():
            if value is None:
                os.environ.pop(name, None)
            else:
                os.environ[name] = value
        session_provider_env.update(saved_session_provider_env)
        init_provider()
        if connection_requested:
            emit({"jsonrpc": "2.0", "method": "provider_connection_result", "params": {
                "provider": provider_id,
                "model": model,
                "connected": provider_ready and not connection_category,
                "response_preview": connection_preview,
                "error_category": connection_category,
                "http_status": connection_status,
                "network_access": "explicit_one_request",
                "tool_executed": False,
                "request_count": 1 if connection_attempted else 0,
                "secret_value_visible": False,
            }})
        else:
            # This adapter-only validation intentionally sends no request.
            emit({"jsonrpc": "2.0", "method": "provider_test_result", "params": {
                "provider": provider_id,
                "model": model,
                "configured": bool(secret),
                "execution_enabled": provider_ready,
                "network_access": "not_probed",
                "error": test_error,
                "error_category": test_category,
                "secret_value_visible": False,
            }})
    elif method == "agent.set_provider_secret":
        # Private IPC only. Never emit, persist, or add credential to
        # prompts. Provider SDK reads process memory via its env var.
        provider_id = req.get("params", {}).get("provider", "openai")
        secret = req.get("params", {}).get("secret", "")
        env_names = {
            "openai": "OPENAI_API_KEY",
            "anthropic": "ANTHROPIC_API_KEY",
            "google_gemini": "GEMINI_API_KEY",
            "openai_compatible": "CCAD_OPENAI_COMPATIBLE_API_KEY",
            "openrouter": "OPENROUTER_API_KEY",
            "cerebras": "CEREBRAS_API_KEY",
            "local_model": "CCAD_LOCAL_MODEL_API_KEY",
            "ollama": "CCAD_OLLAMA_API_KEY",
            "local_model_server": "CCAD_LOCAL_MODEL_API_KEY",
        }
        env_name = env_names.get(provider_id, "OPENAI_API_KEY")
        clear_session_provider_env()
        if secret:
            set_session_provider_env(env_name, secret)
            if provider_id == "google_gemini":
                set_session_provider_env("GOOGLE_API_KEY", secret)
        provider_ready = init_provider()
        # Complete the selected key operation with a dedicated event.
        # Ambient provider_state traffic is not reliable Settings UI
        # feedback because set_config may have emitted an earlier state.
        emit({"jsonrpc": "2.0", "method": "provider_secret_result", "params": {
            "provider": provider_id,
            "configured": bool(secret),
            "execution_enabled": provider_ready,
            "network_access": "not_probed",
            "error": "" if provider_ready else ("provider_unavailable" if secret else "missing_api_key"),
            "error_category": "" if provider_ready else ("provider_unavailable" if secret else "missing_api_key"),
            "secret_value_visible": False,
        }})
    elif method == "agent.set_thread_id":
        params = req.get("params", {})
        thread_id = str(params.get("thread_id", "")).strip()
        if thread_id:
            os.environ["CCAD_AGENT_THREAD_ID"] = thread_id
            session_id = str(params.get("session_id") or thread_id)
            task_id = memory_task_scopes.current(session_id)
            memory_manager.set_identities(
                task_id=task_id or uuid.uuid4().hex,
                thread_id=thread_id,
                project_id=str(params.get("project_id") or "project"),
                retain_stm_task=bool(task_id))
            memory_manager.configure(config_manager.get("memory", {}))
        else:
            os.environ.pop("CCAD_AGENT_THREAD_ID", None)
        emit({"jsonrpc": "2.0", "method": "thread_state", "params": {
            "configured": bool(thread_id), "memory_tiers": memory_manager.state(),
            "secret_value_visible": False,
        }})
    elif method == "agent.resume_thread":
        thread_id = os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local")
        if checkpoint_saver is None:
            emit({"jsonrpc": "2.0", "method": "thread_state", "params": {
                "resumable": False, "reason": "checkpoint_disabled",
            }})
        else:
            snapshot = executor.get_state({"configurable": {"thread_id": thread_id}})
            resume_value = req.get("params", {}).get("resume")
            if resume_value is not None and snapshot.next:
                resumed = resume_checkpointed_run(thread_id, resume_value)
                emit({"jsonrpc": "2.0", "method": "thread_resumed", "params": {
                    "thread_id": thread_id,
                    "next": list(executor.get_state({"configurable": {"thread_id": thread_id}}).next),
                    "message_count": len(resumed.get("messages", [])) if isinstance(resumed, dict) else 0,
                }})
                snapshot = executor.get_state({"configurable": {"thread_id": thread_id}})
            emit({"jsonrpc": "2.0", "method": "thread_state", "params": {
                "resumable": bool(snapshot.values), "thread_id": thread_id,
                "next": list(snapshot.next), "checkpoint_id": snapshot.config.get("configurable", {}).get("checkpoint_id", ""),
            }})
    elif method == "agent.methods":
        emit({"jsonrpc": "2.0", "method": "agent_methods",
              "params": orchestrator_method_catalog()})
    elif method == "agent.list_models":
        raw_provider = req.get("params", {}).get("provider", "openrouter")
        if not isinstance(raw_provider, str):
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": "", "ok": False,
                              "error": "invalid_params",
                              "error_detail": "provider must be a string",
                              "network_access": "none", "models": []}})
            return True
        provider_id = raw_provider.strip().lower()
        if provider_id == "openai":
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, **fetch_openai_models()}})
        elif provider_id == "anthropic":
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, **fetch_anthropic_models()}})
        elif provider_id == "google_gemini":
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, **fetch_gemini_models()}})
        elif provider_id == "openrouter":
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, **fetch_openrouter_models()}})
        elif provider_id == "cerebras":
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, **fetch_cerebras_models()}})
        elif provider_id == "ollama":
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, **fetch_ollama_models()}})
        else:
            emit({"jsonrpc": "2.0", "method": "provider_models",
                  "params": {"provider": provider_id, "ok": False,
                              "error": "unsupported_provider",
                              "error_detail": "model catalog is unavailable for this provider",
                              "network_access": "explicit_refresh",
                              "models": []}})
    elif method == "agent.pending_calls":
        thread_id = req.get("params", {}).get("thread_id", "")
        emit({"jsonrpc": "2.0", "method": "pending_calls_state", "params":
              pending_call_snapshot(str(thread_id))})
    elif method == "agent.context_state":
        requested_thread = req.get("params", {}).get("thread_id", "")
        thread_id = str(requested_thread or
                        os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local"))
        emit({"jsonrpc": "2.0", "method": "context_state_snapshot", "params": {
            "thread_id": thread_id,
            "revision": context_revisions.get(thread_id, ""),
            "content_emitted": False,
            "secret_value_visible": False,
        }})
    elif method == "agent.memory_state":
        tier = req.get("params", {}).get("tier")
        try:
            state = memory_manager.state(str(tier)) if tier else memory_manager.state()
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tiers": state if tier is None else {str(tier): state},
                "secret_value_visible": False,
            }})
        except (TypeError, ValueError) as error:
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tiers": {}, "error": str(error), "secret_value_visible": False}})
    elif method == "agent.memory_set_enabled":
        params = req.get("params", {})
        tier = str(params.get("tier", ""))
        requested = params.get("enabled")
        if tier not in memory_manager.TIERS or not isinstance(requested, bool):
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tier": tier, "enabled": False, "persisted": False,
                "error": "invalid_memory_preference", "secret_value_visible": False}})
            return True
        previous_config = dict(config_manager.get("memory", {}))
        previous_enabled = memory_manager.enabled[tier]
        memory_config = dict(previous_config)
        memory_config[tier] = requested
        try:
            config_manager.update_checked("memory", memory_config)
        except ConfigPersistenceError as error:
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tier": tier, **memory_manager.state(tier), "persisted": False,
                "error": error.category, "tiers": memory_manager.state(),
                "secret_value_visible": False}})
            return True
        try:
            state = (memory_manager.enable(tier) if requested
                     else memory_manager.disable(tier))
        except (TypeError, ValueError, RuntimeError, MemoryStoreError) as error:
            try:
                config_manager.update_checked("memory", previous_config)
            except ConfigPersistenceError:
                pass
            state = memory_manager.state(tier)
            category = getattr(error, "category", "memory_activation_failed")
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tier": tier, **state, "enabled": previous_enabled,
                "persisted": config_manager.get("memory", {}) == previous_config,
                "error": category, "tiers": memory_manager.state(),
                "secret_value_visible": False}})
            return True
        emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
            "tier": tier, **state, "persisted": True,
            "tiers": memory_manager.state(), "secret_value_visible": False}})
    elif method == "agent.memory_reset":
        params = req.get("params", {})
        tier = params.get("tier")
        tier = str(tier) if tier else None
        try:
            if not bool(params.get("confirmed", False)):
                raise ValueError("explicit confirmation required to reset persistent memories")
            removed = memory_manager.reset(tier)
            emit({"jsonrpc": "2.0", "method": "memory_reset", "params": {
                "tier": tier or "all", "removed": removed,
                "secret_value_visible": False}})
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tiers": memory_manager.state(), "secret_value_visible": False}})
        except (TypeError, ValueError, MemoryStoreError) as error:
            emit({"jsonrpc": "2.0", "method": "memory_reset", "params": {
                "tier": tier or "all", "removed": 0,
                "error": getattr(error, "category", str(error)),
                "secret_value_visible": False}})
    elif method in {"agent.memory_list", "agent.memory_add", "agent.memory_update",
                    "agent.memory_delete"}:
        params = req.get("params", {})
        try:
            if method == "agent.memory_list":
                tier = str(params.get("tier", "")).strip() or None
                scope = str(params.get("scope", "")).strip() or None
                result = {"entries": memory_manager.list(tier=tier, scope=scope),
                          "tier": tier or "all", "secret_value_visible": False}
                event = "memory_state"
            elif method == "agent.memory_add":
                tier = str(params.get("tier", "ltm"))
                if tier == "stm" and not memory_task_scopes.is_active(
                        memory_manager.identities["stm"]):
                    raise ValueError("start a task with `/task start` before adding STM")
                entry = memory_manager.add(
                    str(params.get("content", "")),
                    tier=tier,
                    scope=str(params.get("scope", "")),
                    title=str(params.get("title", "")))
                result = {"id": entry["id"], "tier": entry["tier"],
                          "scope": entry["scope"], "secret_value_visible": False}
                event = "memory_added"
            elif method == "agent.memory_update":
                entry = memory_manager.update(
                    str(params.get("id", "")), str(params.get("content", "")),
                    title=params.get("title"), scope=params.get("scope"))
                result = {"id": str(params.get("id", "")), "updated": entry is not None,
                          "secret_value_visible": False}
                event = "memory_updated"
            else:
                if not bool(params.get("confirmed", False)):
                    raise ValueError("explicit confirmation required to delete a memory")
                result = {"removed": memory_manager.delete(str(params.get("id", ""))),
                          "secret_value_visible": False}
                event = "memory_deleted"
            emit({"jsonrpc": "2.0", "method": event, "params": result})
        except (TypeError, ValueError, RuntimeError) as error:
            emit({"jsonrpc": "2.0", "method": "memory_error", "params": {
                "operation": method, "error": str(error),
                "secret_value_visible": False}})

    return method in {
        "agent.test_provider",
        "agent.test_provider_connection",
        "agent.set_provider_secret",
        "agent.set_thread_id",
        "agent.resume_thread",
        "agent.methods",
        "agent.list_models",
        "agent.pending_calls",
        "agent.context_state",
        "agent.memory_state",
        "agent.memory_set_enabled",
        "agent.memory_reset",
        "agent.memory_list",
        "agent.memory_add",
        "agent.memory_update",
        "agent.memory_delete",
    }

if __name__ == "__main__":
    initialize_agent_process()
    # LangfuseRuntime owns tracing.  Do not install the process-global
    # LangChain auto-instrumentor: it can duplicate spans and bypass the
    # metadata-only exporter configured by Agent Settings.

    init_checkpointer()
    executor = create_orchestrator()
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Python Multi-Agent Orchestrator ready."}})
    
    inbound_queue = queue.Queue()
    input_queue = inbound_queue
    def read_protocol_lines():
        for protocol_line in sys.stdin:
            if not route_protocol_line(protocol_line):
                input_queue.put(protocol_line)
        input_queue.put(None)
    threading.Thread(target=read_protocol_lines, name="ccad-agent-stdin", daemon=True).start()

    while True:
        try:
            line = deferred_queue.get_nowait()
        except queue.Empty:
            line = inbound_queue.get()
        if line is None:
            break
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
            method = req.get("method")
            if handle_provider_and_state_request(req, executor):
                continue
            elif method == "tool_result":
                # Accept broker response by correlation ID. If graph is paused
                # at an interrupt, feed authoritative result into same thread.
                result = req.get("result")
                error = req.get("error")
                tool_result_params = req.get("params", {})
                if not isinstance(tool_result_params, dict):
                    tool_result_params = {}
                raw_call_id = req.get("id") or tool_result_params.get("call_id", "agent-tool-call")
                call_id = raw_call_id if isinstance(raw_call_id, str) else str(raw_call_id)
                thread_id = str(tool_result_params.get("thread_id") or
                                os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local"))
                with pending_calls_lock:
                    pending_result_queue = pending_calls.get(call_id)
                if (checkpoint_saver is None or executor is None) and \
                        pending_result_queue is None:
                    emit({"jsonrpc": "2.0", "method": "tool_result_ignored", "params": {
                        "call_id": call_id, "reason": "unknown_or_late_call",
                    }})
                    continue
                if checkpoint_saver is not None and executor is not None:
                    snapshot = executor.get_state({"configurable": {"thread_id": thread_id}})
                    expected_call_id = ""
                    for checkpoint_task in snapshot.tasks:
                        for checkpoint_interrupt in getattr(checkpoint_task, "interrupts", ()):
                            value = getattr(checkpoint_interrupt, "value", {})
                            if isinstance(value, dict) and value.get("call_id"):
                                expected_call_id = value["call_id"]
                                break
                        if expected_call_id:
                            break
                    received_call_id = str(call_id)
                    if not snapshot.next or not expected_call_id:
                        emit({"jsonrpc": "2.0", "method": "tool_result_ignored", "params": {
                            "call_id": received_call_id, "thread_id": thread_id,
                            "reason": "no_pending_checkpoint",
                        }})
                        continue
                    if received_call_id != str(expected_call_id):
                        emit({"jsonrpc": "2.0", "method": "tool_result_ignored", "params": {
                            "call_id": received_call_id, "thread_id": thread_id,
                            "expected_call_id": expected_call_id,
                            "reason": "call_id_mismatch",
                        }})
                        continue
                    emit({"jsonrpc": "2.0", "method": "tool_result_ack", "params": {
                        "call_id": call_id,
                        "success": error is None and result is not None,
                        "result_present": result is not None,
                        "error_present": error is not None,
                    }})
                    resume_value = {"error": error} if error is not None else result
                    resumed = resume_checkpointed_run(thread_id, resume_value)
                    emit({"jsonrpc": "2.0", "method": "thread_resumed", "params": {
                        "thread_id": thread_id,
                        "call_id": received_call_id,
                        "message_count": len(resumed.get("messages", [])) if isinstance(resumed, dict) else 0,
                    }})
                else:
                    if pending_result_queue is None:
                        emit({"jsonrpc": "2.0", "method": "tool_result_ignored", "params": {
                            "call_id": call_id, "reason": "unknown_or_late_call",
                        }})
                        continue
                    emit({"jsonrpc": "2.0", "method": "tool_result_ack", "params": {
                        "call_id": call_id,
                        "success": error is None and result is not None,
                        "result_present": result is not None,
                        "error_present": error is not None,
                    }})
                    pending_result_queue.put(json.dumps({
                        "jsonrpc": "2.0", "method": "tool_result", "id": call_id,
                        "result": result, "error": error,
                    }))
            elif method == "agent.cancel_tool":
                cancel_params = req.get("params", {})
                if not isinstance(cancel_params, dict):
                    cancel_params = {}
                raw_call_id = cancel_params.get("call_id", "")
                call_id = raw_call_id if isinstance(raw_call_id, str) else str(raw_call_id)
                reason = str(cancel_params.get("reason", "canceled_by_user"))
                thread_id = str(cancel_params.get("thread_id") or
                                os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local"))
                with pending_calls_lock:
                    pending_result_queue = pending_calls.get(call_id)
                if pending_result_queue is None:
                    if checkpoint_saver is not None and executor is not None:
                        snapshot = executor.get_state({"configurable": {"thread_id": thread_id}})
                        expected_call_id = ""
                        for checkpoint_task in snapshot.tasks:
                            for checkpoint_interrupt in getattr(checkpoint_task, "interrupts", ()):
                                value = getattr(checkpoint_interrupt, "value", {})
                                if isinstance(value, dict) and value.get("call_id"):
                                    expected_call_id = str(value["call_id"])
                                    break
                            if expected_call_id:
                                break
                        if snapshot.next and expected_call_id == call_id:
                            resumed = resume_checkpointed_run(
                                thread_id, {"error": {"code": -32800, "message": reason}})
                            emit({"jsonrpc": "2.0", "method": "tool_canceled", "params": {
                                "call_id": call_id, "thread_id": thread_id, "reason": reason,
                            }})
                            emit({"jsonrpc": "2.0", "method": "thread_resumed", "params": {
                                "thread_id": thread_id, "call_id": call_id,
                                "message_count": len(resumed.get("messages", []))
                                if isinstance(resumed, dict) else 0,
                            }})
                            continue
                    emit({"jsonrpc": "2.0", "method": "tool_cancel_ignored", "params": {
                        "call_id": call_id, "thread_id": thread_id,
                        "reason": "unknown_or_late_call",
                    }})
                    continue
                pending_result_queue.put(json.dumps({
                    "jsonrpc": "2.0", "method": "tool_result", "id": call_id,
                    "error": {"code": -32800, "message": str(reason)},
                }))
                emit({"jsonrpc": "2.0", "method": "tool_canceled", "params": {
                    "call_id": call_id, "reason": str(reason),
                }})
            elif method == "human_message":
                params = req.get("params", {})
                text = params.get("text", "")
                raw_context = params.get("context", "")
                requested_thread = str(params.get("thread_id") or
                                       os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local"))
                requested_session = str(params.get("session_id") or requested_thread)
                requested_task = (memory_task_scopes.current(requested_session)
                                  or uuid.uuid4().hex)
                task_is_active = memory_task_scopes.is_active(requested_task)
                os.environ["CCAD_AGENT_THREAD_ID"] = requested_thread
                project_id = str(params.get("project_id") or
                                 config_manager.get("project_name", "project"))
                memory_manager.set_identities(task_id=requested_task,
                                              thread_id=requested_thread,
                                              project_id=project_id,
                                              retain_stm_task=task_is_active)
                memory_manager.configure(config_manager.get("memory", {}))
                if not isinstance(raw_context, str):
                    raw_context = str(raw_context or "")
                memory_query = text
                if text.partition(" ")[0].casefold() == "/context":
                    memory_query = text.partition(" ")[2].strip()
                    if memory_query.casefold().startswith("preview "):
                        memory_query = memory_query[8:].strip()
                memory_entries, memory_retrieval = local_memory_entries(memory_query)
                memory_runtime = memory_manager.state()
                with telemetry_runtime.observation("assemble-context", "retriever", {
                        "memory_entry_count": len(memory_entries),
                        "history_message_count": len(session_messages),
                        "raw_context_chars": len(raw_context),
                    }):
                    package = build_context_package(
                        raw_context, memory_entries, bound_session_history(session_messages),
                        char_limit=agent_context_limit(),
                        memory_retrieval=memory_retrieval,
                        memory_runtime=memory_runtime)
                context_str = package["content"]
                context_metadata = package["metadata"]
                context_truncated = context_metadata["truncated"]
                intake = scan_intake(text, raw_context)
                emit({"jsonrpc": "2.0", "method": "intake_state", "params": intake})
                if not intake["accepted"]:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": "Request blocked by CCad intake guardrail; remove instruction injection or inline secret and retry.",
                        "kind": "intake_blocked", "category": intake["category"],
                        "secret_value_visible": False,
                    }})
                    continue
                context_thread_id = requested_thread
                previous_context_revision = context_revisions.get(context_thread_id, "")
                current_context_revision = context_metadata["project_revision"]
                context_changed = current_context_revision != previous_context_revision
                context_change_kind = (
                    "initial" if not previous_context_revision else
                    ("changed" if context_changed else "unchanged")
                )
                context_revisions[context_thread_id] = current_context_revision
                if len(context_revisions) > CONTEXT_REVISION_THREAD_LIMIT:
                    oldest_thread_id = next(iter(context_revisions))
                    if oldest_thread_id != context_thread_id:
                        context_revisions.pop(oldest_thread_id, None)
                emit({"jsonrpc": "2.0", "method": "context_state", "params": {
                    "thread_id": context_thread_id,
                    "revision": current_context_revision,
                    "previous_revision": previous_context_revision,
                    "changed": context_changed,
                    "change_kind": context_change_kind,
                    "content_present": bool(context_str),
                    "content_size": context_metadata["content_size"],
                    "original_content_size": len(raw_context),
                    "context_limit": context_metadata["context_limit"],
                    "truncated": context_truncated,
                    "content_emitted": False,
                    "sources": context_metadata["sources"],
                    "memory_content_emitted": False,
                    "memory_entry_count": context_metadata["memory_entry_count"],
                    "history_message_count": context_metadata["history_message_count"],
                    "history_in_context_package": context_metadata["history_in_context_package"],
                    "history_sent_as_provider_messages": context_metadata["history_sent_as_provider_messages"],
                    "context_schema_version": context_metadata["schema_version"],
                    "package_digest": context_metadata["package_digest"],
                    "estimated_token_count": context_metadata["estimated_token_count"],
                    "project_snapshot_chars": context_metadata["project_snapshot_chars"],
                    "project_summary_chars": context_metadata["project_summary_chars"],
                    "project_source_chars": context_metadata["project_source_chars"],
                    "project_snapshot_omitted": context_metadata["project_snapshot_omitted"],
                    "omitted_memory_entry_count": context_metadata["omitted_memory_entry_count"],
                    "memory_tier_counts": context_metadata["memory_tier_counts"],
                    "memory_tier_chars": context_metadata["memory_tier_chars"],
                    "memory_retrieval": context_metadata["memory_retrieval"],
                    "memory_runtime": context_metadata["memory_runtime"],
                    "project_counts": context_metadata["project_counts"],
                }})
                
                # Robust Command Parser
                if text.startswith("/"):
                    cmd_parts = text.split(" ", 1)
                    cmd_base = cmd_parts[0].lower()
                    cmd_args = cmd_parts[1] if len(cmd_parts) > 1 else ""
                    
                    if cmd_base == "/context":
                        draft = cmd_args.strip()
                        if draft.casefold().startswith("preview "):
                            draft = draft[8:].strip()
                        preview_messages = bound_session_history(session_messages)
                        if draft:
                            preview_messages.append(HumanMessage(content=draft))
                        preview_system = get_system_prompt(
                            "the CCad PCB Routing Expert.") + f"\nContext: {context_str}"
                        preview_provider, preview_model = active_provider_model()
                        report = build_provider_request_report(
                            preview_system, preview_messages, agent_tools,
                            provider=preview_provider,
                            model=preview_model or "provider default (not resolved)",
                            context_content=context_str,
                            context_metadata=context_metadata,
                            large_context_threshold=os.environ.get(
                                "CCAD_AGENT_LARGE_CONTEXT_TOKENS", "4096"))
                        report.update({
                            "request_mode": "local_preview",
                            "provider_request_sent": False,
                            "preview_prompt_included": bool(draft),
                            "preview_prompt_chars": len(draft),
                        })
                        if os.environ.get("CCAD_TRACE_DEBUG", "").lower() in {
                                "1", "true", "yes"}:
                            print("[ccad-context-preview] " + json.dumps(
                                report, sort_keys=True), file=sys.stderr, flush=True)
                        emit({"jsonrpc": "2.0", "method": "provider_request_context",
                              "params": report})
                        emit({"jsonrpc": "2.0", "method": "message", "params": {
                            "text": format_large_context_explanation(
                                report, context_metadata, mode="preview"),
                            "kind": "context_preview",
                            "request_mode": "local_preview",
                            "provider_request_sent": False,
                            "tool_executed": False,
                            "large_context": report["large_context"],
                            "estimated_input_tokens": report["estimated_input_tokens"],
                            "large_context_threshold_tokens": report[
                                "large_context_threshold_tokens"],
                            "secret_value_visible": False,
                        }})
                        continue
                    if cmd_base == "/commands":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/context [draft]` (inspect bounded context and memory; no provider call)\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`\n- `/task start|status|end` (manage task-scoped STM)\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`"}})
                        continue
                    elif cmd_base == "/task":
                        task_action = cmd_args.strip().casefold()
                        if task_action == "start":
                            task_id, cleared = memory_task_scopes.start(requested_session)
                            memory_manager.set_identities(
                                task_id=task_id, thread_id=requested_thread,
                                project_id=project_id, retain_stm_task=True)
                            memory_manager.configure(config_manager.get("memory", {}))
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": "Task-scoped memory started. STM is isolated to this task; "
                                f"a previous task scope, if any, was cleared ({cleared} records).",
                                "kind": "memory_task_state", "active": True,
                                "runtime_entries": 0, "secret_value_visible": False}})
                        elif task_action == "end":
                            ended_id, cleared = memory_task_scopes.end(requested_session)
                            memory_manager.set_identities(
                                task_id=uuid.uuid4().hex, thread_id=requested_thread,
                                project_id=project_id, retain_stm_task=False)
                            memory_manager.configure(config_manager.get("memory", {}))
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": ("Task-scoped memory ended and its process-only "
                                         f"STM was cleared ({cleared} records)." if ended_id
                                         else "No active task-scoped memory."),
                                "kind": "memory_task_state", "active": False,
                                "runtime_entries": 0, "secret_value_visible": False}})
                        elif task_action == "status":
                            task_id = memory_task_scopes.current(requested_session)
                            memory_state = memory_manager.state("stm")
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": ("Task-scoped memory active; "
                                         f"{memory_state['runtime_entries']} STM entries loaded."
                                         if task_id else "No active task-scoped memory."),
                                "kind": "memory_task_state", "active": bool(task_id),
                                "runtime_entries": memory_state["runtime_entries"],
                                "secret_value_visible": False}})
                        else:
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": "Use `/task start`, `/task status`, or `/task end`.",
                                "kind": "memory_task_usage", "secret_value_visible": False}})
                        continue
                    elif cmd_base == "/memory":
                        try:
                            event, result = execute_memory_command(memory_manager, cmd_args)
                            emit({"jsonrpc": "2.0", "method": event, "params": result})
                        except (ValueError, RuntimeError) as error:
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": str(error), "kind": "memory_command_error",
                                "secret_value_visible": False}})
                        continue
                    elif cmd_base == "/marketplace":
                        handle_marketplace(text)
                        continue
                    elif cmd_base == "/set":
                        provider_name, separator, model_name = cmd_args.partition(":")
                        if separator and provider_name.strip() and model_name.strip():
                            os.environ["CCAD_PROVIDER"] = provider_name.strip()
                            os.environ["CCAD_MODEL"] = model_name.strip()
                            init_provider()
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Model set to {provider_name.strip()}:{model_name.strip()}"}})
                        continue
                    elif cmd_base in ["/cc", "/compact"]:
                        handle_compaction_command(context_thread_id)
                        continue
                    elif cmd_base == "/workflow":
                        if cmd_args.startswith("use:"):
                            active_workflow = cmd_args.replace("use:", "").strip()
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Active workflow set to: {active_workflow}"}})
                        elif cmd_args.startswith("chaining phase:"):
                            chaining_phase = cmd_args.replace("chaining phase:", "").strip()
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Workflow chaining phase set to: {chaining_phase}"}})
                        elif cmd_args.startswith("chaining state:"):
                            val = cmd_args.replace("chaining state:", "").strip().lower()
                            chaining_state = (val == "true")
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Workflow chaining state set to: {chaining_state}"}})
                        continue
                    elif cmd_base == "/hooks":
                        hook_name = cmd_args.strip()
                        if hook_name:
                            active_hooks.append(hook_name)
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Hook registered: {hook_name}. Will be triggered during lifecycle."}})
                        continue
                    elif cmd_base == "/schedule":
                        sched_info = cmd_args.strip()
                        if sched_info:
                            schedules.append(sched_info)
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Schedule created: {sched_info}. Background task queued."}})
                        continue
                    elif cmd_base == "/route":
                        active_workflow = "routing_pass"
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Initiating routing workflow pass..."}})
                        session_messages.append(HumanMessage(content=(
                            "Inspect typed project context first. Route only a bounded, validated request "
                            "using catalog tools ccad_ui_route_track and ccad_ui_place_via when available. "
                            "Do not promise complete autorouting or mutate without approval.")))
                        # Fall through to graph execution
                    elif cmd_base == "/drc":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Running DRC checks..."}})
                        # project.drc is a read-only, result-bearing broker
                        # call. Waiting on the exact correlation ID prevents
                        # the chat from claiming an in-progress DRC forever.
                        drc_result = dispatch_client_tool("project.drc", {})
                        try:
                            drc_report = json.loads(drc_result)
                        except (TypeError, json.JSONDecodeError):
                            drc_report = {"error": "invalid_drc_broker_result"}
                        if isinstance(drc_report, dict) and drc_report.get("error"):
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": "DRC did not complete: " + str(drc_report["error"]),
                                "kind": "tool_error", "tool": "project.drc",
                            }})
                        else:
                            error_count = int(drc_report.get("error_count", 0))
                            warning_count = int(drc_report.get("warning_count", 0))
                            diagnostic_count = len(drc_report.get("diagnostics", []))
                            emit({"jsonrpc": "2.0", "method": "message", "params": {
                                "text": (f"DRC complete — {error_count} error(s), "
                                         f"{warning_count} warning(s), "
                                         f"{diagnostic_count} diagnostic(s)."),
                                "kind": "drc_result", "tool": "project.drc",
                                "error_count": error_count, "warning_count": warning_count,
                                "diagnostic_count": diagnostic_count,
                            }})
                        continue
                    elif cmd_base == "/place":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {
                            "text": "Placement workflow unavailable: no typed footprint source and placement transaction are registered. No project action was sent."}})
                        continue
                    elif cmd_base == "/design":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Opening the Component Designer Wizard..."}})
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.open_component_wizard", "args": {}}})
                        continue
                    elif cmd_base == "/explain":
                        active_workflow = "default"
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Explaining the current context..."}})
                        session_messages.append(HumanMessage(content="Explain the current board selection or context in detail. Please provide a concise summary of the active design constraints."))
                        # Fall through to graph execution
                    elif cmd_base == "/clear":
                        session_messages = []
                        context_revisions.pop(context_thread_id, None)
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Chat history and context cleared."}})
                        continue
                    elif cmd_base == "/settings":
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.open_settings", "args": {}}})
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Opening agent settings panel..."}})
                        continue
                    elif cmd_base == "/help":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/context [draft]` (inspect bounded context and memory; no provider call)\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`\n- `/task start|status|end` (manage task-scoped STM)\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`\n- `/route`\n- `/drc`\n- `/place`\n- `/design`\n- `/explain`\n- `/clear`\n- `/settings`"}})
                        continue
                    else:
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Unknown command: {cmd_base}"}})
                        continue

                session_messages = bound_session_history(session_messages)
                session_messages.append(HumanMessage(content=text))
                if "post prompt" in [h.lower() for h in active_hooks]:
                    hooks.trigger_hook("post prompt", emit, text)

                # Keep chat useful and truthful while no provider is configured.
                # Do not enter the graph: it cannot produce an answer and older
                # code could then index an empty message list.
                if llm is None:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": ("Local CCad agent received your request and current "
                                 f"design context ({len(context_str)} chars). "
                                 "Provider execution is unavailable; configure a provider "
                                 "key or use local CCad tools."),
                        "kind": "provider_unavailable",
                        "context_received": bool(context_str),
                    }})
                    continue

                try:
                    final_state = invoke_agent_run({"messages": session_messages, "goal": text,
                                                    "context": context_str, "next_node": "",
                                                    "context_metadata": context_metadata,
                                                    "thread_id": context_thread_id})
                except Exception as error:
                    export_state = telemetry_runtime.flush_turn()
                    emit({"jsonrpc": "2.0", "method": "observability_state",
                          "params": export_state})
                    trace = telemetry_runtime.current_trace()
                    emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
                        "run_state": "failed", **trace, "error_type": type(error).__name__,
                        "prompt_emitted": False, "secret_value_visible": False,
                    }})
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": provider_error_user_message(error),
                        "kind": "provider_error", "error_type": type(error).__name__,
                        "cause": classify_provider_error(error),
                        "http_status": provider_http_status(error),
                        "retry_after_seconds": provider_retry_after_seconds(error),
                    }})
                    continue
                export_state = telemetry_runtime.flush_turn()
                emit({"jsonrpc": "2.0", "method": "observability_state",
                      "params": export_state})
                session_messages = bound_session_history(final_state["messages"])
                last_msg = session_messages[-1]
                has_tool_calls = bool(getattr(last_msg, "tool_calls", None))
                has_legacy_tool = "<TOOL>" in str(getattr(last_msg, "content", ""))
                trace = telemetry_runtime.current_trace()
                emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
                    "run_state": "awaiting_tool_approval" if (has_tool_calls or has_legacy_tool) else "completed",
                    **trace,
                    "token_usage": "unavailable", "cost": "unavailable",
                }})
                
                if hasattr(last_msg, "tool_calls") and last_msg.tool_calls:
                    for tcall in last_msg.tool_calls:
                        tool_name = tcall.get("name", "")
                        args = tcall.get("args", {})
                        if "pre tool call" in [h.lower() for h in active_hooks]:
                            hooks.trigger_hook("pre tool call", emit, tool_name)
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {
                            "tool": tool_name, "args": args,
                            "call_id": tcall.get("id", "") or "agent-tool-call",
                        }})
                        if "post tool call" in [h.lower() for h in active_hooks]:
                            hooks.trigger_hook("post tool call", emit, tool_name)
                elif "<TOOL>" in last_msg.content:
                    tool_call_str = last_msg.content.replace("<TOOL>", "").strip()
                    tool_name = tool_call_str.split(" ")[0]
                    if "pre tool call" in [h.lower() for h in active_hooks]:
                        hooks.trigger_hook("pre tool call", emit, tool_name)
                    args_str = tool_call_str[len(tool_name):].strip()
                    args = {}
                    try:
                        args = json.loads(args_str)
                    except Exception as e:
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error parsing tool args: {e}"}})
                    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": tool_name, "args": args}})
                    if "post tool call" in [h.lower() for h in active_hooks]:
                        hooks.trigger_hook("post tool call", emit, tool_name)
                else:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": last_msg.content}})
                    if "pre exit/end" in [h.lower() for h in active_hooks]:
                        hooks.trigger_hook("pre exit/end", emit)
            elif method in ("agent.langfuse_status", "agent.observability_status"):
                emit({"jsonrpc": "2.0", "method": "observability_state",
                      "params": observability_state()})
            elif method in ("agent.langfuse_set_secret", "agent.set_observability_secret"):
                params = req.get("params", {})
                public_key = params.get("public_key", "")
                secret_key = params.get("secret_key", "")
                if not isinstance(public_key, str) or not isinstance(secret_key, str):
                    emit({"jsonrpc": "2.0", "method": "observability_state", "params": {
                        "configured": False, "enabled": False,
                        "exporter_initialized": False, "backend": "langfuse",
                        "last_test": "not_run", "reason": "invalid_secret_payload",
                        "secret_value_visible": False,
                    }})
                    continue
                observability_secrets["public_key"] = public_key
                observability_secrets["secret_key"] = secret_key
                emit({"jsonrpc": "2.0", "method": "observability_state",
                      "params": reconfigure_observability()})
            elif method in ("agent.langfuse_test", "agent.test_export"):
                state = telemetry_runtime.test_export()
                emit({"jsonrpc": "2.0", "method": "observability_state", "params": state})
                if state.get("last_test") == "flushed":
                    message = "Langfuse test trace flushed. Inspect it in Langfuse before treating export as connected."
                else:
                    message = "Observability test was not exported: " + str(
                        state.get("reason", "not_configured"))
                emit({"jsonrpc": "2.0", "method": "message", "params": {
                    "text": message, "secret_value_visible": False,
                }})
            elif method in ("agent.set_config", "agent.langfuse_set_config"):
                raw_config_data = req.get("params", {})
                if not isinstance(raw_config_data, dict):
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": "Agent configuration was not saved: expected an object.",
                        "kind": "configuration_error", "secret_value_visible": False}})
                    continue
                config_data: dict[str, Any] = raw_config_data
                rejected_secret_keys = []
                secret_key_fragments = ("api_key", "apikey", "secret", "token",
                                        "password", "credential")
                clean_config = sanitize_persisted_config(
                    config_data, secret_key_fragments, rejected_secret_keys)
                persistence_error = None
                for k, v in clean_config.items():
                    try:
                        if k == "memory":
                            config_manager.update_checked(k, v)
                        else:
                            config_manager.update(k, v)
                    except ConfigPersistenceError as error:
                        persistence_error = error.category
                        break
                text = ("Agent configuration saved successfully." if not rejected_secret_keys
                        else "Agent configuration saved; secret fields were rejected.")
                if persistence_error:
                    text = "Agent configuration was not fully saved: " + persistence_error
                emit({"jsonrpc": "2.0", "method": "message", "params": {
                    "text": text, "kind": "configuration_error" if persistence_error else "status",
                    "persisted": not bool(persistence_error),
                    "secret_value_visible": False}})
                if "provider" in clean_config or "model" in clean_config:
                    provider = clean_config.get("provider", "openai")
                    model = clean_config.get("model", "gpt-5.1")
                    os.environ["CCAD_PROVIDER"] = str(provider) if isinstance(provider, str) else "openai"
                    os.environ["CCAD_MODEL"] = str(model) if isinstance(model, str) else "gpt-5.1"
                    init_provider()
                if "observability" in clean_config:
                    emit({"jsonrpc": "2.0", "method": "observability_state",
                          "params": reconfigure_observability()})
                if "memory" in clean_config and not persistence_error:
                    failures = memory_manager.configure(clean_config.get("memory", {}))
                    emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                        "tiers": memory_manager.state(), "storage_errors": failures,
                        "persisted": True, "secret_value_visible": False}})
            elif method == "agent.get_config":
                emit({"jsonrpc": "2.0", "method": "config_state", "params": config_manager.config})
            elif method == "agent.set_tool_catalog":
                try:
                    state = install_native_tool_catalog(req.get("params", {}).get("catalog"))
                except (TypeError, ValueError) as error:
                    state = {"accepted": False, "method_count": 0, "tool_count": 0,
                             "error": str(error), "secret_value_visible": False}
                emit({"jsonrpc": "2.0", "method": "tool_catalog_state", "params": state})
            elif method == "agent.activate_provider":
                params = req.get("params", {})
                provider_id = params.get("provider") or config_manager.get("provider", "openai")
                model = params.get("model") or config_manager.get("model", "")
                os.environ["CCAD_PROVIDER"] = provider_id
                if model:
                    os.environ["CCAD_MODEL"] = model
                provider_ready = init_provider()
                emit({"jsonrpc": "2.0", "method": "provider_activation_result", "params": {
                    "provider": provider_id, "model": model, "initialized": provider_ready,
                    "secret_value_visible": False,
                }})
            elif method == "agent.mcp_status":
                servers = config_manager._normalize_mcp_servers(
                    config_manager.get("mcp_servers", []))
                emit({"jsonrpc": "2.0", "method": "mcp_status", "params": {
                    "servers": servers, "configured": bool(servers),
                    "runtime": "not_started", "process_execution": False}})
            elif method == "agent.mcp_plan":
                servers = config_manager._normalize_mcp_servers(
                    config_manager.get("mcp_servers", []))
                emit({"jsonrpc": "2.0", "method": "mcp_plan", "params": {
                    "servers": servers, "launch_allowed": False,
                    "reason": "MCP process execution requires supervised runtime and explicit approval"}})
            elif method == "agent.generate_component":
                prompt = req.get("params", {}).get("prompt", "")
                ctype = req.get("params", {}).get("type", "footprint")
                pkg = req.get("params", {}).get("package", "DIP")
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"AI Generator: Crafting {ctype} for '{prompt}'..."}})
                
                # Real AI invocation for component generation
                if not llm:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Error: LLM provider not configured for component generation."}})
                    continue
                    
                gen_prompt = f"Design a generic {ctype} component based on '{prompt}' and package '{pkg}'. Reply strictly with a JSON object containing a list of 'pins' (each with 'pin' number, 'name', and 'type'). Do not use markdown blocks."
                try:
                    response = llm.invoke([SystemMessage(content="You are a JSON-only API. No markdown formatting."), HumanMessage(content=gen_prompt)])
                    resp_text = response.content.strip()
                    if resp_text.startswith("```json"):
                        resp_text = resp_text[7:]
                    if resp_text.endswith("```"):
                        resp_text = resp_text[:-3]
                    parsed = json.loads(resp_text)
                    pins = parsed.get("pins", [])
                    if not isinstance(pins, list) or not pins:
                        raise ValueError("provider returned no component pins")
                except Exception as error:
                    # A failed generation must never become plausible-looking,
                    # invented electronic data.  The UI retains the prompt so
                    # the user can correct it or retry after fixing the cause.
                    emit({"jsonrpc": "2.0", "method": "component_generation_failed", "params": {
                        "category": classify_provider_error(error),
                        "message": "Component generation failed; no component was created.",
                    }})
                    continue

                emit({"jsonrpc": "2.0", "method": "generated_component", "params": {"pins": pins, "name": "AI_" + pkg}})
            elif method == "agent.get_marketplace_catalog":
                catalog = get_dynamic_marketplace_catalog()
                emit({"jsonrpc": "2.0", "method": "marketplace_catalog", "params": catalog})
            elif "result" in req:
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Tool executed successfully on C++ side."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: {str(e)}"}})
