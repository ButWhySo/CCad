"""Exercise history replacement against LangGraph's real checkpoint reducer."""

from pathlib import Path
import sys
from typing import Annotated, TypedDict

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from history_compaction import (HistoryCompactionError,
                                replace_checkpoint_history)
from langchain_core.messages import AIMessage, BaseMessage, HumanMessage
from langgraph.checkpoint.memory import InMemorySaver
from langgraph.graph import END, StateGraph
from langgraph.graph.message import add_messages
from langgraph.types import interrupt


class State(TypedDict):
    messages: Annotated[list[BaseMessage], add_messages]


builder = StateGraph(State)
builder.add_node("finish", lambda _state: {})
builder.set_entry_point("finish")
builder.add_edge("finish", END)
saver = InMemorySaver()
graph = builder.compile(checkpointer=saver)
config = {"configurable": {"thread_id": "semantic-compaction-checkpoint"}}
original = [HumanMessage(content="older user request"),
            AIMessage(content="older assistant outcome"),
            HumanMessage(content="newest user request"),
            AIMessage(content="newest assistant outcome")]
graph.invoke({"messages": original}, config=config)
replacement = [HumanMessage(content="[recap] preserve U3 on F.Cu"),
              HumanMessage(content="newest user request"),
              AIMessage(content="newest assistant outcome")]
assert replace_checkpoint_history(graph, config["configurable"]["thread_id"],
                                  replacement)
saved = graph.get_state(config).values["messages"]
assert len(saved) == len(replacement)
assert [(item.type, item.content) for item in saved] == [
    (item.type, item.content) for item in replacement]
assert [item.id for item in saved[1:]] == [item.id for item in replacement[1:]]


class FailAfterCheckpointWrite:
    """Raise after a real write to prove the helper restores durable history."""

    def __init__(self, wrapped):
        self.wrapped = wrapped
        self.fail_next_update = True

    def get_state(self, target_config):
        return self.wrapped.get_state(target_config)

    def update_state(self, target_config, patch):
        result = self.wrapped.update_state(target_config, patch)
        if self.fail_next_update:
            self.fail_next_update = False
            raise RuntimeError("simulated transport error after durable write")
        return result


rollback_builder = StateGraph(State)
rollback_builder.add_node("finish", lambda _state: {})
rollback_builder.set_entry_point("finish")
rollback_builder.add_edge("finish", END)
rollback_graph = rollback_builder.compile(checkpointer=InMemorySaver())
rollback_config = {"configurable": {"thread_id": "rollback-checkpoint"}}
rollback_graph.invoke({"messages": original}, config=rollback_config)
rollback_executor = FailAfterCheckpointWrite(rollback_graph)
try:
    replace_checkpoint_history(rollback_executor, "rollback-checkpoint", replacement)
except HistoryCompactionError as error:
    assert error.category == "checkpoint_update_failed"
else:
    raise AssertionError("failed checkpoint write was reported as applied")
restored = rollback_graph.get_state(rollback_config).values["messages"]
assert [(item.type, item.content) for item in restored] == [
    (item.type, item.content) for item in original]
assert [item.id for item in restored] == [item.id for item in original]

stale_builder = StateGraph(State)
stale_builder.add_node("finish", lambda _state: {})
stale_builder.set_entry_point("finish")
stale_builder.add_edge("finish", END)
stale_graph = stale_builder.compile(checkpointer=InMemorySaver())
stale_config = {"configurable": {"thread_id": "stale-checkpoint"}}
stale_graph.invoke({"messages": original}, config=stale_config)
stale_state = stale_graph.get_state(stale_config)
expected_ids = [item.id for item in stale_state.values["messages"]]
expected_checkpoint_id = stale_state.config["configurable"]["checkpoint_id"]
stale_graph.update_state(stale_config, {"messages": [
    HumanMessage(content="new turn arrived during summary")
]})
changed_before = list(stale_graph.get_state(stale_config).values["messages"])
try:
    replace_checkpoint_history(
        stale_graph, "stale-checkpoint", replacement,
        expected_message_ids=expected_ids,
        expected_checkpoint_id=expected_checkpoint_id)
except HistoryCompactionError as error:
    assert error.category == "checkpoint_history_changed"
else:
    raise AssertionError("stale summary overwrote a newer conversation")
changed_after = list(stale_graph.get_state(stale_config).values["messages"])
assert [(item.type, item.content) for item in changed_after] == [
    (item.type, item.content) for item in changed_before]

pending_builder = StateGraph(State)


def wait_for_review(_state):
    interrupt("review required")
    return {}


pending_builder.add_node("review", wait_for_review)
pending_builder.set_entry_point("review")
pending_builder.add_edge("review", END)
pending_graph = pending_builder.compile(checkpointer=InMemorySaver())
pending_config = {"configurable": {"thread_id": "pending-review"}}
pending_graph.invoke({"messages": [HumanMessage(content="pending proposal")]},
                     config=pending_config)
before = list(pending_graph.get_state(pending_config).values["messages"])
try:
    replace_checkpoint_history(pending_graph, "pending-review", replacement)
except HistoryCompactionError as error:
    assert error.category == "thread_has_pending_graph_work"
else:
    raise AssertionError("compaction accepted a thread paused for human review")
after = list(pending_graph.get_state(pending_config).values["messages"])
assert [(item.type, item.content) for item in before] == [
    (item.type, item.content) for item in after]

print("PASS real LangGraph checkpoint compaction and pending-review safety")
