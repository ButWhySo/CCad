import sys
import json
import operator
import os
from typing import Annotated, TypedDict, List
from langgraph.graph import StateGraph, END
from langchain_core.messages import BaseMessage, HumanMessage, AIMessage, SystemMessage
from langchain_core.tools import tool

class AgentState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]
    goal: str
    context: str
    next_node: str

@tool
def ui_place_via(x_mm: float, y_mm: float, dry_run: bool = False):
    """Places a via on the PCB at the specified x, y coordinates (in mm)."""
    pass

@tool
def ui_add_track(x1: float, y1: float, x2: float, y2: float):
    """Adds a track segment between two coordinates."""
    pass

@tool
def ui_place_footprint(name: str, x: float, y: float):
    """Places a footprint component."""
    pass

@tool
def project_review():
    """Generates a project review summary of the current board state."""
    pass

@tool
def ui_screenshot():
    """Takes a screenshot of the current GUI."""
    pass

router_tools = [ui_place_via, ui_add_track]
librarian_tools = [ui_place_footprint, project_review]
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
    if os.environ.get("CCAD_ANTHROPIC_MODEL") or os.environ.get("ANTHROPIC_API_KEY"):
        try:
            from langchain_anthropic import ChatAnthropic
            model_name = os.environ.get("CCAD_ANTHROPIC_MODEL", "claude-3-opus-20240229")
            llm = ChatAnthropic(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            return True
        except ImportError:
            pass
    if os.environ.get("CCAD_GEMINI_MODEL") or os.environ.get("GEMINI_API_KEY"):
        try:
            from langchain_google_genai import ChatGoogleGenerativeAI
            model_name = os.environ.get("CCAD_GEMINI_MODEL", "gemini-1.5-pro-latest")
            llm = ChatGoogleGenerativeAI(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            return True
        except ImportError:
            pass
    if os.environ.get("OPENAI_API_KEY") or os.environ.get("CCAD_OPENAI_MODEL"):
        try:
            from langchain_openai import ChatOpenAI
            model_name = os.environ.get("CCAD_OPENAI_MODEL", "gpt-4o")
            llm = ChatOpenAI(model=model_name, temperature=0)
            router_llm = llm.bind_tools(router_tools)
            librarian_llm = llm.bind_tools(librarian_tools)
            return True
        except ImportError:
            pass
    return False

init_provider()

def supervisor_node(state: AgentState):
    if not llm:
        # Mock supervisor
        messages = state.get("messages", [])
        if not messages:
            return {"next_node": "END"}
        last_msg = messages[-1].content.lower()
        if "via" in last_msg or "track" in last_msg or "route" in last_msg:
            return {"next_node": "router"}
        if "footprint" in last_msg or "component" in last_msg or "place" in last_msg:
            return {"next_node": "librarian"}
        return {"next_node": "librarian"} # Default mock fallback
        
    context_str = state.get("context", "")
    system_msg = SystemMessage(content=f"You are a supervisor managing a PCB routing expert and a component librarian expert.\n"
                                       f"Current Context: {context_str}\n"
                                       f"Based on the user's request, decide who should act next. Respond ONLY with 'router', 'librarian', or 'FINISH'.")
    
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

def router_node(state: AgentState):
    if not router_llm:
        messages = state.get("messages", [])
        last_msg = messages[-1].content
        if "via" in last_msg.lower():
            return {"messages": [AIMessage(content="<TOOL>ui.place_via {\"x_mm\": 50, \"y_mm\": 50, \"dry_run\": false}")]}
        return {"messages": [AIMessage(content=f"Echo from Router (Mock): {last_msg}")]}
    
    context_str = state.get("context", "")
    system_msg = SystemMessage(content=f"You are the CCad PCB Routing Expert.\nContext: {context_str}")
    prompt = [system_msg] + state["messages"]
    response = router_llm.invoke(prompt, config={"callbacks": callbacks} if callbacks else {})
    return {"messages": [response]}

def librarian_node(state: AgentState):
    if not librarian_llm:
        messages = state.get("messages", [])
        last_msg = messages[-1].content
        return {"messages": [AIMessage(content=f"Echo from Librarian (Mock): {last_msg}")]}
        
    context_str = state.get("context", "")
    system_msg = SystemMessage(content=f"You are the CCad Component Librarian.\nContext: {context_str}")
    prompt = [system_msg] + state["messages"]
    response = librarian_llm.invoke(prompt, config={"callbacks": callbacks} if callbacks else {})
    return {"messages": [response]}

def execute_tool_node(state: AgentState):
    return {"messages": []}

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

if __name__ == "__main__":
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
                
                final_state = executor.invoke({"messages": [HumanMessage(content=text)], "goal": text, "context": context_str, "next_node": ""})
                last_msg = final_state["messages"][-1]
                
                if hasattr(last_msg, "tool_calls") and last_msg.tool_calls:
                    for tc in last_msg.tool_calls:
                        tool_name = tc["name"]
                        if tool_name == "ui_place_via":
                            tool_name = "ui.place_via"
                        elif tool_name == "ui_screenshot":
                            tool_name = "ui.screenshot"
                        elif tool_name == "project_review":
                            tool_name = "project.review"
                        elif tool_name == "ui_add_track":
                            tool_name = "ui.add_track"
                        elif tool_name == "ui_place_footprint":
                            tool_name = "ui.place_footprint"
                            
                        args = tc["args"]
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": tool_name, "args": args}})
                elif "<TOOL>" in last_msg.content:
                    tool_call_str = last_msg.content.replace("<TOOL>", "").strip()
                    tool_name = tool_call_str.split(" ")[0]
                    args_str = tool_call_str[len(tool_name):].strip()
                    args = {}
                    try:
                        args = json.loads(args_str)
                    except:
                        pass
                    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": tool_name, "args": args}})
                else:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": last_msg.content}})
            elif "result" in req:
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Tool executed successfully on C++ side."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: {str(e)}"}})
