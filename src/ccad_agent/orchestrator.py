import sys
import json
import operator
import os
import re
import queue
import threading
import time
import uuid
import atexit
import hashlib
import urllib.request
import urllib.error
from typing import Annotated, TypedDict, List
from langgraph.graph import StateGraph, END
from langgraph.types import Command, interrupt
from langchain_core.tools import tool
from langchain_core.messages import AIMessage, BaseMessage, HumanMessage, SystemMessage

from config import AgentConfigManager
from telemetry import trace_function, tracer
import hooks
from memory_store import MemoryStore

def emit(payload: dict):
    print(json.dumps(payload), flush=True)

def fetch_openrouter_models():
    """Explicit, bounded OpenRouter catalog refresh; never called at startup."""
    key = os.environ.get("OPENROUTER_API_KEY", "")
    if not key:
        return {"ok": False, "error": "missing_api_key", "models": []}
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
            return {"ok": False, "error": "invalid_catalog_shape", "models": [], "network_access": "explicit_refresh"}
        models = []
        for item in payload.get("data", []):
            if not isinstance(item, dict) or not item.get("id"):
                continue
            models.append({
                "id": item["id"], "name": item.get("name", item["id"]),
                "context_length": item.get("context_length"),
                "architecture": item.get("architecture", {}),
            })
        return {"ok": True, "models": models, "count": len(models), "network_access": "explicit_refresh"}
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as error:
        return {"ok": False, "error": type(error).__name__, "models": [], "network_access": "explicit_refresh"}

def cerebras_model_snapshot():
    """Return current documented public presets without a network call."""
    models = [
        {"id": "gpt-oss-120b", "display_name": "OpenAI GPT OSS 120B", "tier": "production"},
        {"id": "llama3.1-8b", "display_name": "Llama 3.1 8B", "tier": "production"},
        {"id": "zai-glm-4.7", "display_name": "Z.ai GLM 4.7", "tier": "preview"},
    ]
    return {"ok": True, "models": models, "count": len(models),
            "network_access": "none", "source": "official_curated_snapshot"}

def context_revision(context: str) -> str:
    """Return stable opaque context identity; never expose context contents."""
    return hashlib.sha256(context.encode("utf-8")).hexdigest()[:16]

broker_wait_enabled = False
current_run_trace_id = ""
current_run_span_id = ""
inbound_queue = None
deferred_queue = queue.Queue()
pending_calls = {}
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
    with pending_calls_lock:
        pending_calls[call_id] = result_queue
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
    with pending_calls_lock:
        process_calls = sorted(pending_calls.keys())
    checkpoint_calls = []
    if checkpoint_saver is not None and executor is not None:
        try:
            snapshot = executor.get_state({"configurable": {
                "thread_id": thread_id or os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local")
            }})
            for task in snapshot.tasks:
                for pending_interrupt in getattr(task, "interrupts", ()):
                    value = getattr(pending_interrupt, "value", {})
                    if isinstance(value, dict) and value.get("call_id"):
                        checkpoint_calls.append(str(value["call_id"]))
        except Exception:
            checkpoint_calls = []
    return {
        "process_call_ids": process_calls,
        "checkpoint_call_ids": sorted(set(checkpoint_calls)),
        "count": len(process_calls) + len(set(checkpoint_calls)),
        "secret_value_visible": False,
    }

def orchestrator_method_catalog():
    """Describe only the child-process JSON-RPC controls and their safety."""
    return {
        "schema_version": 1,
        "methods": [
            {"name": "agent.methods", "read_only": True},
            {"name": "agent.pending_calls", "read_only": True, "secrets": False},
            {"name": "agent.list_models", "read_only": True,
             "network_access": "provider_specific",
             "network_access_by_provider": {"openrouter": "explicit_refresh", "cerebras": "none"},
             "providers": ["openrouter", "cerebras"],
             "params": {"provider": {"type": "string", "enum": ["openrouter", "cerebras"],
                                        "default": "openrouter"}},
             "response": {"method": "provider_models", "fields": [
                 "provider", "ok", "error", "error_detail", "models",
                 "count", "network_access"]}},
            {"name": "agent.cancel_tool", "read_only": False,
             "approval_required": False, "side_effect": "cancel_wait_only"},
            {"name": "tool_result", "read_only": False,
             "approval_required": True, "side_effect": "client_authorized_result"},
            {"name": "human_message", "read_only": False, "provider_call": True,
             "preflight": "intake_guard",
             "responses": ["intake_state", "context_state", "message", "tool_call"],
             "secret_value_visible": False},
        ],
        "secret_value_visible": False,
    }

def dispatch_checkpointed_tool(tool_name: str, args: dict):
    """Pause graph until C++ client returns authoritative tool result."""
    call_id = checkpoint_tool_call_id(tool_name, args)
    decision = interrupt({"kind": "ccad_tool_call", "tool": tool_name,
                          "args": args, "call_id": call_id})
    if isinstance(decision, dict) and "error" in decision:
        return json.dumps(decision)
    return json.dumps(decision) if isinstance(decision, (dict, list)) else str(decision)

