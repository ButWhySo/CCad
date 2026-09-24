"""Real isolated JSON-RPC persistence/restart test; provider network is disabled."""

import json
import os
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
AGENT_DIR = ROOT / "src" / "ccad_agent"


def exchange(env, requests):
    process = subprocess.run(
        [sys.executable, str(AGENT_DIR / "orchestrator.py")],
        input="".join(json.dumps(item) + "\n" for item in requests),
        text=True, capture_output=True, cwd=AGENT_DIR, env=env, timeout=45,
        check=False)
    if process.returncode != 0:
        raise AssertionError(
            f"isolated Agent process exited {process.returncode}: {process.stderr[-2000:]}")
    events = []
    for line in process.stdout.splitlines():
        try:
            events.append(json.loads(line))
        except json.JSONDecodeError:
            raise AssertionError(f"non-JSON Agent protocol output: {line[:200]}")
    return events


def run():
    with TemporaryDirectory(prefix="ccad-conversation-runtime-") as temporary:
        appdata = Path(temporary)
        db_path = appdata / "conversations.sqlite3"
        env = os.environ.copy()
        env.update({
            "APPDATA": str(appdata),
            "LOCALAPPDATA": str(appdata),
            "CCAD_AGENT_CONVERSATION_DB": str(db_path),
            "CCAD_AGENT_DEFER_PROVIDER_INIT": "1",
            "CCAD_TRACE_EXPORT_ENABLED": "0",
            "CCAD_PROVIDER": "google_gemini",
        })
        for key_name in (
            "OPENAI_API_KEY", "ANTHROPIC_API_KEY", "GEMINI_API_KEY", "GOOGLE_API_KEY",
            "OPENROUTER_API_KEY", "CEREBRAS_API_KEY", "CCAD_OPENAI_COMPATIBLE_API_KEY",
            "CCAD_LOCAL_MODEL_API_KEY", "CCAD_OLLAMA_API_KEY", "LANGFUSE_PUBLIC_KEY",
            "LANGFUSE_SECRET_KEY", "OTEL_EXPORTER_OTLP_HEADERS",
        ):
            env.pop(key_name, None)

        thread = {"thread_id": "roundtrip-thread", "session_id": "roundtrip-session",
                  "project_id": "isolated-project"}
        first = exchange(env, [
            {"jsonrpc": "2.0", "id": 1, "method": "agent.set_thread_id", "params": thread},
            {"jsonrpc": "2.0", "id": 2, "method": "human_message", "params": {
                **thread, "text": "Inspect U3 and preserve its position.", "context": "{}"}},
        ])
        assert any(item.get("method") == "thread_state" and
                   item.get("params", {}).get("conversation_message_count") == 0
                   for item in first)
        assert any(item.get("method") == "message" and
                   item.get("params", {}).get("kind") == "provider_unavailable"
                   for item in first)
        assert db_path.is_file()

        second = exchange(env, [
            {"jsonrpc": "2.0", "id": 3, "method": "agent.set_thread_id", "params": thread},
            {"jsonrpc": "2.0", "id": 4, "method": "human_message", "params": {
                "thread_id": "other-thread", "session_id": "other-session",
                "project_id": "isolated-project", "text": "Check J4 on B.Cu.",
                "context": "{}"}},
            {"jsonrpc": "2.0", "id": 5, "method": "agent.set_thread_id", "params": thread},
            {"jsonrpc": "2.0", "id": 6, "method": "agent.set_thread_id", "params": {
                "thread_id": "other-thread", "session_id": "other-session",
                "project_id": "isolated-project"}},
        ])
        loaded_counts = [item["params"].get("conversation_message_count")
                         for item in second if item.get("method") == "thread_state"]
        assert loaded_counts == [2, 2, 2]

        sys.path.insert(0, str(AGENT_DIR))
        from conversation_store import ConversationStore
        store = ConversationStore(db_path)
        records = store.search_turn_records("roundtrip-thread", "Inspect U3 preserve position")
        assert records and records[0]["outcome"] == "provider_unavailable"
        assert records[0]["source_message_ids"]
        assert store.thread_recap("roundtrip-thread")["source_turn_ids"] == [
            records[0]["turn_id"]]
        assert "Inspect U3" in store.load_messages("roundtrip-thread")[0].content
        assert [message.content for message in store.load_messages("other-thread")
                if message.type == "human"] == ["Check J4 on B.Cu."]
        assert not store.search_turn_records("roundtrip-thread", "J4 B.Cu")
        assert store.search_turn_records("other-thread", "J4 B.Cu")


if __name__ == "__main__":
    run()
    print("Conversation IPC persistence/restart contract passed; provider calls: 0.")
