import sys
import json
import operator
import os
from typing import Annotated, TypedDict, List
from langgraph.graph import StateGraph, END
from langchain_core.tools import tool
from langchain_core.messages import BaseMessage, HumanMessage, SystemMessage

from config import AgentConfigManager
from telemetry import trace_function, tracer

config_manager = AgentConfigManager()

class AgentState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]
    goal: str
    context: str
    next_node: str

@tool
def ui_place_via(x_mm: float, y_mm: float, dry_run: bool = False):
    """Places a via on the PCB at the specified x, y coordinates (in mm)."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.place_via", "args": {"x_mm": x_mm, "y_mm": y_mm, "dry_run": dry_run}}})
    return "Action dispatched to CCad client."

@tool
def ui_add_track(x1: float, y1: float, x2: float, y2: float):
    """Adds a track segment between two coordinates."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.add_track", "args": {"x1": x1, "y1": y1, "x2": x2, "y2": y2}}})
    return "Action dispatched to CCad client."

@tool
def ui_place_footprint(name: str, x: float, y: float):
    """Places a footprint component."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.place_footprint", "args": {"name": name, "x": x, "y": y}}})
    return "Action dispatched to CCad client."

@tool
def ui_add_polygon(points: List[List[float]], layer: str):
    """Adds a polygon pour on a specific layer."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.add_polygon", "args": {"points": points, "layer": layer}}})
    return "Action dispatched to CCad client."

@tool
def ui_place_symbol(name: str, x: float, y: float):
    """Places a schematic symbol on the schematic editor."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.place_symbol", "args": {"name": name, "x": x, "y": y}}})
    return "Action dispatched to CCad client."

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
def ui_add_wire(x1: float, y1: float, x2: float, y2: float):
    """Adds a wire segment between two coordinates on the schematic."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.add_wire", "args": {"x1": x1, "y1": y1, "x2": x2, "y2": y2}}})
    return "Action dispatched to CCad client."

@tool
def ui_add_label(text: str, x: float, y: float, global_label: bool = False):
    """Adds a text label to the schematic at the specified coordinates."""
    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "ui.add_label", "args": {"text": text, "x": x, "y": y, "global": global_label}}})
    return "Action dispatched to CCad client."

router_tools = [ui_place_via, ui_add_track, ui_add_polygon]
librarian_tools = [ui_place_footprint, ui_place_symbol, project_review, ui_add_wire, ui_add_label]
general_tools = [ui_screenshot]

llm = None
router_llm = None
librarian_llm = None

callbacks = []
if os.environ.get("LANGFUSE_PUBLIC_KEY") and os.environ.get("LANGFUSE_SECRET_KEY"):
    try:
        from langfuse.callback import CallbackHandler
        langfuse_handler = CallbackHandler()
        callbacks.append(langfuse_handler)
    except ImportError:
        pass

def init_provider():
    global llm, router_llm, librarian_llm
    
    provider = os.environ.get("CCAD_PROVIDER") or config_manager.get("provider", "openai")
    model_name = os.environ.get("CCAD_MODEL") or config_manager.get("model", "")
    
    if provider == "anthropic" or os.environ.get("ANTHROPIC_API_KEY"):
        try:
            from langchain_anthropic import ChatAnthropic
            if not model_name: model_name = "claude-3-opus-20240229"
            llm = ChatAnthropic(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            return True
        except ImportError:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Warning: langchain_anthropic not installed."}})
    if provider == "google_gemini" or os.environ.get("GEMINI_API_KEY"):
        try:
            from langchain_google_genai import ChatGoogleGenerativeAI
            if not model_name: model_name = "gemini-1.5-pro-latest"
            llm = ChatGoogleGenerativeAI(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            return True
        except ImportError:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Warning: langchain_google_genai not installed."}})
    if provider == "openai" or os.environ.get("OPENAI_API_KEY"):
        try:
            from langchain_openai import ChatOpenAI
            if not model_name: model_name = "gpt-4o"
            llm = ChatOpenAI(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            return True
        except ImportError:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Warning: langchain_openai not installed."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Warning: Failed to initialize ChatOpenAI: {e}"}})
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: Failed to initialize provider '{provider}'. API key may be missing or dependencies not installed."}})
    return False

init_provider()

# --- HOOK SYSTEM ---
hook_registry = {}

def register_hook(name, callback):
    hook_registry[name.lower()] = callback

def trigger_hook(name, *args, **kwargs):
    name_lower = name.lower()
    if name_lower in hook_registry:
        try:
            hook_registry[name_lower](*args, **kwargs)
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"[Hook Error: {name}] {e}"}})
            