def dispatch_client_tool(tool_name: str, args: dict, *, await_result: bool = False) -> str:
    """Send one client tool call and optionally await its authoritative result."""
    call_id = (checkpoint_tool_call_id(tool_name, args)
               if checkpoint_saver is not None and broker_wait_enabled
               else new_tool_call_id(tool_name))
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {
        "tool": tool_name, "args": args, "call_id": call_id,
    }})
    if broker_wait_enabled and await_result:
        emit_tool_approval_state()
        if checkpoint_saver is not None:
            return dispatch_checkpointed_tool(tool_name, args)
        return wait_for_broker_result(call_id)
    return "Action dispatched to CCad client."

config_manager = AgentConfigManager()
memory_store = MemoryStore()

def local_memory_context():
    """Return bounded project memories for explicit short-term context use."""
    memory_config = config_manager.get("memory", {})
    if not memory_config.get("stm", True):
        return ""
    entries = memory_store.list(scope="project")[-8:]
    if not entries:
        return ""
    lines = ["[CCAD pinned local memory]"]
    for entry in entries:
        lines.append(f"- [{entry.get('id', 'unknown')}] {entry.get('title') or 'memory'}: {entry.get('content', '')[:1000]}")
    return "\n".join(lines)

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
    next_node: str

@tool
def ui_place_via(x_mm: float, y_mm: float, dry_run: bool = False):
    """Places a via on the PCB at the specified x, y coordinates (in mm)."""
    args = {"x_mm": x_mm, "y_mm": y_mm, "dry_run": dry_run}
    call_id = (checkpoint_tool_call_id("ui.place_via", args)
               if checkpoint_saver is not None and broker_wait_enabled
               else new_tool_call_id("ui-place-via"))
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {
        "tool": "ui.place_via",
        "args": args,
        "call_id": call_id,
    }})
    if broker_wait_enabled and not dry_run:
        emit_tool_approval_state()
        if checkpoint_saver is not None:
            return dispatch_checkpointed_tool("ui.place_via", args)
        return wait_for_broker_result(call_id)
    return "Action dispatched to CCad client."

@tool
def ui_add_track(x1: float, y1: float, x2: float, y2: float):
    """Adds a track segment between two coordinates."""
    args = {"start_x_mm": x1, "start_y_mm": y1, "end_x_mm": x2, "end_y_mm": y2}
    call_id = (checkpoint_tool_call_id("ui.route_track", args)
               if checkpoint_saver is not None and broker_wait_enabled
               else new_tool_call_id("ui-route-track"))
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.route_track", "args": args, "call_id": call_id}})
    if broker_wait_enabled:
        emit_tool_approval_state()
        if checkpoint_saver is not None:
            return dispatch_checkpointed_tool("ui.route_track", args)
        return wait_for_broker_result(call_id)
    return "Action dispatched to CCad client."

@tool
def ui_place_footprint(name: str, x: float, y: float):
    """Places a footprint component."""
    return dispatch_client_tool("ui.place_footprint", {"name": name, "x": x, "y": y}, await_result=True)

@tool
def ui_add_polygon(points: List[List[float]], layer: str):
    """Adds a polygon pour on a specific layer."""
    # Assuming the first two points map to start and end for rectangular zones for parity
    if len(points) >= 2:
        x1, y1 = points[0][0], points[0][1]
        x2, y2 = points[1][0], points[1][1]
        args = {"start_x_mm": x1, "start_y_mm": y1, "end_x_mm": x2, "end_y_mm": y2, "layer": layer}
        call_id = (checkpoint_tool_call_id("ui.add_zone", args)
                   if checkpoint_saver is not None and broker_wait_enabled
                   else new_tool_call_id("ui-add-zone"))
        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.add_zone", "args": args, "layer": layer, "call_id": call_id}})
        if broker_wait_enabled:
            emit_tool_approval_state()
            if checkpoint_saver is not None:
                return dispatch_checkpointed_tool("ui.add_zone", args)
            return wait_for_broker_result(call_id)
    return "Action dispatched to CCad client."

@tool
def ui_place_symbol(name: str, x: float, y: float):
    """Places a schematic symbol on the schematic editor."""
    return dispatch_client_tool("ui.place_symbol", {"name": name, "x": x, "y": y}, await_result=True)

@tool
def project_review():
    """Generates a project review summary of the current board state."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "project.review", "args": {}}})
    return "Action dispatched to CCad client."

@tool
def ui_screenshot():
    """Takes a screenshot of the current GUI."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.screenshot", "args": {}}})
    return "Action dispatched to CCad client."

@tool
def ui_open_component_wizard():
    """Opens the AI Component Designer Wizard in the GUI."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.open_component_wizard", "args": {}}})
    return "Component Wizard requested from CCad client."

@tool
def ui_add_wire(x1: float, y1: float, x2: float, y2: float):
    """Adds a wire segment between two coordinates on the schematic."""
    return dispatch_client_tool("ui.add_wire", {"x1": x1, "y1": y1, "x2": x2, "y2": y2}, await_result=True)

@tool
def ui_add_label(text: str, x: float, y: float, global_label: bool = False):
    """Adds a text label to the schematic at the specified coordinates."""
    return dispatch_client_tool("ui.add_label", {"text": text, "x": x, "y": y, "global": global_label}, await_result=True)

@tool
def lib_catalog_info(component_id: str):
    """Gets metadata info from the CCad library catalog for a specific component ID."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "lib.catalog_info", "args": {"component_id": component_id}}})
    return "Action dispatched to CCad client."

