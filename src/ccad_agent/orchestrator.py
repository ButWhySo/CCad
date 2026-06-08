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

# Define some mock tools that LangChain can bind.
# These mirror the native CCad tools. The actual execution happens in C++.
@tool
def ui_place_via(x_mm: float, y_mm: float, dry_run: bool = False):
    """Places a via on the PCB at the specified x, y coordinates (in mm)."""
    pass

@tool
def project_review():
    """Generates a project review summary of the current board state."""
    pass

@tool
def ui_screenshot():
    """Takes a screenshot of the current GUI."""
    pass

tools = [ui_place_via, project_review, ui_screenshot]

# Initialize LLM
llm = None
if os.environ.get("OPENAI_API_KEY"):
    try:
        from langchain_openai import ChatOpenAI
        llm = ChatOpenAI(model="gpt-4o", temperature=0)
        llm = llm.bind_tools(tools)
    except ImportError:
        pass

def should_continue(state: AgentState):
    messages = state.get("messages", [])
    if not messages:
        return END
    last_message = messages[-1]
    
    # If using real LLM with tool_calls
    if hasattr(last_message, "tool_calls") and last_message.tool_calls:
        return "execute_tool"
    
    # Fallback for mock logic
    if "<TOOL>" in last_message.content:
        return "execute_tool"
        
    return END

def plan_node(state: AgentState):
    messages = state.get("messages", [])
    if not messages:
        return {"messages": []}
    
    context_str = state.get("context", "")
    
    if llm:
        system_msg = SystemMessage(content=f"You are the CCad agent. You help the user design PCBs.\nCurrent Context: {context_str}")
        # Build prompt with system message at the start
        prompt = [system_msg] + messages
        response = llm.invoke(prompt)
        return {"messages": [response]}
    else:
        # Mock planner
        last_msg = messages[-1].content
        if "via" in last_msg.lower():
            # In mock mode we use the string format to keep things simple
            return {"messages": [AIMessage(content="<TOOL>ui.place_via {\"x_mm\": 50, \"y_mm\": 50, \"dry_run\": false}")]}
        return {"messages": [AIMessage(content=f"Echo from LangGraph (Mock mode): {last_msg}")]}

def execute_tool_node(state: AgentState):
    # This node doesn't really do anything in our bridge architecture, 
    # because the main loop intercepts the tool call and emits it.
    # The result will come back as a new human message simulating the tool result.
    return {"messages": []}

def create_orchestrator():
    graph_builder = StateGraph(AgentState)
    graph_builder.add_node("planner", plan_node)
    graph_builder.add_node("execute_tool", execute_tool_node)

    graph_builder.set_entry_point("planner")
    graph_builder.add_conditional_edges("planner", should_continue, {"execute_tool": "execute_tool", END: END})
    graph_builder.add_edge("execute_tool", "planner")

    return graph_builder.compile()

def emit(payload: dict):
    print(json.dumps(payload), flush=True)

if __name__ == "__main__":
    executor = create_orchestrator()
    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Python Orchestrator ready."}})
    
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
                
                final_state = executor.invoke({"messages": [HumanMessage(content=text)], "goal": text, "context": context_str})
                last_msg = final_state["messages"][-1]
                
                # Check for real LangChain tool calls
                if hasattr(last_msg, "tool_calls") and last_msg.tool_calls:
                    for tc in last_msg.tool_calls:
                        tool_name = tc["name"]
                        # Langchain replaces . with _ in names, let's map back if needed
                        if tool_name == "ui_place_via":
                            tool_name = "ui.place_via"
                        elif tool_name == "ui_screenshot":
                            tool_name = "ui.screenshot"
                        elif tool_name == "project_review":
                            tool_name = "project.review"
                            
                        args = tc["args"]
                        emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": tool_name, "args": args}})
                elif "<TOOL>" in last_msg.content:
                    # Mock mode tool call
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
                # Tool result returned
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Tool executed successfully on C++ side."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: {str(e)}"}})