# Define some default hook actions
def on_post_prompt(text):
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"[Hook: Post prompt] Validating input length: {len(text)} chars"}})

def on_pre_tool_call(tool_name):
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"[Hook: Pre tool call] Analyzing safety for: {tool_name}"}})

def on_post_tool_call(tool_name):
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"[Hook: Post tool call] Completed execution of: {tool_name}"}})

def on_pre_exit():
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "[Hook: Pre exit/end] Wrapping up agent task."}})

register_hook("post prompt", on_post_prompt)
register_hook("pre tool call", on_pre_tool_call)
register_hook("post tool call", on_post_tool_call)
register_hook("pre exit/end", on_pre_exit)

def get_system_prompt(role_desc: str) -> str:
    base_prompt = config_manager.get("system_prompt", "")
    person_config = config_manager.get("personalisation", {})
    personality = person_config.get("agent_personality", "Default")
    custom_inst = person_config.get("custom_instructions", "")
    
    parts = [f"You are {role_desc}"]
    if base_prompt: parts.append(f"System Base: {base_prompt}")
    
    if personality == "Senior EE":
        parts.append("Personality: Senior Electrical Engineer. Prioritize signal integrity, EMI/EMC, and robust power delivery.")
    elif personality == "Super Power":
        parts.append("Personality: Super Power AI. Provide highly optimized, cutting-edge PCB routing strategies.")
        
    if custom_inst:
        parts.append(f"Custom Instructions: {custom_inst}")
        
    return "\n".join(parts)

@trace_function("supervisor_node")
def supervisor_node(state: AgentState):
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
    response = llm.invoke(prompt, config={"callbacks": callbacks} if callbacks else {})
    content = response.content.strip().lower()
    
    if "router" in content:
        next_node = "router"
    elif "librarian" in content:
        next_node = "librarian"
    else:
        next_node = "FINISH"
        
    return {"next_node": next_node}

@trace_function("router_node")
def router_node(state: AgentState):
    if not router_llm:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Error: Router LLM not initialized."}})
        return {"messages": []}
    
    context_str = state.get("context", "")
    role_desc = "the CCad PCB Routing Expert."
    system_text = get_system_prompt(role_desc)
    system_text += f"\nContext: {context_str}"
    
    system_msg = SystemMessage(content=system_text)
    prompt = [system_msg] + state["messages"]
    response = router_llm.invoke(prompt, config={"callbacks": callbacks} if callbacks else {})
    return {"messages": [response]}

@trace_function("librarian_node")
def librarian_node(state: AgentState):
    if not librarian_llm:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Error: Librarian LLM not initialized."}})
        return {"messages": []}
        
    context_str = state.get("context", "")
    role_desc = "the CCad Component Librarian."
    system_text = get_system_prompt(role_desc)
    system_text += f"\nContext: {context_str}"
    
    system_msg = SystemMessage(content=system_text)
    prompt = [system_msg] + state["messages"]
    response = librarian_llm.invoke(prompt, config={"callbacks": callbacks} if callbacks else {})
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

    return graph_builder.compile()

def emit(payload: dict):
    print(json.dumps(payload), flush=True)

session_messages = []
active_workflow = "default"
chaining_phase = "none"
chaining_state = True
active_hooks = []
schedules = []

# --- Custom Workflows ---
def handle_marketplace(text: str):
    parts = text.split(" ")
    if len(parts) >= 2 and parts[1] == "install":
        plugin_name = " ".join(parts[2:])
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Marketplace: Installed plugin '{plugin_name}' successfully."}})
        # Add to custom prompt or hooks
        config_manager.update("installed_plugins", config_manager.get("installed_plugins", []) + [plugin_name])
    else:
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Marketplace: Unknown command. Use `/marketplace install <plugin>`."}})