@tool
def lib_catalog_search(query: str):
    """Searches the CCad library catalog for components matching a query."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "lib.catalog_search", "args": {"query": query}}})
    return "Action dispatched to CCad client."

router_tools = [ui_place_via, ui_add_track, ui_add_polygon]
librarian_tools = [ui_place_footprint, ui_place_symbol, project_review, ui_add_wire, ui_add_label, lib_catalog_info, lib_catalog_search]
general_tools = [ui_screenshot, ui_open_component_wizard]

llm = None
router_llm = None
librarian_llm = None
checkpoint_saver = None
checkpoint_context = None

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

class MockProvider:
    """Deterministic offline provider for harness and protocol tests."""
    def __init__(self):
        self.tool_issued = False

    def bind_tools(self, _tools):
        return self

    def invoke(self, messages, config=None):
        prompt = "\n".join(str(getattr(message, "content", "")) for message in messages)
        # The supervisor must return a routing decision; only the routing
        # expert may emit the tool call.  Keeping these two responses distinct
        # makes the offline harness exercise the same graph edges as a real
        # provider instead of accidentally terminating at the supervisor.
        if "supervisor managing" in prompt.lower():
            return AIMessage(content="router")
        if ("PCB Routing Expert" in prompt and "via" in prompt.lower()
                and not self.tool_issued):
            self.tool_issued = True
            dry_run = os.environ.get("CCAD_MOCK_MUTATION", "").lower() != "1"
            return AIMessage(content="", tool_calls=[{
                "name": "ui_place_via",
                "args": {"x_mm": 10.0, "y_mm": 10.0, "dry_run": dry_run},
                "id": "mock-tool-1",
                "type": "tool_call",
            }])
        return AIMessage(content="[mock provider] Request understood. Use approved CCad tools for design changes.")

callbacks = []
if os.environ.get("LANGFUSE_PUBLIC_KEY") and os.environ.get("LANGFUSE_SECRET_KEY"):
    try:
        from langfuse.callback import CallbackHandler
        langfuse_handler = CallbackHandler()
        callbacks.append(langfuse_handler)
    except ImportError:
        pass

# LangSmith is opt-in and provider-owned: no credentials means no exporter,
# no network. Passing handler at graph level captures node/tool runs too.
if (os.environ.get("LANGCHAIN_TRACING_V2", "").lower() == "true"
        and os.environ.get("LANGCHAIN_API_KEY")):
    try:
        from langchain.callbacks.tracers import LangChainTracer
        callbacks.append(LangChainTracer(
            project_name=os.environ.get("LANGCHAIN_PROJECT", "ccad")))
    except ImportError:
        pass

def emit_provider_failure(provider: str, error: Exception):
    """Report adapter failure without exposing key, prompt, or endpoint data."""
    emit({"jsonrpc": "2.0", "method": "provider_state", "params": {
        "provider": provider, "configured": True, "execution_enabled": False,
        "error": type(error).__name__, "secret_value_visible": False,
    }})

def classify_provider_error(error: Exception):
    """Return safe, actionable category; never include secret-bearing text."""
    status = getattr(error, "status_code", None)
    text = str(error).lower()
    if status in (401, 403) or any(marker in text for marker in ("unauthorized", "invalid api key", "authentication")):
        return "authentication"
    if status == 404 or any(marker in text for marker in ("model not found", "does not exist", "unknown model")):
        return "model_not_found"
    if status in (402, 429) or any(marker in text for marker in ("rate limit", "quota", "too many requests", "insufficient credits")):
        return "quota_or_rate_limit"
    if isinstance(error, (TimeoutError,)) or "timeout" in text:
        return "timeout"
    if isinstance(error, (ImportError, ModuleNotFoundError)):
        return "dependency"
    return "provider_unavailable"

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

def init_provider():
    global llm, router_llm, librarian_llm, broker_wait_enabled

    # Reconfiguration must not retain a previously initialized adapter or its
    # credential-backed client after a key/provider is removed.
    llm = None
    router_llm = None
    librarian_llm = None
    broker_wait_enabled = False
    
    provider = os.environ.get("CCAD_PROVIDER") or config_manager.get("provider", "openai")
    # Keep GUI provider IDs and adapter IDs identical at the process boundary.
    # The local-model label is a ChatOpenAI-compatible OpenAI protocol server.
    if provider == "local_model_server":
        provider = "local_model"
    if provider == "openai_compatible" and os.environ.get("CCAD_OPENAI_COMPATIBLE_API_KEY"):
        os.environ.setdefault("OPENAI_API_KEY", os.environ["CCAD_OPENAI_COMPATIBLE_API_KEY"])
    if provider == "openrouter" and os.environ.get("OPENROUTER_API_KEY"):
        os.environ.setdefault("OPENAI_API_KEY", os.environ["OPENROUTER_API_KEY"])
    if provider == "cerebras" and os.environ.get("CEREBRAS_API_KEY"):
        os.environ.setdefault("OPENAI_API_KEY", os.environ["CEREBRAS_API_KEY"])
    if provider == "local_model" and os.environ.get("CCAD_LOCAL_MODEL_API_KEY"):
        os.environ.setdefault("OPENAI_API_KEY", os.environ["CCAD_LOCAL_MODEL_API_KEY"])
    model_name = os.environ.get("CCAD_MODEL") or config_manager.get("model", "")
    if provider == "openai_compatible":
        model_name = os.environ.get("CCAD_OPENAI_COMPATIBLE_MODEL") or model_name
    elif provider == "openrouter":
        model_name = os.environ.get("CCAD_OPENROUTER_MODEL") or model_name
    elif provider == "cerebras":
        model_name = os.environ.get("CCAD_CEREBRAS_MODEL") or model_name
    elif provider == "local_model":
        model_name = os.environ.get("CCAD_LOCAL_MODEL_NAME") or model_name
    elif provider == "google_gemini":
        model_name = os.environ.get("CCAD_GEMINI_MODEL") or model_name

    if provider == "mock":
        llm = MockProvider()
        router_llm = llm
        librarian_llm = llm
        emit({"jsonrpc": "2.0", "method": "provider_state", "params": {
            "provider": "mock", "configured": True, "execution_enabled": True,
            "network_access": False, "secret_value_visible": False,
        }})
        return True
    
    if provider == "anthropic":
        try:
            from langchain_anthropic import ChatAnthropic
            if not model_name: model_name = "claude-opus-5"
            llm = ChatAnthropic(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            broker_wait_enabled = True
            emit_provider_ready(provider, model_name)
            return True
        except ImportError:
            emit_dependency_warning("langchain_anthropic")
        except Exception as error:
            emit_provider_failure(provider, error)
    if provider == "google_gemini":
        try:
            from langchain_google_genai import ChatGoogleGenerativeAI
            if not model_name: model_name = "gemini-3.8-flash"
            # The UI/API uses the provider-neutral GEMINI_API_KEY name; the
            # LangChain Google adapter reads GOOGLE_API_KEY.
            google_api_key = os.environ.get("GEMINI_API_KEY") or os.environ.get("GOOGLE_API_KEY")
            if google_api_key:
                os.environ["GOOGLE_API_KEY"] = google_api_key
            llm = ChatGoogleGenerativeAI(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            broker_wait_enabled = True
            emit_provider_ready(provider, model_name)
            return True
        except ImportError:
            emit_dependency_warning("langchain_google_genai")
        except Exception as error:
            emit_provider_failure(provider, error)
    if provider in ("openai", "openai_compatible", "openrouter", "local_model", "cerebras"):
        try:
            from langchain_openai import ChatOpenAI
            if provider == "openai_compatible":
                model_name = model_name or os.environ.get("CCAD_OPENAI_COMPATIBLE_MODEL", "") or "default"
                base_url = os.environ.get("CCAD_OPENAI_COMPATIBLE_BASE_URL", "")
            elif provider == "openrouter":
                model_name = model_name or "openrouter/auto"
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
            else:
                model_name = model_name or "gpt-5.1"
                base_url = ""
            kwargs = {"model": model_name, "temperature": 0,
                      "timeout": provider_timeout_seconds()}
            if base_url:
                kwargs["base_url"] = base_url
            llm = ChatOpenAI(**kwargs)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            broker_wait_enabled = True
            emit_provider_ready(provider, model_name)
            return True
        except ImportError:
            emit_dependency_warning("langchain_openai")
        except Exception as error:
            emit_provider_failure(provider, error)
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Agent provider '{provider}' unavailable. Configure its environment in Agent Settings; local CCad tools remain available."}})
    return False

provider_initialized = init_provider()
# Runtime readiness is independent from provider readiness. Harnesses can now
# distinguish "Python agent process is alive" from "selected API adapter works".
emit({"jsonrpc": "2.0", "method": "backend_state", "params": {
    "runtime": "python",
    "ready": True,
    "provider_initialized": bool(provider_initialized),
    "network_access": "not_probed",
    "secret_value_visible": False,
}})


def invoke_agent_run(state):
    """Invoke graph under one run span without exporting prompt contents."""
    with tracer.start_as_current_span("agent_run") as span:
        span.set_attribute("ccad.agent.workflow", active_workflow)
        span.set_attribute("ccad.agent.provider_ready", llm is not None)
        run_config = {"run_name": "ccad_agent_run"}
        thread_id = state.get("thread_id") or os.environ.get("CCAD_AGENT_THREAD_ID", "ccad-local")
        run_config["configurable"] = {"thread_id": thread_id}
        # Metadata is deliberately non-content: prompt, context, tool args, and
        # credentials must not be exported by observability callbacks.
        run_config["metadata"] = {
            "ccad_provider": os.environ.get("CCAD_PROVIDER", "configured"),
            "ccad_workflow": active_workflow,
            "ccad_context_present": bool(state.get("context", "")),
            "ccad_thread_id_present": bool(thread_id),
        }
        run_config["tags"] = ["ccad", "agent", active_workflow]
        # Bound supervisor -> specialist -> tool cycles.  This is a safety
        # limit, not a provider retry: a malformed tool call must not consume
        # quota forever while chaining is enabled.
        try:
            recursion_limit = int(os.environ.get("CCAD_AGENT_RECURSION_LIMIT", "12"))
        except ValueError:
            recursion_limit = 12
        run_config["recursion_limit"] = min(32, max(4, recursion_limit))
        if callbacks:
            run_config["callbacks"] = callbacks
        return executor.invoke(state, config=run_config)



def get_system_prompt(role_desc: str) -> str:
    base_prompt = config_manager.get("system_prompt", "")
    person_config = config_manager.get("personalisation", {})
    personality = person_config.get("agent_personality", "Default")
    custom_inst = person_config.get("custom_instructions", "")
    
    parts = [f"You are {role_desc}"]
    if base_prompt: parts.append(f"System Base: {base_prompt}")
    
    # Inject active workflow context
    if active_workflow == "routing_pass":
        parts.append("Current Phase: ROUTING. You must strictly focus on trace placement, impedance matching, and differential pairs. Use ui_add_track and ui_place_via.")
    elif active_workflow == "placement_pass":
        parts.append("Current Phase: PLACEMENT. Focus on component alignment, signal flow, and thermal separation. Use ui_place_footprint.")
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
        retries = min(2, max(0, int(os.environ.get("CCAD_PROVIDER_RETRIES", "2"))))
    except ValueError:
        retries = 2

    def quota_or_rate_limited(error):
        status = getattr(error, "status_code", None)
        text = str(error).lower()
        return status in (402, 403, 429) or any(
            marker in text for marker in ("rate limit", "quota", "too many requests", "insufficient credits")
        )

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

@trace_function("supervisor_node")
def supervisor_node(state: AgentState):
    if "pre node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("pre node", emit, "supervisor")
    # Enforce workflow routing
    if active_workflow == "routing_pass":
        return {"next_node": "router"}
    elif active_workflow == "placement_pass":
        return {"next_node": "librarian"}

    if not llm:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Error: Supervisor LLM not initialized. Please configure a provider."}})
        return {"next_node": "END"}
        
    context_str = state.get("context", "")
    role_desc = "a supervisor managing a PCB routing expert and a component librarian expert."
    system_text = get_system_prompt(role_desc)
    system_text += f"\nCurrent Context: {context_str}\nBased on the user's request, decide who should act next. Respond ONLY with 'router', 'librarian', or 'FINISH'."
    
    system_msg = SystemMessage(content=system_text)
    
    prompt = [system_msg] + state["messages"]
    response = invoke_provider_with_retry(
        llm, prompt, config={"callbacks": callbacks} if callbacks else {})
    content = response.content.strip().lower()
    
    if "router" in content:
        next_node = "router"
    elif "librarian" in content:
        next_node = "librarian"
    else:
        next_node = "FINISH"
    if "post node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post node", emit, f"supervisor -> {next_node}")
        
    # Preserve supervisor response so caller can present actual agent output;
    # previously only routing decision survived and chat echoed the user turn.
    return {"next_node": next_node, "messages": [response]}

@trace_function("router_node")
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
    response = invoke_provider_with_retry(
        router_llm, prompt, config={"callbacks": callbacks} if callbacks else {})
    if "post node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post node", emit, "router")
    return {"messages": [response]}

@trace_function("librarian_node")
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
    response = invoke_provider_with_retry(
        librarian_llm, prompt, config={"callbacks": callbacks} if callbacks else {})
    if "post node" in [h.lower() for h in active_hooks]:
        hooks.trigger_hook("post node", emit, "librarian")
    return {"messages": [response]}

from langgraph.prebuilt import ToolNode
execute_tool_node = ToolNode(router_tools + librarian_tools + general_tools)

def should_route(state: AgentState):
    if not chaining_state:
        return END

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
    
    if hasattr(last_message, "tool_calls") and last_message.tool_calls:
        return "execute_tool"
        
    if "<TOOL>" in last_message.content:
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
    return executor.invoke(
        Command(resume=resume_value),
        config={"configurable": {"thread_id": thread_id}},
    )

session_messages = []
active_workflow = "default"
chaining_phase = "none"
chaining_state = True
active_hooks = []
schedules = []
last_context_revision = ""

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

def compact_session_history(messages):
    """Compact chat history without sending content to any provider.

    Keep recent turns intact; replace older turns with opaque, local metadata.
    This bounds prompt growth while avoiding a misleading provider-generated
    summary or leakage of project/chat content in the compaction event.
    """
    if len(messages) <= 4:
        return messages
    recent = messages[-4:]
    older = messages[:-4]
    chars = sum(len(str(getattr(item, "content", "") or "")) for item in older)
    summary = SystemMessage(content=(
        "[CCAD local context summary] older_messages="
        f"{len(older)}; older_chars={chars}; details omitted."
    ))
    return [summary, *recent]

# --- Custom Workflows ---
def handle_marketplace(text: str):
    parts = text.split(" ")
    if len(parts) >= 2 and parts[1] == "install":
        plugin_name = " ".join(parts[2:])
        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "marketplace.install", "args": {"plugin": plugin_name}}})
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Marketplace: Installing plugin '{plugin_name}'..."}})
        
        # Actually register it into config so it persists
        installed = config_manager.get("installed_plugins", [])
        if plugin_name not in installed:
            installed.append(plugin_name)
            config_manager.update("installed_plugins", installed)
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Plugin '{plugin_name}' activated and hooked into context."}})
        else:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Plugin '{plugin_name}' is already installed."}})
    else:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Marketplace: Unknown command. Use `/marketplace install <plugin>`."}})

def get_dynamic_marketplace_catalog():
    # Scan dynamic plugins if they exist, fallback to core + installed state
    installed = config_manager.get("installed_plugins", [])
    core_plugins = [
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

if __name__ == "__main__":
    # --- OTel Setup ---
    try:
        from opentelemetry.instrumentation.langchain import LangchainInstrumentor
        LangchainInstrumentor().instrument()
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "OpenTelemetry Langchain Instrumentation enabled."}})
    except ImportError:
        pass

    init_checkpointer()
    executor = create_orchestrator()
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Python Multi-Agent Orchestrator ready."}})
    
    inbound_queue = queue.Queue()
    def read_protocol_lines():
        for protocol_line in sys.stdin:
            if not route_protocol_line(protocol_line):
                inbound_queue.put(protocol_line)
        inbound_queue.put(None)
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
            if method == "agent.test_provider":
                # Transient test: never update config_manager or write config.
                params = req.get("params", {})
                provider_id = params.get("provider", "openai")
                model = params.get("model", "").strip()
                secret = params.get("secret", "")
                os.environ["CCAD_PROVIDER"] = provider_id
                if model:
                    os.environ["CCAD_MODEL"] = model
                    model_env = {"google_gemini": "CCAD_GEMINI_MODEL",
                                 "openai_compatible": "CCAD_OPENAI_COMPATIBLE_MODEL",
                                 "openrouter": "CCAD_OPENROUTER_MODEL",
                                 "cerebras": "CCAD_CEREBRAS_MODEL",
                                 "local_model": "CCAD_LOCAL_MODEL_NAME",
                                 "local_model_server": "CCAD_LOCAL_MODEL_NAME"}.get(provider_id)
                    if model_env: os.environ[model_env] = model
                env_names = {"openai": "OPENAI_API_KEY", "anthropic": "ANTHROPIC_API_KEY",
                             "google_gemini": "GEMINI_API_KEY",
                             "openai_compatible": "CCAD_OPENAI_COMPATIBLE_API_KEY",
                             "openrouter": "OPENROUTER_API_KEY",
                             "cerebras": "CEREBRAS_API_KEY",
                             "local_model": "CCAD_LOCAL_MODEL_API_KEY",
                             "local_model_server": "CCAD_LOCAL_MODEL_API_KEY"}
                env_name = env_names.get(provider_id, "OPENAI_API_KEY")
                if secret:
                    os.environ[env_name] = secret
                    if provider_id == "google_gemini": os.environ["GOOGLE_API_KEY"] = secret
                    if provider_id in ("openai_compatible", "openrouter", "local_model", "local_model_server", "cerebras"):
                        os.environ["OPENAI_API_KEY"] = secret
                else:
                    os.environ.pop(env_name, None)
                    if provider_id == "google_gemini": os.environ.pop("GOOGLE_API_KEY", None)
                    if provider_id in ("openai_compatible", "openrouter", "local_model", "local_model_server", "cerebras"):
                        os.environ.pop("OPENAI_API_KEY", None)
                provider_ready = init_provider()
                if not provider_ready:
                    # Settings' Test Provider needs a terminal state even when
                    # no key/adapter exists; otherwise GUI waits for its
                    # timeout and reports a misleading "no response".
                    emit({"jsonrpc": "2.0", "method": "provider_state", "params": {
                        "provider": provider_id,
                        "model": model,
                        "configured": bool(secret),
                        "execution_enabled": False,
                        "error": "provider_unavailable" if secret else "missing_api_key",
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
                    "local_model_server": "CCAD_LOCAL_MODEL_API_KEY",
                }
                env_name = env_names.get(provider_id, "OPENAI_API_KEY")
                if secret:
                    os.environ[env_name] = secret
                    if provider_id == "google_gemini":
                        os.environ["GOOGLE_API_KEY"] = secret
                    if provider_id in ("openai_compatible", "openrouter", "local_model", "local_model_server", "cerebras"):
                        os.environ["OPENAI_API_KEY"] = secret
                else:
                    os.environ.pop(env_name, None)
                    if provider_id == "google_gemini":
                        os.environ.pop("GOOGLE_API_KEY", None)
                    if provider_id in ("openai_compatible", "openrouter", "local_model", "local_model_server", "cerebras"):
                        os.environ.pop("OPENAI_API_KEY", None)
                init_provider()
                emit({"jsonrpc": "2.0", "method": "provider_state", "params": {
                    "provider": provider_id,
                    "configured": bool(secret),
                    "execution_enabled": llm is not None,
                    "secret_value_visible": False,
                }})
            elif method == "agent.set_thread_id":
                thread_id = req.get("params", {}).get("thread_id", "").strip()
                if thread_id:
                    os.environ["CCAD_AGENT_THREAD_ID"] = thread_id
                else:
                    os.environ.pop("CCAD_AGENT_THREAD_ID", None)
                emit({"jsonrpc": "2.0", "method": "thread_state", "params": {
                    "configured": bool(thread_id), "secret_value_visible": False,
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
                provider_id = req.get("params", {}).get("provider", "openrouter")
                if provider_id == "openrouter":
                    emit({"jsonrpc": "2.0", "method": "provider_models",
                          "params": {"provider": provider_id, **fetch_openrouter_models()}})
                elif provider_id == "cerebras":
                    emit({"jsonrpc": "2.0", "method": "provider_models",
                          "params": {"provider": provider_id, **cerebras_model_snapshot()}})
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
                if checkpoint_saver is None and pending_result_queue is None:
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
                if checkpoint_saver is not None:
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
                    resume_value = {"error": error} if error is not None else result
                    resumed = resume_checkpointed_run(thread_id, resume_value)
                    emit({"jsonrpc": "2.0", "method": "thread_resumed", "params": {
                        "thread_id": thread_id,
                        "call_id": received_call_id,
                        "message_count": len(resumed.get("messages", [])) if isinstance(resumed, dict) else 0,
                    }})
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
                    if checkpoint_saver is not None:
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
                text = req.get("params", {}).get("text", "")
                raw_context = req.get("params", {}).get("context", "")
                if not isinstance(raw_context, str):
                    raw_context = str(raw_context or "")
                request_context_present = bool(raw_context.strip())
                memory_context = local_memory_context()
                memory_context_present = bool(memory_context)
                if memory_context:
                    raw_context = (raw_context + "\n\n" + memory_context).strip()
                context_str = bound_context_text(raw_context)
                context_truncated = len(context_str) < len(raw_context)
                intake = scan_intake(text, raw_context)
                emit({"jsonrpc": "2.0", "method": "intake_state", "params": intake})
                if not intake["accepted"]:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": "Request blocked by CCad intake guardrail; remove instruction injection or inline secret and retry.",
                        "kind": "intake_blocked", "category": intake["category"],
                        "secret_value_visible": False,
                    }})
                    continue
                previous_context_revision = last_context_revision
                current_context_revision = context_revision(context_str)
                context_changed = current_context_revision != previous_context_revision
                context_change_kind = (
                    "initial" if not previous_context_revision else
                    ("changed" if context_changed else "unchanged")
                )
                last_context_revision = current_context_revision
                emit({"jsonrpc": "2.0", "method": "context_state", "params": {
                    "revision": current_context_revision,
                    "previous_revision": previous_context_revision,
                    "changed": context_changed,
                    "change_kind": context_change_kind,
                    "content_present": bool(context_str),
                    "content_size": len(context_str),
                    "original_content_size": len(raw_context),
                    "context_limit": agent_context_limit(),
                    "truncated": context_truncated,
                    "content_emitted": False,
                    "sources": (["request_context"] if request_context_present else []) +
                               (["local_project_memory"] if memory_context_present else []),
                    "memory_content_emitted": False,
                }})
                
                # Robust Command Parser
                if text.startswith("/"):
                    cmd_parts = text.split(" ", 1)
                    cmd_base = cmd_parts[0].lower()
                    cmd_args = cmd_parts[1] if len(cmd_parts) > 1 else ""
                    
                    if cmd_base == "/commands":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`"}})
                        continue
                    elif cmd_base == "/memory":
                        memory_args = cmd_args.strip()
                        if memory_args == "list":
                            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {"entries": memory_store.list()}})
                        elif memory_args.startswith("list scope:"):
                            scope = memory_args[len("list scope:"):].strip()
                            emit({"jsonrpc": "2.0", "method": "memory_state", "params": {"scope": scope, "entries": memory_store.list(scope=scope)}})
                        elif memory_args.startswith("add "):
                            content, title, scope = parse_memory_add_args(memory_args[4:].strip())
                            entry = memory_store.add(content, title=title, scope=scope)
                            emit({"jsonrpc": "2.0", "method": "memory_added", "params": {"id": entry["id"], "scope": entry["scope"]}})
                        elif memory_args.startswith("delete "):
                            removed = memory_store.delete(memory_args[7:].strip())
                            emit({"jsonrpc": "2.0", "method": "memory_deleted", "params": {"removed": removed}})
                        elif memory_args.startswith("update "):
                            entry_id, content, scope, title = parse_memory_update_args(memory_args[7:].strip())
                            entry = memory_store.update(entry_id, content, title=title, scope=scope)
                            emit({"jsonrpc": "2.0", "method": "memory_updated", "params": {"id": entry_id, "updated": entry is not None}})
                        elif memory_args == "clear all":
                            emit({"jsonrpc": "2.0", "method": "memory_cleared", "params": {"removed": memory_store.clear()}})
                        elif memory_args.startswith("clear scope:"):
                            scope = memory_args[len("clear scope:"):].strip()
                            emit({"jsonrpc": "2.0", "method": "memory_cleared", "params": {"scope": scope, "removed": memory_store.clear(scope)}})
                        else:
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Memory: use `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`. Bare clear does nothing."}})
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
                        before = len(session_messages)
                        session_messages = compact_session_history(session_messages)
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Context compacted locally: {before} messages -> {len(session_messages)}; recent turns preserved."}})
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
                        session_messages.append(HumanMessage(content="Start the routing workflow and autoroute the current board context. Please use the ui_add_track and ui_place_via tools to route all unrouted nets based on the context."))
                        # Fall through to graph execution
                    elif cmd_base == "/drc":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Running DRC checks..."}})
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "action.drc", "args": {}}})
                        continue
                    elif cmd_base == "/place":
                        active_workflow = "placement_pass"
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Initiating component placement pass..."}})
                        session_messages.append(HumanMessage(content="Start the placement workflow. Please use the ui_place_footprint tool to optimally place components on the board canvas."))
                        # Fall through to graph execution
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
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Chat history and context cleared."}})
                        continue
                    elif cmd_base == "/settings":
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.open_settings", "args": {}}})
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Opening agent settings panel..."}})
                        continue
                    elif cmd_base == "/help":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/memory list|list scope:x|add [scope:x] [title:y] <text>|update <id> [scope:x] [title:y] <text>|delete <id>|clear all|clear scope:<name>`\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`\n- `/route`\n- `/drc`\n- `/place`\n- `/design`\n- `/explain`\n- `/clear`\n- `/settings`"}})
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

                run_trace_id = "ccad-agent-" + uuid.uuid4().hex
                run_span_id = uuid.uuid4().hex[:16]
                current_run_trace_id = run_trace_id
                current_run_span_id = run_span_id
                emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
                    "run_state": "running", "trace_id": run_trace_id,
                    "span_id": run_span_id, "provider": os.environ.get("CCAD_PROVIDER", "configured"),
                    "token_usage": "unavailable", "cost": "unavailable",
                }})
                try:
                    final_state = invoke_agent_run({"messages": session_messages, "goal": text, "context": context_str, "next_node": ""})
                except Exception as error:
                    emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
                        "run_state": "failed", "trace_id": run_trace_id,
                        "span_id": run_span_id, "error_type": type(error).__name__,
                        "prompt_emitted": False, "secret_value_visible": False,
                    }})
                    emit({"jsonrpc": "2.0", "method": "message", "params": {
                        "text": ("Provider request failed after bounded retries; no tool was executed. "
                                 f"Cause: {classify_provider_error(error)}. "
                                 "Check provider, model ID, key, quota, or network settings."),
                        "kind": "provider_error", "error_type": type(error).__name__,
                        "cause": classify_provider_error(error),
                    }})
                    continue
                session_messages = bound_session_history(final_state["messages"])
                last_msg = session_messages[-1]
                has_tool_calls = bool(getattr(last_msg, "tool_calls", None))
                has_legacy_tool = "<TOOL>" in str(getattr(last_msg, "content", ""))
                emit({"jsonrpc": "2.0", "method": "telemetry", "params": {
                    "run_state": "awaiting_tool_approval" if (has_tool_calls or has_legacy_tool) else "completed",
                    "trace_id": run_trace_id, "span_id": run_span_id,
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
            elif method == "agent.test_export":
                from telemetry import trace_provider
                with tracer.start_as_current_span("test_export_span"):
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Test trace span generated!"}})
                trace_provider.force_flush()
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "OTel span processors flushed successfully."}})
            elif method == "agent.set_config":
                config_data = req.get("params", {})
                for k, v in config_data.items():
                    config_manager.update(k, v)
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Agent configuration saved successfully."}})
                if "provider" in config_data or "model" in config_data:
                    os.environ["CCAD_PROVIDER"] = config_data.get("provider", "openai")
                    os.environ["CCAD_MODEL"] = config_data.get("model", "gpt-5.1")
                    init_provider()
            elif method == "agent.get_config":
                emit({"jsonrpc": "2.0", "method": "config_state", "params": config_manager.config})
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
                except Exception as e:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error generating component via LLM: {e}. Falling back to default."}})
                    pins = [
                        {"pin": "1", "name": "VCC", "type": "Power"},
                        {"pin": "2", "name": "GND", "type": "Power"},
                        {"pin": "3", "name": "IN", "type": "Input"},
                        {"pin": "4", "name": "OUT", "type": "Output"},
                    ]

                emit({"jsonrpc": "2.0", "method": "generated_component", "params": {"pins": pins, "name": "AI_" + pkg}})
            elif method == "agent.get_marketplace_catalog":
                catalog = get_dynamic_marketplace_catalog()
                emit({"jsonrpc": "2.0", "method": "marketplace_catalog", "params": catalog})
            elif "result" in req:
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Tool executed successfully on C++ side."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: {str(e)}"}})
