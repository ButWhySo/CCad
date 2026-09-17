import json
import uuid
from opentelemetry import trace

hook_registry = {}
_fallback_trace_id = uuid.uuid4().hex
_fallback_span_counter = 0

def register_hook(name, callback):
    """Registers a hook callback by name."""
    hook_registry[name.lower()] = callback

def trigger_hook(name, emit_func, *args, **kwargs):
    """Triggers a registered hook by name, safely."""
    name_lower = name.lower()
    if name_lower in hook_registry:
        try:
            hook_registry[name_lower](emit_func, *args, **kwargs)
        except Exception as e:
            emit_func({"jsonrpc": "2.0", "method": "message", "params": {"text": f"[Hook Error: {name}] {e}"}})

def get_current_trace_info():
    """Gets current trace ID and span ID for telemetry reporting."""
    global _fallback_span_counter
    span = trace.get_current_span()
    if span and span.get_span_context().is_valid:
        ctx = span.get_span_context()
        return format(ctx.span_id, '016x'), format(ctx.trace_id, '032x')
    # Keep fallback telemetry correlated when OTel has no active span. Random
    # IDs per event made one run appear as unrelated traces to the GUI.
    _fallback_span_counter += 1
    return format(_fallback_span_counter, '016x'), _fallback_trace_id

# --- Default Hook Actions ---

def on_post_prompt(emit_func, text):
    span_id, trace_id = get_current_trace_info()
    emit_func({"jsonrpc": "2.0", "method": "telemetry", "params": {"run_state": "Processing", "span_id": span_id, "trace_id": trace_id}})

def on_pre_tool_call(emit_func, tool_name):
    span_id, trace_id = get_current_trace_info()
    emit_func({"jsonrpc": "2.0", "method": "telemetry", "params": {"run_state": f"Tool: {tool_name}", "span_id": span_id}})

def on_post_tool_call(emit_func, tool_name):
    emit_func({"jsonrpc": "2.0", "method": "telemetry", "params": {"run_state": "Processing"}})

def on_pre_exit(emit_func):
    emit_func({"jsonrpc": "2.0", "method": "telemetry", "params": {"run_state": "Idle"}})

def on_pre_node(emit_func, node_name):
    span_id, trace_id = get_current_trace_info()
    emit_func({"jsonrpc": "2.0", "method": "telemetry", "params": {"run_state": f"Node: {node_name}", "span_id": span_id}})

def on_post_node(emit_func, node_info):
    pass # Hook extension point

# Register default hooks
register_hook("post prompt", on_post_prompt)
register_hook("pre tool call", on_pre_tool_call)
register_hook("post tool call", on_post_tool_call)
register_hook("pre exit/end", on_pre_exit)
register_hook("pre node", on_pre_node)
register_hook("post node", on_post_node)