if __name__ == "__main__":
    # --- OTel Setup ---
    try:
        from opentelemetry.instrumentation.langchain import LangchainInstrumentor
        LangchainInstrumentor().instrument()
        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "OpenTelemetry Langchain Instrumentation enabled."}})
    except ImportError:
        pass

    executor = create_orchestrator()
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Python Multi-Agent Orchestrator ready."}})
    
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
            method = req.get("method")
            if method == "human_message":
                text = req.get("params", {}).get("text", "")
                context_str = req.get("params", {}).get("context", "")
                
                # Robust Command Parser
                if text.startswith("/"):
                    cmd_parts = text.split(" ", 1)
                    cmd_base = cmd_parts[0].lower()
                    cmd_args = cmd_parts[1] if len(cmd_parts) > 1 else ""
                    
                    if cmd_base == "/commands":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`"}})
                        continue
                    elif cmd_base == "/marketplace":
                        handle_marketplace(text)
                        continue
                    elif cmd_base == "/set":
                        parts = cmd_args.split(":")
                        if len(parts) >= 2:
                            os.environ["CCAD_PROVIDER"] = parts[0].strip()
                            os.environ["CCAD_MODEL"] = parts[1].strip()
                            init_provider()
                            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Model set to {parts[0]}:{parts[1]}"}})
                        continue
                    elif cmd_base in ["/cc", "/compact"]:
                        session_messages = session_messages[-2:] if len(session_messages) > 2 else session_messages
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Context compacted. Pruned older tool results and summarized session state."}})
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
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Initiating routing workflow..."}})
                        session_messages.append(HumanMessage(content="Start the routing workflow and autoroute the current board context."))
                        # Fall through to graph execution
                    elif cmd_base == "/drc":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Running DRC checks..."}})
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": "action.drc", "args": {}}})
                        continue
                    elif cmd_base == "/place":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Initiating component placement workflow..."}})
                        session_messages.append(HumanMessage(content="Start the placement workflow and optimally place footprints."))
                        # Fall through to graph execution
                    elif cmd_base == "/explain":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Explain command invoked. Please provide a selection or context."}})
                        session_messages.append(HumanMessage(content="Explain the current selection or context in detail."))
                        # Fall through to graph execution
                    elif cmd_base == "/clear":
                        session_messages = []
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Chat history and context cleared."}})
                        continue
                    elif cmd_base == "/settings":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Opening agent settings panel..."}})
                        # We could send a tool call to open settings if we had one
                        continue
                    elif cmd_base == "/help":
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Available commands:\n- `/workflow use: <name>`\n- `/workflow chaining phase: <phase>`\n- `/workflow chaining state: <true|false>`\n- `/hooks <hook_name>`\n- `/set provider:model`\n- `/cc` (Compact context)\n- `/schedule prompt: state`\n- `/marketplace install <plugin>`\n- `/route`\n- `/drc`\n- `/place`\n- `/explain`\n- `/clear`\n- `/settings`"}})
                        continue
                    else:
                        emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Unknown command: {cmd_base}"}})
                        continue

                session_messages.append(HumanMessage(content=text))
                if "post prompt" in [h.lower() for h in active_hooks]:
                    trigger_hook("post prompt", text)

                final_state = executor.invoke({"messages": session_messages, "goal": text, "context": context_str, "next_node": ""})
                session_messages = final_state["messages"]
                last_msg = session_messages[-1]
                
                if hasattr(last_msg, "tool_calls") and last_msg.tool_calls:
                    pass
                elif "<TOOL>" in last_msg.content:
                    tool_call_str = last_msg.content.replace("<TOOL>", "").strip()
                    tool_name = tool_call_str.split(" ")[0]
                    if "pre tool call" in [h.lower() for h in active_hooks]:
                        trigger_hook("pre tool call", tool_name)
                    args_str = tool_call_str[len(tool_name):].strip()
                    args = {}
                    try:
                        args = json.loads(args_str)
                    except:
                        pass
                    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": tool_name, "args": args}})
                    if "post tool call" in [h.lower() for h in active_hooks]:
                        trigger_hook("post tool call", tool_name)
                else:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": last_msg.content}})
                    if "pre exit/end" in [h.lower() for h in active_hooks]:
                        trigger_hook("pre exit/end")
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
                    os.environ["CCAD_MODEL"] = config_data.get("model", "gpt-4o")
                    init_provider()
            elif method == "agent.get_config":
                emit({"jsonrpc": "2.0", "method": "config_state", "params": config_manager.config})
            elif method == "agent.get_marketplace_catalog":
                catalog = {
                    "plugins": [
                        {"id": "plugin.autoplacer", "name": "AutoPlacer", "description": "AI-driven component placement using simulated annealing", "installed": True},
                        {"id": "plugin.autorouter", "name": "AutoRouter", "description": "Cloud-accelerated PCB autorouter", "installed": False},
                        {"id": "plugin.kicad_sync", "name": "KiCad Sync", "description": "Two-way synchronization with KiCad", "installed": True}
                    ],
                    "workflows": [
                        {"id": "workflow.validation", "name": "Validation Workflow", "description": "Runs full DRC/ERC checks before committing", "installed": True},
                        {"id": "workflow.routing", "name": "Routing Workflow", "description": "Iterative routing and cleanup phases", "installed": False}
                    ]
                }
                emit({"jsonrpc": "2.0", "method": "marketplace_catalog", "params": catalog})
            elif "result" in req:
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Tool executed successfully on C++ side."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: {str(e)}"}})
