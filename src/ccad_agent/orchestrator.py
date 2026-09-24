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
from memory_compaction import (MemoryCompactionError, MemoryCompactionPlans,
                               MEMORY_SUMMARY_SYSTEM_PROMPT)
from context_broker import ContextBroker, extract_context_signals
from history_compaction import (HistoryCompactionError,
                                compact_history,
                                prepare_history_compaction,
                                replace_checkpoint_history)
from context_package import (build_context_package, build_provider_request_report,
                             format_large_context_explanation)
from conversation_store import (ConversationStore, ConversationStoreError,
                                budgeted_history_window)
from model_catalog import (fetch_anthropic_models as _fetch_anthropic_models,
                           fetch_cerebras_models as _fetch_cerebras_models,
                           fetch_gemini_models as _fetch_gemini_models,
                           fetch_ollama_models as _fetch_ollama_models,
                           fetch_openai_models as _fetch_openai_models,
                           fetch_openrouter_models as _fetch_openrouter_models)

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
    return _fetch_openrouter_models(catalog_failure)

def fetch_cerebras_models():
    return _fetch_cerebras_models(catalog_failure)

def fetch_ollama_models():
    return _fetch_ollama_models(catalog_failure)

def fetch_openai_models():
    return _fetch_openai_models(catalog_failure)

def fetch_anthropic_models():
    return _fetch_anthropic_models(catalog_failure)

def fetch_gemini_models():
    return _fetch_gemini_models(catalog_failure)



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
memory_compaction_plans = MemoryCompactionPlans()
try:
    _memory_token_budget = int(os.environ.get("CCAD_AGENT_MEMORY_TOKENS", "1000"))
except ValueError:
    _memory_token_budget = 1000
context_broker = ContextBroker(memory_token_budget=_memory_token_budget)
active_turn_contexts: dict[str, dict] = {}


def invalidate_thread_context(thread_id):
    """Purge cached and currently active context after a scope/write change."""
    thread = str(thread_id or "")
    if thread:
        context_broker.invalidate_thread(thread)
        active_turn_contexts.pop(thread, None)


def project_retrieval_signals(raw_context):
    """Extract only stable selection/editor IDs from the typed context envelope."""
    try:
        decoded = json.loads(raw_context) if isinstance(raw_context, str) else {}
    except (TypeError, json.JSONDecodeError):
        return "", []
    if not isinstance(decoded, dict):
        return "", []
    project = decoded.get("project", decoded)
    if not isinstance(project, dict):
        project = decoded
    editor = ""
    for source in (decoded, project, decoded.get("editor_state", {})):
        if isinstance(source, dict):
            editor = next((str(source[key]) for key in
                           ("active_editor", "editor", "document_kind")
                           if source.get(key)), editor)
        if editor:
            break
    selection = decoded.get("selection", project.get("selection", []))
    selected = []
    if isinstance(selection, dict):
        selection = selection.get("objects", selection.get("items", []))
    if isinstance(selection, list):
        for item in selection[:32]:
            if isinstance(item, (str, int)):
                selected.append(str(item))
            elif isinstance(item, dict):
                for key in ("id", "uuid", "reference", "ref", "refdes", "net", "layer"):
                    if item.get(key):
                        selected.append(str(item[key]))
    return editor[:40], selected[:32]


def recent_retrieval_text(messages):
    """Expose only bounded text blocks to deterministic retrieval signals."""
    result = []
    for message in list(messages)[-8:]:
        content = getattr(message, "content", "")
        if isinstance(content, str):
            result.append(content[:240])
        elif isinstance(content, list):
            result.append(" ".join(block["text"][:240] for block in content
                                   if isinstance(block, dict) and
                                   isinstance(block.get("text"), str))[:240])
    return [item for item in result if item]

def search_memory_context(query: str) -> str:
    """Expose a real, read-only deep retrieval tool to the active Agent turn."""
    thread_id = str(memory_manager.identities["ltm"])
    context = active_turn_contexts.get(thread_id)
    if context is None:
        return json.dumps({"ok": False, "error": "turn_context_unavailable",
                           "results": [], "secret_value_visible": False})
    if len(str(query)) > 1000:
        return json.dumps({"ok": False, "error": "query_too_long", "results": [],
                           "secret_value_visible": False})
    if not any(memory_manager.enabled.values()):
        return json.dumps({"ok": True, "results": [], "reason": "all_tiers_disabled",
                           "secret_value_visible": False})
    with telemetry_runtime.observation("memory.refresh", "retriever", {
            "thread_id_hash": hashlib.sha256(thread_id.encode()).hexdigest()[:16],
            "query_digest": hashlib.sha256(str(query).encode()).hexdigest()[:24],
            "context_version": str(context.get("version", 0)),
        }) as observation:
        refreshed = context_broker.refresh_memory(
            memory_manager, context, query, reason="agent_requested_deeper_memory")
        if observation is not None:
            observation.update(metadata={
                "result_count": str(len(refreshed["memories"])),
                "new_context_version": str(refreshed["version"]),
                "memory_chars": str(refreshed["memory_chars"]),
            })
    active_turn_contexts[thread_id] = refreshed
    results = [{key: entry.get(key, "") for key in
                ("id", "title", "tier", "scope", "content")}
               for entry in refreshed["memories"]]
    return json.dumps({"ok": True, "context_version": refreshed["version"],
                       "change_reason": refreshed["change_reason"],
                       "memory_summary": refreshed["memory_summary"],
                       "memory_manifest": refreshed["manifest"],
                       "retrieval": refreshed["memory_retrieval"],
                       "results": results, "secret_value_visible": False},
                      ensure_ascii=False, sort_keys=True)


