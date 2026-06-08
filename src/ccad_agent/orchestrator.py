import sys
import json
import operator
from typing import Annotated, TypedDict, List
from langgraph.graph import StateGraph, END
from langchain_core.messages import BaseMessage, HumanMessage, AIMessage

class AgentState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]
    goal: str

def should_continue(state: AgentState):
    messages = state.get("messages", [])
    if not messages:
        return END
    last_message = messages[-1]
    if "<TOOL>" in last_message.content:
        return "execute_tool"
    return END

def plan_node(state: AgentState):
    # Mock planner
    messages = state.get("messages", [])
    if not messages:
        return {"messages": []}
    
    last_msg = messages[-1].content
    if "via" in last_msg.lower():
        return {"messages": [AIMessage(content="<TOOL>pcb.add-via {}")]}
    
    return {"messages": [AIMessage(content=f"Echo from LangGraph: {last_msg}")]}

def execute_tool_node(state: AgentState):
    # Mock tool execution
    return {"messages": [AIMessage(content="Tool execution complete.")]}

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
                
                # Run the graph
                final_state = executor.invoke({"messages": [HumanMessage(content=text)], "goal": text})
                last_msg = final_state["messages"][-1].content
                
                if "<TOOL>" in last_msg:
                    # emit tool call
                    tool_name = last_msg.replace("<TOOL>", "").strip().split(" ")[0]
                    emit({"jsonrpc": "2.0", "method": "tool_call", "params": {"tool": tool_name, "args": {}}})
                else:
                    emit({"jsonrpc": "2.0", "method": "message", "params": {"text": last_msg}})
            elif "result" in req:
                # Tool result returned
                emit({"jsonrpc": "2.0", "method": "message", "params": {"text": "Tool executed successfully on C++ side."}})
        except Exception as e:
            emit({"jsonrpc": "2.0", "method": "message", "params": {"text": f"Error: {str(e)}"}})

