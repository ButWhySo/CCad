"""Contracts for durable, thread-scoped conversation projection."""

import importlib.util
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from langchain_core.messages import AIMessage, HumanMessage, ToolMessage

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src" / "ccad_agent"))
MODULE = Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "conversation_store.py"
SPEC = importlib.util.spec_from_file_location("ccad_conversation_store", MODULE)
assert SPEC is not None and SPEC.loader is not None
CONVERSATION = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = CONVERSATION
SPEC.loader.exec_module(CONVERSATION)
ConversationStore = CONVERSATION.ConversationStore
budgeted_history_window = CONVERSATION.budgeted_history_window


def run():
    with TemporaryDirectory(prefix="ccad-conversation-contract-") as temp:
        store = ConversationStore(Path(temp) / "conversations.sqlite3")
        thread_a = "thread-a"
        thread_b = "thread-b"
        user = HumanMessage(content="Place U3 near the USB connector.", id="user-1")
        tool_call = AIMessage(content="", id="assistant-1", tool_calls=[{
            "name": "project.inspect", "args": {"reference": "U3"}, "id": "call-1",
            "type": "tool_call",
        }])
        tool_result = ToolMessage(content='{"reference":"U3"}', name="project.inspect",
                                  tool_call_id="call-1", id="tool-1")
        answer = AIMessage(content="U3 is 12 mm from the USB connector.", id="assistant-2")

        assert store.append_messages(thread_a, [user, tool_call, tool_result, answer],
                                     turn_id="turn-1", session_id="session-a",
                                     project_id="project-a") == 4
        assert store.append_messages(thread_a, [user, tool_call, tool_result, answer],
                                     turn_id="turn-1") == 0
        assert store.append_messages(thread_b, [HumanMessage(content="Other chat", id="other-1")],
                                     turn_id="turn-b") == 1

        loaded = store.load_messages(thread_a)
        assert [message.id for message in loaded] == [
            "user-1", "assistant-1", "tool-1", "assistant-2"]
        assert loaded[1].tool_calls[0]["id"] == "call-1"
        assert loaded[2].tool_call_id == "call-1"
        assert [message.content for message in store.load_messages(thread_b)] == ["Other chat"]
        assert {item["thread_id"] for item in store.list_threads()} == {thread_a, thread_b}

        record = store.record_turn(
            thread_a, "turn-1", "Place U3 near the USB connector.", loaded,
            outcome="completed", project_id="project-a", project_revision_before="rev-1",
            project_revision_after="rev-1")
        assert record["source_message_ids"] == ["user-1", "assistant-1", "tool-1", "assistant-2"]
        assert record["tool_ids"] == ["project.inspect"]
        assert record["referenced_entities"]["reference"] == ["U3"]
        assert record["lexical_fields"]["outcome"] == "completed"
        assert record["assistant_summary"] == "U3 is 12 mm from the USB connector."
        assert store.search_turn_records(thread_a, "USB connector U3")
        assert not store.search_turn_records(thread_b, "USB connector U3")

        prior_thread = "thread-prior"
        prior_messages = [HumanMessage(content="Preserve GND return clearance around U3",
                                      id="prior-user"),
                          AIMessage(content="GND return clearance around U3 was preserved",
                                    id="prior-answer")]
        store.append_messages(prior_thread, prior_messages, turn_id="prior-turn",
                              project_id="project-a")
        store.record_turn(prior_thread, "prior-turn",
            "Preserve GND return clearance around U3", prior_messages,
            outcome="completed", project_id="project-a")
        other_project = "thread-other-project"
        foreign_messages = [HumanMessage(content="Preserve GND return clearance around U3",
                                         id="foreign-user"),
                            AIMessage(content="Foreign project match", id="foreign-answer")]
        store.append_messages(other_project, foreign_messages, turn_id="foreign-turn",
                              project_id="project-b")
        store.record_turn(other_project, "foreign-turn",
            "Preserve GND return clearance around U3", foreign_messages,
            outcome="completed", project_id="project-b")
        weak_thread = "thread-weak"
        weak_messages = [HumanMessage(content="GND routing note", id="weak-user"),
                         AIMessage(content="Single shared term only", id="weak-answer")]
        store.append_messages(weak_thread, weak_messages, turn_id="weak-turn",
                              project_id="project-a")
        store.record_turn(weak_thread, "weak-turn", "GND routing note", weak_messages,
                          outcome="completed", project_id="project-a")
        scoped = store.search_project_history("project-a",
            "GND return clearance U3", exclude_thread_id=thread_a)
        assert [item["turn_id"] for item in scoped] == ["prior-turn"]
        assert scoped[0]["retrieval_scope"] == "active_project"
        assert scoped[0]["source_message_ids"] == ["prior-user", "prior-answer"]
        assert store.search_project_history("project-a", "GND return",
                                            exclude_thread_id="thread-prior") == []
        assert store.search_project_history("", "GND") == []

        secret = HumanMessage(content="api_key=fixture-secret-value", id="secret-1")
        store.append_messages(thread_a, [secret], turn_id="turn-secret")
        persisted_secret = store.load_messages(thread_a)[-1].content
        assert "fixture-secret-value" not in persisted_secret
        assert "[REDACTED]" in persisted_secret

        old_user = HumanMessage(content="Old question", id="old-user")
        old_call = AIMessage(content="", id="old-ai", tool_calls=[{
            "name": "project.inspect", "args": {}, "id": "old-call", "type": "tool_call"}])
        old_result = ToolMessage(content="x" * 5000, name="project.inspect",
                                 tool_call_id="old-call", id="old-result")
        old_answer = AIMessage(content="Old answer", id="old-answer")
        newest = HumanMessage(content="Keep my current request verbatim", id="newest")
        window = budgeted_history_window(
            [old_user, old_call, old_result, old_answer, newest],
            token_budget=64, max_messages=8)
        assert window[-1].content == "Keep my current request verbatim"
        assert not any(message.id in {"old-call", "old-result"} for message in window)
        tool_window = budgeted_history_window(
            [old_user, old_call, old_result, old_answer],
            token_budget=2000, max_messages=8)
        assert [message.id for message in tool_window] == [
            "old-user", "old-ai", "old-result", "old-answer"]
        compacted_window = budgeted_history_window(
            [newest, AIMessage(content="Tool called", id="new-ai", tool_calls=[{
                "name": "project.inspect", "args": {}, "id": "new-call",
                "type": "tool_call"}]),
             ToolMessage(content="x" * 8000, name="project.inspect",
                         tool_call_id="new-call", id="new-result"),
             AIMessage(content="Completed with verified result.", id="new-answer")],
            token_budget=64, max_messages=8)
        assert [message.id for message in compacted_window] == ["newest", "new-answer"]
        long_answer = AIMessage(content="z" * 10000, id="long-answer")
        clipped_window = budgeted_history_window([newest, long_answer],
                                                token_budget=128, max_messages=8)
        assert clipped_window[0] is newest
        assert clipped_window[1].id == "long-answer"
        assert "full transcript is preserved" in clipped_window[1].content
        assert len(long_answer.content) == 10000

        store.compact_projection(thread_a, [answer], turn_id="turn-compact")
        assert [message.id for message in store.load_model_messages(thread_a)] == ["assistant-2"]
        raw_ids = [message.id for message in store.load_messages(thread_a)]
        assert raw_ids[:4] == ["user-1", "assistant-1", "tool-1", "assistant-2"]
        assert raw_ids[-1] == "secret-1"
        assert store.turn_id_for_message(thread_a, "user-1") == "turn-1"
        assert store.turn_id_for_message(thread_a, "missing") == ""
        store.clear_model_projection(thread_a)
        assert store.load_model_messages(thread_a) == []
        assert [message.id for message in store.load_messages(thread_a)] == raw_ids
        assert store.search_turn_records(thread_a, "USB")
        recap = store.thread_recap(thread_a)
        assert recap["source_turn_ids"] == ["turn-1"]
        assert recap["turns"][0]["turn_id"] == "turn-1"


if __name__ == "__main__":
    run()
    print("Conversation store contracts passed.")
