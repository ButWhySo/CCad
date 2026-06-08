import operator
from typing import Annotated, TypedDict, List
from langgraph.graph import StateGraph, END
from langchain_core.messages import BaseMessage, HumanMessage, AIMessage

class AgentState(TypedDict):
    messages: Annotated[List[BaseMessage], operator.add]
    goal: str

def should_continue(state: AgentState):
    messages = state["messages"]
    last_message = messages[-1]
    if "<TOOL>" in last_message.content:
        return "execute_tool"
    return END

def plan_node(state: AgentState):
    # This is a stub for the planner that would call an LLM.
    messages = state.get("messages", [])
    print(f"[Orchestrator] Planning for goal: {state.get('goal')}")
    return {"messages": [AIMessage(content="Plan generated. <TOOL>ccad_cli {}")]}

def execute_tool_node(state: AgentState):
    # This is a stub for the tool execution that interfaces with CCad CLI or GUI UI map.
    messages = state.get("messages", [])
    print("[Orchestrator] Executing tool...")
    return {"messages": [AIMessage(content="Tool execution result: OK")]}

def create_orchestrator():
    graph_builder = StateGraph(AgentState)
    graph_builder.add_node("planner", plan_node)
    graph_builder.add_node("execute_tool", execute_tool_node)

    graph_builder.set_entry_point("planner")
    graph_builder.add_conditional_edges("planner", should_continue, {"execute_tool": "execute_tool", END: END})
    graph_builder.add_edge("execute_tool", "planner")

    return graph_builder.compile()

if __name__ == "__main__":
    executor = create_orchestrator()
    print("Orchestrator initialized.")