def build_memory_search_tool():
    """Build the local deterministic search tool beside C++ broker tools."""
    schema = create_model("CCad_MemorySearchArgs",
                          query=(str, Field(..., min_length=1, max_length=1000,
                                            description="Specific memory detail to retrieve")))
    return StructuredTool.from_function(
        search_memory_context, name="ccad_search_memory",
        description=("Search enabled CCad memory tiers for relevant stored user preferences, "
                     "conversation knowledge, and task facts. Read-only; results are scoped "
                     "to the active user/task/thread and returned with provenance."),
        args_schema=schema)


def handle_durable_memory_compaction(arguments: str, thread_id: str) -> None:
    """Explicit plan -> provider summary -> reviewed atomic apply lifecycle."""
    import shlex

    try:
        tokens = shlex.split(str(arguments))
        if not tokens:
            raise ValueError("Use `/memory compact plan tier:ltm scope:conversation`.")
        action = tokens[0].casefold()
        if action == "plan":
            tokens.pop(0)
            options = {}
            for token in tokens:
                key, separator, value = token.partition(":")
                if not separator or key not in {"tier", "scope"} or not value.strip():
                    raise ValueError("Plan syntax: `/memory compact plan tier:ltm|episodic scope:<scope>`.")
                if key in options:
                    raise ValueError(f"Specify `{key}` only once.")
                options[key] = value.strip()
            tier, scope = options.get("tier", ""), options.get("scope", "")
            if tier not in {"ltm", "episodic"} or not scope:
                raise ValueError("Choose durable tier `ltm` or `episodic` and an exact scope.")
            if not memory_manager.enabled[tier] or memory_manager.storage_errors.get(tier):
                raise ValueError(f"Memory tier `{tier}` must be enabled and available to compact.")
            namespace = memory_manager.identities[tier]
            telemetry_runtime.begin_turn()
            with telemetry_runtime.session(thread_id), telemetry_runtime.observation(
                    "memory.compaction.plan", "agent", {
                        "tier": tier, "scope_present": "true",
                    }):
                plan = memory_compaction_plans.create(
                    memory_manager.list(tier=tier, scope=scope), tier=tier,
                    namespace=namespace, scope=scope)
            emit({"jsonrpc": "2.0", "method": "observability_state",
                  "params": telemetry_runtime.flush_turn()})
            if not plan["ready"]:
                reasons = {
                    "too_few_records": "At least two records in that tier, namespace, and scope are required.",
                    "insufficient_source": "Selected records are too short to benefit from compaction.",
                    "unsafe_or_invalid_source": "A source record is unsafe or invalid; no provider request was sent.",
                    "source_exceeds_compaction_bound": "Source data exceeds the safe compaction bound.",
                    "tier_not_durable": "Only LTM and episodic records can be compacted.",
                }
                raise ValueError(reasons.get(plan["reason"],
                                             "No eligible memory records were found."))
            provider, model_name = active_provider_model()
            plan["provider"] = provider
            plan["model"] = model_name
            request_chars = (len(MEMORY_SUMMARY_SYSTEM_PROMPT) +
                             len(plan["source_json"]) +
                             int(plan["report"]["summary_limit_chars"]))
            estimated_tokens = (request_chars + 3) // 4 + 8
            plan["report"].update(provider=provider, model=model_name,
                                  request_chars=request_chars,
                                  estimated_input_tokens=estimated_tokens)
            emit({"jsonrpc": "2.0", "method": "memory_compaction_state", "params": {
                "stage": "planned", "plan_id": plan["plan_id"],
                "tier": tier, "scope": scope,
                "source_record_count": plan["report"]["source_record_count"],
                "source_chars": plan["report"]["source_chars"],
                "estimated_input_tokens": estimated_tokens,
                "provider": provider, "model": model_name,
                "tools_enabled": False, "expires_in_seconds": 900,
                "source_contents_emitted": False,
                "secret_value_visible": False}})
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "kind": "memory_compaction_plan",
                "text": (f"Prepared compaction for {plan['report']['source_record_count']} "
                         f"{tier} records ({plan['report']['source_chars']} characters; "
                         f"about {estimated_tokens} input tokens with {provider}/{model_name}). "
                         "No provider request or memory change occurred. Review the scope and "
                         f"send only with `/memory compact send:{plan['plan_id']}`. "
                         "That sends the selected memory text to the named provider; its result "
                         "will be shown for review before any records are replaced."),
                "secret_value_visible": False}})
            return

        if len(tokens) != 1 or ":" not in tokens[0]:
            raise ValueError("Use `/memory compact send:<plan-id>`, `apply:<plan-id>`, or `cancel:<plan-id>`." )
        operation, plan_id = tokens[0].split(":", 1)
        operation, plan_id = operation.casefold(), plan_id.strip()
        if not plan_id:
            raise ValueError("A compaction plan ID is required.")
        plan = memory_compaction_plans.get(plan_id)
        if plan["namespace"] != memory_manager.identities.get(plan["tier"]):
            raise MemoryCompactionError("plan_namespace_changed")
        if operation == "cancel":
            telemetry_runtime.begin_turn()
            with telemetry_runtime.session(thread_id), telemetry_runtime.observation(
                    "memory.compaction.cancel", "span", {
                        "tier": plan["tier"], "source_record_count": str(
                            plan["report"]["source_record_count"]),
                    }):
                memory_compaction_plans.cancel(plan_id)
            emit({"jsonrpc": "2.0", "method": "observability_state",
                  "params": telemetry_runtime.flush_turn()})
            emit({"jsonrpc": "2.0", "method": "memory_compaction_state", "params": {
                "stage": "cancelled", "plan_id": plan_id,
                "source_contents_emitted": False, "secret_value_visible": False}})
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": "Memory compaction cancelled; stored records are unchanged.",
                "kind": "memory_compaction_cancelled", "secret_value_visible": False}})
            return
        if operation == "send":
            if plan["status"] != "planned":
                raise MemoryCompactionError("plan_already_summarized")
            if (not memory_manager.enabled[plan["tier"]] or llm is None):
                raise MemoryCompactionError("provider_or_memory_unavailable")
            memory_store.validate_compaction_sources(
                plan["source_entries"], tier=plan["tier"],
                namespace=plan["namespace"], scope=plan["scope"])
            provider, model_name = active_provider_model()
            callbacks = active_callbacks()
            run_config: RunnableConfig = {
                "run_name": "ccad-durable-memory-compaction",
                "tags": ["ccad", "memory-compaction", provider],
                "metadata": {
                    "ccad_operation": "durable_memory_compaction",
                    "ccad_provider": provider, "ccad_model": model_name,
                    "ccad_thread_id_present": "true" if thread_id else "false",
                    "ccad_memory_tier": plan["tier"],
                    "ccad_source_record_count": str(plan["report"]["source_record_count"]),
                    "ccad_source_chars": str(plan["report"]["source_chars"]),
                    "ccad_tools_enabled": "false",
                },
            }
            if callbacks:
                run_config["callbacks"] = callbacks
            telemetry_runtime.begin_turn()
            emit({"jsonrpc": "2.0", "method": "memory_compaction_state", "params": {
                "stage": "request_started", "plan_id": plan_id,
                "provider": provider, "model": model_name,
                "estimated_input_tokens": plan["report"]["estimated_input_tokens"],
                "provider_request_sent": False, "tools_enabled": False,
                "source_contents_emitted": False, "secret_value_visible": False}})
            try:
                with telemetry_runtime.session(thread_id), telemetry_runtime.observation(
                        "memory.compact", "agent", {
                            "provider": provider, "model": model_name,
                            "tier": plan["tier"],
                            "source_record_count": str(plan["report"]["source_record_count"]),
                            "source_chars": str(plan["report"]["source_chars"]),
                        }):
                    with telemetry_runtime.observation(
                            "memory.compact.summary", "generation", {
                                "source_record_count": str(plan["report"]["source_record_count"]),
                                "tools_enabled": "false",
                            }, model=model_name):
                        response = llm.invoke([
                            SystemMessage(content=(MEMORY_SUMMARY_SYSTEM_PROMPT +
                                                  "\nMaximum summary length: " +
                                                  str(plan["max_summary_chars"]) +
                                                  " characters.")),
                            HumanMessage(content=plan["source_json"]),
                        ], config=run_config)
                summary_text = getattr(response, "content", "")
                if not isinstance(summary_text, str):
                    raise MemoryCompactionError("non_text_summary")
                plan = memory_compaction_plans.set_summary(plan_id, summary_text)
                usage = getattr(response, "usage_metadata", None)
                usage_data = ({key: value for key, value in usage.items()
                               if key in {"input_tokens", "output_tokens", "total_tokens"}
                               and isinstance(value, int) and not isinstance(value, bool)}
                              if isinstance(usage, dict) else {})
                emit({"jsonrpc": "2.0", "method": "memory_compaction_state", "params": {
                    "stage": "summary_ready", "plan_id": plan_id,
                    "tier": plan["tier"],
                    "source_record_count": plan["report"]["source_record_count"],
                    "source_chars": plan["report"]["source_chars"],
                    "summary_chars": len(plan["summary"]),
                    "provider": provider, "model": model_name,
                    "provider_usage": usage_data, "provider_request_sent": True,
                    "tools_enabled": False, "persistent_change": False,
                    "secret_value_visible": False}})
                emit({"jsonrpc": "2.0", "method": "observability_state",
                      "params": telemetry_runtime.flush_turn()})
                emit({"jsonrpc": "2.0", "method": "message", "params": {
                    "kind": "memory_compaction_summary",
                    "text": ("Proposed durable-memory summary (not saved):\n\n" +
                             plan["summary"] + "\n\nReview it, then apply only with "
                             f"`/memory compact apply:{plan_id}`. Cancel with "
                             f"`/memory compact cancel:{plan_id}`."),
                    "provider_request_sent": True, "tools_enabled": False,
                    "persistent_change": False, "secret_value_visible": False}})
            except Exception as error:
                export_state = telemetry_runtime.flush_turn()
                emit({"jsonrpc": "2.0", "method": "observability_state",
                      "params": export_state})
                category = (error.category if isinstance(error, MemoryCompactionError)
                            else classify_provider_error(error))
                emit({"jsonrpc": "2.0", "method": "memory_compaction_state", "params": {
                    "stage": "failed", "plan_id": plan_id,
                    "category": category, "provider_request_sent": True,
                    "persistent_change": False, "secret_value_visible": False}})
                emit({"jsonrpc": "2.0", "method": "message", "params": {
                    "text": ("Memory summary was not accepted; original records are unchanged. "
                             + (str(error) if isinstance(error, MemoryCompactionError)
                                else provider_error_user_message(error))),
                    "kind": "memory_compaction_failed", "category": category,
                    "provider_request_sent": True, "persistent_change": False,
                    "secret_value_visible": False}})
            return
        if operation == "apply":
            if plan["status"] != "summarized":
                raise MemoryCompactionError("summary_review_required")
            if not memory_manager.enabled[plan["tier"]]:
                raise MemoryCompactionError("memory_tier_disabled")
            source = plan["source_entries"]
            tags = list(dict.fromkeys(tag for item in source
                                      for tag in item.get("tags", []) if tag))[:32]
            expiries = []
            for item in source:
                value = str(item.get("expires_at", ""))
                if value:
                    try:
                        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
                    except ValueError as error:
                        raise MemoryCompactionError("invalid_source_expiry") from error
                    if parsed.tzinfo is None:
                        raise MemoryCompactionError("invalid_source_expiry")
                    expiries.append((parsed, value))
            expires_at = min(expiries, key=lambda item: item[0])[1] if expiries else ""
            telemetry_runtime.begin_turn()
            with telemetry_runtime.session(thread_id), telemetry_runtime.observation(
                    "memory.compaction.apply", "agent", {
                        "tier": plan["tier"],
                        "source_record_count": str(len(source)),
                        "summary_chars": str(len(plan["summary"])),
                    }):
                entry = memory_manager.apply_compaction(
                    tier=plan["tier"], namespace=plan["namespace"],
                    scope=plan["scope"], source_entries=source,
                    summary=plan["summary"], title=f"Compacted memory ({len(source)} records)",
                    tags=tags, expires_at=expires_at)
            invalidate_thread_context(memory_manager.identities["ltm"])
            memory_compaction_plans.cancel(plan_id)
            emit({"jsonrpc": "2.0", "method": "observability_state",
                  "params": telemetry_runtime.flush_turn()})
            emit({"jsonrpc": "2.0", "method": "memory_compaction_state", "params": {
                "stage": "applied", "plan_id": plan_id,
                "tier": plan["tier"], "replaced_record_count": len(source),
                "summary_record_id": entry["id"], "persistent_change": True,
                "secret_value_visible": False}})
            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {
                "tier": plan["tier"], **memory_manager.state(plan["tier"]),
                "secret_value_visible": False}})
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": (f"Applied reviewed summary. Replaced {len(source)} unchanged "
                         f"{plan['tier']} records with one durable record; runtime cache "
                         "reloaded. The operation was atomic."),
                "kind": "memory_compaction_applied", "persistent_change": True,
                "secret_value_visible": False}})
            return
        raise ValueError("Use `plan`, `send:<plan-id>`, `apply:<plan-id>`, or `cancel:<plan-id>`. ")
    except (MemoryCompactionError, ValueError, RuntimeError, MemoryStoreError) as error:
        category = getattr(error, "category", "memory_compaction_invalid_request")
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": f"Memory compaction not performed ({category}): {error}",
            "kind": "memory_compaction_refused", "category": category,
            "provider_request_sent": False, "persistent_change": False,
            "secret_value_visible": False}})


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
    if "ccad_search_memory" not in {tool.name for tool in agent_tools}:
        agent_tools.append(build_memory_search_tool())
    if llm is not None:
        router_llm = bind_native_tools(llm)
        librarian_llm = router_llm
    execute_tool_node = ToolNode(agent_tools)
    if executor is not None:
        executor = create_orchestrator()
    return {"accepted": True, "method_count": len(native_tool_catalog),
            "tool_count": len(agent_tools), "native_tool_count": len(native_tool_catalog),
            "local_tool_count": len(agent_tools) - len(native_tool_catalog),
            "secret_value_visible": False}

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
            # Google uses RESOURCE_EXHAUSTED for both rate limits and quota
            # exhaustion. Do not claim billing/quota failure without evidence.
            if any(marker in text for marker in (
                    "rate limit", "too many requests", "requests per minute")):
                return "rate_limited"
            if any(marker in text for marker in (
                    "exceeded your current quota", "quota exhausted",
                    "quota exceeded", "daily quota", "billing quota")):
                return "quota_exhausted"
            return "quota_or_rate_limit"
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
        # A bare ResourceExhausted signal is ambiguous and handled separately.
        if any(marker in text or marker in error_name for marker in (
                "exceeded your current quota",
                "quota exhausted", "quota exceeded")):
            return "quota_exhausted"
        if any(marker in text or marker in error_name for marker in (
                "resourceexhausted", "resource exhausted")):
            return "quota_or_rate_limit"
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
        "quota_or_rate_limit": "The provider returned a limit error that does not distinguish quota from request rate. Check provider usage/quota and any retry-after value; no automatic retry was sent.",
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
             "Read the typed project context and project_retrieval matches before design-specific work. Retrieved positions and IDs come from the active typed model. Schematic pin membership is an authoritative netlist assignment; shared PCB net IDs are not proof of geometric copper continuity. Use project.state for complete live PCB/schematic state before changes or when requested details are not present.",
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
            "quota_exhausted", "quota_or_rate_limit", "rate_limited", "authentication",
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
conversation_store = ConversationStore()
active_conversation_thread_id = ""
active_conversation_session_id = ""
active_conversation_project_id = ""
active_workflow = "default"
chaining_phase = "none"
chaining_state = True
active_hooks = []
schedules = []
context_revisions = {}
CONTEXT_REVISION_THREAD_LIMIT = 128

