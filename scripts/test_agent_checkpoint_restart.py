"""Two-process proof for CCad's checkpointed interrupt/resume contract."""
import os
import sys
import json
from pathlib import Path

from langchain_core.messages import AIMessage, HumanMessage
from langgraph.checkpoint.sqlite import SqliteSaver
from langgraph.types import Command

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))
import orchestrator as ccad  # noqa: E402


class FakeModel:
    def bind_tools(self, _tools):
        return self

    def invoke(self, messages, config=None):
        if any(message.__class__.__name__ == "ToolMessage" for message in messages):
            return AIMessage(content="FINISH")
        if any("supervisor" in str(getattr(m, "content", "")).lower() for m in messages):
            return AIMessage(content="router")
        return AIMessage(content="", tool_calls=[{
            "name": "ui_add_track",
            "args": {"x1": 1.0, "y1": 1.0, "x2": 2.0, "y2": 2.0},
            "id": "restart-tool",
            "type": "tool_call",
        }])


def configure(db):
    ccad.checkpoint_context = SqliteSaver.from_conn_string(str(db))
    ccad.checkpoint_saver = ccad.checkpoint_context.__enter__()
    ccad.checkpoint_saver.setup()
    ccad.llm = FakeModel()
    ccad.router_llm = ccad.llm
    ccad.librarian_llm = ccad.llm
    ccad.broker_wait_enabled = True
    ccad.executor = ccad.create_orchestrator()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_agent_checkpoint_restart.py <db>")
    db = sys.argv[1]
    phase = os.environ.get("CCAD_RESTART_PHASE", "first")
    thread = {"configurable": {"thread_id": "restart-proof"}}
    configure(db)
    if phase == "first":
        state = ccad.executor.invoke({
            "messages": [HumanMessage(content="route this board")],
            "goal": "route this board", "context": "", "next_node": "",
        }, config=thread)
        snapshot = ccad.executor.get_state(thread)
        interrupts = [item for task in snapshot.tasks for item in task.interrupts]
        assert interrupts, snapshot
        assert interrupts[0].value["kind"] == "ccad_tool_call"
        pending = ccad.pending_call_snapshot("restart-proof")
        assert pending["checkpoint_call_ids"] == [interrupts[0].value["call_id"]], pending
        assert pending["secret_value_visible"] is False
        print("PASS interrupt checkpoint written")
    elif phase == "second":
        state = ccad.executor.invoke(Command(resume={"status": "track_added"}), config=thread)
        assert not ccad.executor.get_state(thread).next
        tool_messages = [message for message in state["messages"]
                         if message.__class__.__name__ == "ToolMessage"]
        assert tool_messages and json.loads(tool_messages[-1].content) == {"status": "track_added"}
        print("PASS restart resume completed")
    elif phase == "denial":
        state = ccad.executor.invoke(Command(resume={"error": {"code": -32001,
                                                                  "message": "approval_denied"}}), config=thread)
        assert not ccad.executor.get_state(thread).next
        tool_messages = [message for message in state["messages"]
                         if message.__class__.__name__ == "ToolMessage"]
        assert tool_messages and json.loads(tool_messages[-1].content) == {
            "error": {"code": -32001, "message": "approval_denied"}}
        print("PASS restart denial completed")
    elif phase == "cancel":
        state = ccad.executor.invoke(Command(resume={"error": {
            "code": -32800, "message": "canceled_by_user"}}), config=thread)
        assert not ccad.executor.get_state(thread).next
        tool_messages = [message for message in state["messages"]
                         if message.__class__.__name__ == "ToolMessage"]
        assert tool_messages and json.loads(tool_messages[-1].content) == {
            "error": {"code": -32800, "message": "canceled_by_user"}}
        print("PASS restart cancellation completed")


if __name__ == "__main__":
    main()