def bound_session_history(messages):
    """Keep newest whole user-turn groups within explicit token/message bounds."""
    try:
        limit = int(os.environ.get("CCAD_AGENT_HISTORY_LIMIT", "24"))
    except ValueError:
        limit = 24
    limit = min(64, max(4, limit))
    try:
        token_budget = int(os.environ.get("CCAD_AGENT_HISTORY_TOKENS", "8192"))
    except ValueError:
        token_budget = 8192
    token_budget = min(32768, max(256, token_budget))
    return budgeted_history_window(messages, token_budget=token_budget,
                                   max_messages=limit)


def activate_conversation(thread_id: str, session_id: str = "",
                          project_id: str = "") -> None:
    """Load one thread's canonical model projection; never mix thread caches."""
    global session_messages, active_conversation_thread_id
    global active_conversation_session_id, active_conversation_project_id
    thread_id = str(thread_id).strip()
    if not thread_id:
        raise ValueError("conversation_thread_id_required")
    if thread_id != active_conversation_thread_id:
        loaded = conversation_store.load_model_messages(thread_id)
        session_messages = bound_session_history(loaded)
        active_conversation_thread_id = thread_id
    active_conversation_session_id = str(session_id or thread_id)
    active_conversation_project_id = str(project_id or "project")


def persist_turn_messages(thread_id: str, turn_id: str, messages: Iterable[Any],
                          session_id: str, project_id: str) -> int:
    return conversation_store.append_messages(
        thread_id, messages, turn_id=turn_id, session_id=session_id,
        project_id=project_id)


def persist_resumed_turn(thread_id: str, resumed: Any, *, terminal: bool) -> tuple[str, str]:
    """Persist authoritative checkpoint output and index it only when terminal."""
    if not isinstance(resumed, dict) or not isinstance(resumed.get("messages"), list):
        return "", ""
    messages = resumed["messages"]
    request = next((item for item in reversed(messages)
                    if getattr(item, "type", "") == "human"), None)
    if request is None or not getattr(request, "id", None):
        return "", ""
    turn_id = conversation_store.turn_id_for_message(thread_id, request.id)
    if not turn_id:
        return "", ""
    session_id = (active_conversation_session_id
                  if active_conversation_thread_id == thread_id else thread_id)
    project_id = (active_conversation_project_id
                  if active_conversation_thread_id == thread_id else "project")
    persist_turn_messages(thread_id, turn_id, messages, session_id, project_id)
    last = messages[-1] if messages else None
    if terminal and last is not None and not getattr(last, "tool_calls", None):
        conversation_store.record_turn(
            thread_id, turn_id, str(request.content), messages,
            outcome="completed", project_id=project_id)
        return str(getattr(last, "content", "") or ""), turn_id
    return "", turn_id

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
        try:
            conversation_store.compact_projection(thread_id, compacted)
        except (ConversationStoreError, ValueError) as error:
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": "Conversation compaction was not saved; the canonical transcript remains unchanged.",
                "kind": "conversation_projection_failed",
                "category": getattr(error, "category", "conversation_projection_write_failed"),
                "secret_value_visible": False}})
            return
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
        connection_retry_after = None
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
                connection_retry_after = provider_retry_after_seconds(error)
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
                "retry_after_seconds": connection_retry_after,
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
            previous_thread = str(memory_manager.identities["ltm"])
            if previous_thread != thread_id:
                invalidate_thread_context(previous_thread)
            os.environ["CCAD_AGENT_THREAD_ID"] = thread_id
            session_id = str(params.get("session_id") or thread_id)
            project_id = str(params.get("project_id") or "project")
            try:
                activate_conversation(thread_id, session_id, project_id)
            except (ConversationStoreError, ValueError) as error:
                emit({"jsonrpc": "2.0", "method": "conversation_state", "params": {
                    "thread_id": thread_id,
                    "available": False,
                    "error": getattr(error, "category", "conversation_history_unavailable"),
                    "secret_value_visible": False}})
                return True
            task_id = memory_task_scopes.current(session_id)
            memory_manager.set_identities(
                task_id=task_id or uuid.uuid4().hex,
                thread_id=thread_id,
                project_id=project_id,
                retain_stm_task=bool(task_id))
            memory_manager.configure(config_manager.get("memory", {}))
            memory_compaction_plans.retain_current(
                memory_manager.identities, memory_manager.enabled)
        else:
            os.environ.pop("CCAD_AGENT_THREAD_ID", None)
        emit({"jsonrpc": "2.0", "method": "thread_state", "params": {
            "configured": bool(thread_id), "memory_tiers": memory_manager.state(),
            "conversation_loaded": bool(thread_id),
            "conversation_message_count": len(session_messages),
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
        invalidate_thread_context(memory_manager.identities["ltm"])
        memory_compaction_plans.retain_current(
            memory_manager.identities, memory_manager.enabled)
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
            invalidate_thread_context(memory_manager.identities["ltm"])
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
            if method != "agent.memory_list":
                invalidate_thread_context(memory_manager.identities["ltm"])
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

def handle_human_message(req):
    global active_workflow, chaining_phase, chaining_state, session_messages

    params = req.get("params", {})
    if not isinstance(params, dict):
        params = {}
    text = params.get("text", "")
    raw_context = params.get("context", "")
    requested_thread = str(params.get("thread_id") or
                           os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local"))
    telemetry_runtime.start_agent_turn(requested_thread, {
        "thread_id_hash": hashlib.sha256(
            requested_thread.encode()).hexdigest()[:16],
        "workflow": active_workflow,
        "provider_ready": str(llm is not None).lower(),
    })
    requested_session = str(params.get("session_id") or requested_thread)
    requested_task = (memory_task_scopes.current(requested_session)
                      or uuid.uuid4().hex)
    task_is_active = memory_task_scopes.is_active(requested_task)
    os.environ["CCAD_AGENT_THREAD_ID"] = requested_thread
    project_id = str(params.get("project_id") or
                     config_manager.get("project_name", "project"))
    try:
        activate_conversation(requested_thread, requested_session, project_id)
    except (ConversationStoreError, ValueError) as error:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": "This conversation could not be loaded safely; no provider request was sent.",
            "kind": "conversation_history_unavailable",
            "category": getattr(error, "category", "conversation_history_unavailable"),
            "provider_request_sent": False, "secret_value_visible": False}})
        return
    previous_thread = str(memory_manager.identities["ltm"])
    memory_manager.set_identities(task_id=requested_task,
                                  thread_id=requested_thread,
                                  project_id=project_id,
                                  retain_stm_task=task_is_active)
    if previous_thread != requested_thread:
        invalidate_thread_context(previous_thread)
    memory_manager.configure(config_manager.get("memory", {}))
    memory_compaction_plans.retain_current(
        memory_manager.identities, memory_manager.enabled)
    if not isinstance(raw_context, str):
        raw_context = str(raw_context or "")
    memory_query = text
    if text.partition(" ")[0].casefold() == "/context":
        memory_query = text.partition(" ")[2].strip()
        if memory_query.casefold().startswith("preview "):
            memory_query = memory_query[8:].strip()
    memory_runtime = memory_manager.state()
    try:
        turn_records = conversation_store.search_turn_records(
            requested_thread, memory_query, limit=5)
        recap = conversation_store.thread_recap(requested_thread)
    except ConversationStoreError as error:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": "Historical conversation retrieval is unavailable; no unverified history was added to context.",
            "kind": "conversation_retrieval_unavailable",
            "category": error.category, "secret_value_visible": False}})
        turn_records = []
        recap = {"source_turn_ids": [], "turns": []}
    active_editor, selected_objects = project_retrieval_signals(raw_context)
    try:
        project_context = json.loads(raw_context) if raw_context else {}
    except (TypeError, json.JSONDecodeError):
        project_context = {}
    project_context = project_context if isinstance(project_context, dict) else {}
    recent_context = recent_retrieval_text(session_messages)
    signals = extract_context_signals(
        memory_query, goal=text, project_id=project_id,
        active_editor=active_editor, selected_objects=selected_objects,
        workflow=active_workflow,
        task=memory_manager.identities["stm"], recent_turns=turn_records,
        recent_context=recent_context)
    with telemetry_runtime.session(requested_thread), telemetry_runtime.observation(
            "context.assemble", "chain", {
                "thread_id_hash": hashlib.sha256(
                    requested_thread.encode()).hexdigest()[:16],
                "project_revision": context_revision(raw_context),
                "signal_digest": signals["digest"],
                "recent_message_count": str(len(session_messages)),
                "historical_turn_count": str(len(turn_records)),
            }) as assembly_observation:
        with telemetry_runtime.observation("memory.retrieve", "retriever", {
                "signal_digest": signals["digest"],
                "enabled_tier_count": str(sum(memory_manager.enabled.values())),
                "retrieval_mode": "deterministic_lexical",
            }) as retrieval_observation:
            turn_context = context_broker.prepare(
                memory_manager, thread_id=requested_thread,
                project_revision=context_revision(raw_context),
                user_request=memory_query, goal=text,
                project_id=project_id, active_editor=active_editor,
                selected_objects=selected_objects, workflow=active_workflow,
                task=memory_manager.identities["stm"], recent_turns=turn_records,
                historical_turn_count=len(turn_records), signals=signals,
                recent_context=recent_context, thread_recap=recap,
                project_snapshot=project_context,
                active_layer=project_context.get("active_pcb_layer_id", ""),
                active_net=project_context.get("active_pcb_net_id", ""))
            if retrieval_observation is not None:
                retrieval_observation.update(
                    input={"signal_digest": signals["digest"],
                           "enabled_tier_count": str(
                               sum(memory_manager.enabled.values()))},
                    output={"result_count": str(
                                len(turn_context["memories"])),
                            "context_version": str(
                                turn_context["version"])},
                    metadata={
                    "cache_hit": str(turn_context["cache_hit"]).lower(),
                    "memory_chars": str(turn_context["memory_chars"]),
                })
        with telemetry_runtime.observation("project.retrieve", "retriever", {
                "signal_digest": signals["digest"],
                "project_revision": context_revision(raw_context),
            }) as project_observation:
            retrieval = turn_context["project_retrieval"]
            if project_observation is not None:
                stats = retrieval.get("stats", {})
                project_observation.update(
                    output={"result_count": str(len(retrieval.get("entities", []))),
                            "index_state": str(stats.get("index_state", "unavailable")),
                            "revision": str(retrieval.get("revision", ""))},
                    metadata={"entity_count": str(stats.get("total_entities", 0)),
                              "omitted_count": str(stats.get("omitted_count", 0)),
                              "characters": str(retrieval.get("characters", 0)),
                              "search_method": str(retrieval.get("search_method", "none")),
                              "content_exported": "false"})
        memory_entries = turn_context["memories"]
        memory_retrieval = turn_context["memory_retrieval"]
        active_turn_contexts[requested_thread] = turn_context
        while len(active_turn_contexts) > ContextBroker.MAX_CACHE_ENTRIES:
            active_turn_contexts.pop(next(iter(active_turn_contexts)))
        with telemetry_runtime.observation("context.package", "span", {
                "context_version": str(turn_context["version"]),
                "memory_count": str(len(memory_entries)),
                "memory_chars": str(turn_context["memory_chars"]),
            }) as package_observation:
            package = build_context_package(
                raw_context, memory_entries,
                bound_session_history(session_messages),
                char_limit=agent_context_limit(),
                memory_retrieval=memory_retrieval,
                memory_runtime=memory_runtime,
                turn_records=turn_records,
                thread_recap=recap,
                memory_summary=turn_context["memory_summary"],
                memory_manifest=turn_context["manifest"],
                project_retrieval=turn_context["project_retrieval"],
                turn_context={key: turn_context[key] for key in
                              ("version", "change_reason", "signal_digest")})
            if package_observation is not None:
                package_observation.update(
                    input={"project_revision": context_revision(raw_context),
                           "signal_digest": signals["digest"]},
                    output={"context_package_digest": package["metadata"][
                                "package_digest"],
                            "context_chars": str(package["metadata"][
                                "content_size"]),
                            "truncated": str(package["metadata"][
                                "truncated"]).lower()})
        if assembly_observation is not None:
            context_metadata = package["metadata"]
            assembly_observation.update(
                input={"signal_digest": signals["digest"],
                       "project_revision": context_revision(raw_context)},
                output={"context_package_digest": context_metadata[
                            "package_digest"],
                        "memory_count": str(len(memory_entries)),
                        "context_chars": str(context_metadata[
                            "content_size"])},
                metadata={
                "context_package_digest": context_metadata["package_digest"],
                "context_chars": str(context_metadata["content_size"]),
                "context_tokens_estimated": str(
                    context_metadata["estimated_token_count"]),
                "truncated": str(context_metadata["truncated"]).lower(),
                "source_count": str(len(context_metadata["sources"])),
            })
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
        return
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
        "historical_turn_count": context_metadata["prior_turn_count"],
        "omitted_historical_turn_count": context_metadata[
            "omitted_prior_turn_count"],
        "thread_recap_turn_count": context_metadata[
            "thread_recap_turn_count"],
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
        "project_retrieval_count": context_metadata["project_retrieval_count"],
        "project_retrieval_schematic_pin_count": context_metadata.get(
            "project_retrieval_kinds", {}).get("schematic_pin", 0),
        "project_retrieval_schematic_symbol_count": context_metadata.get(
            "project_retrieval_kinds", {}).get("schematic_symbol", 0),
        "project_retrieval_board_net_count": context_metadata.get(
            "project_retrieval_kinds", {}).get("board_net", 0),
        "project_retrieval_layer_count": context_metadata.get(
            "project_retrieval_layer_count", 0),
        "project_retrieval_chars": context_metadata["project_retrieval_chars"],
        "project_retrieval_revision": context_metadata["project_retrieval_revision"],
        "project_retrieval_method": context_metadata["project_retrieval_method"],
        "project_retrieval_stats": context_metadata["project_retrieval_stats"],
        "omitted_memory_entry_count": context_metadata["omitted_memory_entry_count"],
        "memory_tier_counts": context_metadata["memory_tier_counts"],
        "memory_tier_chars": context_metadata["memory_tier_chars"],
        "memory_retrieval": context_metadata["memory_retrieval"],
        "memory_runtime": context_metadata["memory_runtime"],
        "memory_summary_chars": context_metadata["memory_summary_chars"],
        "memory_manifest": context_metadata["memory_manifest"],
        "turn_context_version": context_metadata["turn_context_version"],
        "turn_context_change_reason": context_metadata[
            "turn_context_change_reason"],
        "turn_context_signal_digest": context_metadata[
            "turn_context_signal_digest"],
        "context_cache_hit": turn_context["cache_hit"],
        "memory_token_budget": turn_context["memory_token_budget"],
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
            return
        if cmd_base == "/commands":
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/context [draft]` (inspect bounded context and memory; no provider call)\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`\n- `/task start|status|end` (manage task-scoped STM)\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`"}})
            return
        elif cmd_base == "/task":
            task_action = cmd_args.strip().casefold()
            if task_action == "start":
                task_id, cleared = memory_task_scopes.start(requested_session)
                memory_manager.set_identities(
                    task_id=task_id, thread_id=requested_thread,
                    project_id=project_id, retain_stm_task=True)
                invalidate_thread_context(requested_thread)
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
                invalidate_thread_context(requested_thread)
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
            return
        elif cmd_base == "/memory":
            if cmd_args.strip().casefold().startswith("compact"):
                handle_durable_memory_compaction(
                    cmd_args.strip()[len("compact"):], context_thread_id)
            else:
                try:
                    event, result = execute_memory_command(memory_manager, cmd_args)
                    if event in {"memory_added", "memory_updated",
                                 "memory_deleted", "memory_reset"}:
                        invalidate_thread_context(requested_thread)
                    emit({"jsonrpc": "2.0", "method": event, "params": result})
                except (ValueError, RuntimeError) as error:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": str(error), "kind": "memory_command_error",
                        "secret_value_visible": False}})
            return
        elif cmd_base == "/marketplace":
            handle_marketplace(text)
            return
        elif cmd_base == "/set":
            provider_name, separator, model_name = cmd_args.partition(":")
            if separator and provider_name.strip() and model_name.strip():
                os.environ["CCAD_PROVIDER"] = provider_name.strip()
                os.environ["CCAD_MODEL"] = model_name.strip()
                init_provider()
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Model set to {provider_name.strip()}:{model_name.strip()}"}})
            return
        elif cmd_base in ["/cc", "/compact"]:
            handle_compaction_command(context_thread_id)
            return
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
            return
        elif cmd_base == "/hooks":
            hook_name = cmd_args.strip()
            if hook_name:
                active_hooks.append(hook_name)
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Hook registered: {hook_name}. Will be triggered during lifecycle."}})
            return
        elif cmd_base == "/schedule":
            sched_info = cmd_args.strip()
            if sched_info:
                schedules.append(sched_info)
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Schedule created: {sched_info}. Background task queued."}})
            return
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
            return
        elif cmd_base == "/place":
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": "Placement workflow unavailable: no typed footprint source and placement transaction are registered. No project action was sent."}})
            return
        elif cmd_base == "/design":
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Opening the Component Designer Wizard..."}})
            emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.open_component_wizard", "args": {}}})
            return
        elif cmd_base == "/explain":
            active_workflow = "default"
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Explaining the current context..."}})
            session_messages.append(HumanMessage(content="Explain the current board selection or context in detail. Please provide a concise summary of the active design constraints."))
            # Fall through to graph execution
        elif cmd_base == "/clear":
            try:
                conversation_store.clear_model_projection(context_thread_id)
            except (ConversationStoreError, ValueError) as error:
                emit({"jsonrpc": "2.0", "method": "message", "params": {
                    "text": "Active conversation context was not cleared because its projection could not be saved.",
                    "kind": "conversation_projection_failed",
                    "category": getattr(error, "category", "conversation_projection_write_failed"),
                    "secret_value_visible": False}})
                return
            session_messages = []
            context_revisions.pop(context_thread_id, None)
            emit({"jsonrpc": "2.0", "method": "message", "params": {
                "text": "Active model context cleared. The durable transcript remains available in conversation history.",
                "kind": "conversation_context_cleared"}})
            return
        elif cmd_base == "/settings":
            emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.open_settings", "args": {}}})
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Opening agent settings panel..."}})
            return
        elif cmd_base == "/help":
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/context [draft]` (inspect bounded context and memory; no provider call)\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`\n- `/task start|status|end` (manage task-scoped STM)\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`\n- `/route`\n- `/drc`\n- `/place`\n- `/design`\n- `/explain`\n- `/clear`\n- `/settings`"}})
            return
        else:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Unknown command: {cmd_base}"}})
            return

    turn_id = uuid.uuid4().hex
    user_message = HumanMessage(content=text)
    try:
        next_history = bound_session_history([*session_messages, user_message])
    except ValueError:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": "This request exceeds the configured recent-history budget; no provider request was sent.",
            "kind": "conversation_history_budget_exceeded",
            "provider_request_sent": False, "secret_value_visible": False}})
        return
    try:
        persist_turn_messages(requested_thread, turn_id, [user_message],
                              requested_session, project_id)
    except (ConversationStoreError, ValueError) as error:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": "Conversation could not be saved safely; no provider request was sent.",
            "kind": "conversation_store_unavailable",
            "category": getattr(error, "category", "conversation_store_write_failed"),
            "provider_request_sent": False, "secret_value_visible": False}})
        return
    session_messages = next_history
    if "post prompt" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post prompt", emit, text)

    # Keep chat useful and truthful while no provider is configured.
    # Do not enter the graph: it cannot produce an answer and older
    # code could then index an empty message list.
    if llm is None:
        unavailable_text = (
            "Local CCad agent received your request and current "
            f"design context ({len(context_str)} chars). "
            "Provider execution is unavailable; configure a provider "
            "key or use local CCad tools.")
        unavailable_message = AIMessage(content=unavailable_text)
        session_messages = bound_session_history(
            [*session_messages, unavailable_message])
        try:
            persist_turn_messages(requested_thread, turn_id,
                                  [unavailable_message],
                                  requested_session, project_id)
            conversation_store.record_turn(
                requested_thread, turn_id, text,
                [user_message, unavailable_message],
                outcome="provider_unavailable", project_id=project_id,
                project_revision_before=context_metadata.get(
                    "project_revision", ""))
        except (ConversationStoreError, ValueError):
            pass
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": unavailable_text,
            "kind": "provider_unavailable",
            "context_received": bool(context_str),
        }})
        return

    try:
        final_state = invoke_agent_run({"messages": session_messages, "goal": text,
                                        "context": context_str, "next_node": "",
                                        "context_metadata": context_metadata,
                                        "thread_id": context_thread_id})
    except Exception as error:
        failure_text = provider_error_user_message(error)
        failure_message = AIMessage(content=failure_text)
        session_messages = bound_session_history(
            [*session_messages, failure_message])
        try:
            persist_turn_messages(requested_thread, turn_id,
                                  [failure_message], requested_session,
                                  project_id)
            conversation_store.record_turn(
                requested_thread, turn_id, text,
                [user_message, failure_message], outcome="provider_error",
                project_id=project_id,
                project_revision_before=context_metadata.get(
                    "project_revision", ""))
        except (ConversationStoreError, ValueError):
            pass
        export_state = telemetry_runtime.flush_turn()
        emit({"jsonrpc": "2.0", "method": "observability_state",
              "params": export_state})
        trace = telemetry_runtime.current_trace()
        emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
            "run_state": "failed", **trace, "error_type": type(error).__name__,
            "prompt_emitted": False, "secret_value_visible": False,
        }})
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": failure_text,
            "kind": "provider_error", "error_type": type(error).__name__,
            "cause": classify_provider_error(error),
            "http_status": provider_http_status(error),
            "retry_after_seconds": provider_retry_after_seconds(error),
        }})
        return
    export_state = telemetry_runtime.flush_turn()
    emit({"jsonrpc": "2.0", "method": "observability_state",
          "params": export_state})
    session_messages = bound_session_history(final_state["messages"])
    last_msg = session_messages[-1]
    has_tool_calls = bool(getattr(last_msg, "tool_calls", None))
    has_legacy_tool = "<TOOL>" in str(getattr(last_msg, "content", ""))
    try:
        persist_turn_messages(requested_thread, turn_id,
                              final_state.get("messages", []),
                              requested_session, project_id)
        if not (has_tool_calls or has_legacy_tool):
            conversation_store.record_turn(
                requested_thread, turn_id, text,
                final_state.get("messages", []), outcome="completed",
                project_id=project_id,
                project_revision_before=context_metadata.get("project_revision", ""))
    except (ConversationStoreError, ValueError) as error:
        emit({"jsonrpc": "2.0", "method": "message", "params": {
            "text": "The provider turn completed, but its transcript update could not be persisted.",
            "kind": "conversation_store_write_failed",
            "category": getattr(error, "category", "conversation_store_write_failed"),
            "secret_value_visible": False}})
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
        if telemetry_runtime.finish_agent_turn():
            emit({"jsonrpc": "2.0", "method": "observability_state",
                  "params": telemetry_runtime.flush_turn()})
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
                    resumed_snapshot = executor.get_state(
                        {"configurable": {"thread_id": thread_id}})
                    try:
                        assistant_text, resumed_turn_id = persist_resumed_turn(
                            thread_id, resumed, terminal=not bool(resumed_snapshot.next))
                    except (ConversationStoreError, ValueError) as store_error:
                        assistant_text, resumed_turn_id = "", ""
                        emit({"jsonrpc": "2.0", "method": "message", "params": {
                            "text": "The approved tool result was received, but the conversation update could not be saved.",
                            "kind": "conversation_store_write_failed",
                            "category": getattr(store_error, "category",
                                                "conversation_store_write_failed"),
                            "secret_value_visible": False}})
                    if assistant_text:
                        emit({"jsonrpc": "2.0", "method": "message", "params": {
                            "text": assistant_text, "kind": "assistant_response",
                            "turn_id": resumed_turn_id, "secret_value_visible": False}})
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
                handle_human_message(req)
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
                    invalidate_thread_context(memory_manager.identities["ltm"])
                    memory_compaction_plans.retain_current(
                        memory_manager.identities, memory_manager.enabled)
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
