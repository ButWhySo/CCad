"""End-to-end provider-boundary, write-gate, stale-input, and opt-out contracts."""

import json
import sys
from contextlib import contextmanager
from pathlib import Path
from tempfile import TemporaryDirectory

from langchain_core.messages import HumanMessage

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src" / "ccad_agent"))
from conversation_store import ConversationStore  # noqa: E402
from memory_extraction import MemoryExtractionError, MemoryExtractionWorker  # noqa: E402
from memory_store import MemoryStore  # noqa: E402


NOW = "2020-01-02T00:00:00+00:00"
OLD = "2020-01-01T00:00:00+00:00"


def seed(store, thread_id="thread-a"):
    store.ensure_thread(thread_id, session_id=thread_id)
    store.append_messages(thread_id, [HumanMessage(
        content="For every CCad project, preserve exact source IDs.", id="event-a")],
        turn_id="turn-a")
    store.record_turn(thread_id, "turn-a",
                      "For every CCad project, preserve exact source IDs.",
                      [HumanMessage(content="For every CCad project, preserve exact source IDs.",
                                    id="event-a")],
                      outcome="completed", project_id="project-a")
    db = store._connect()
    try:
        db.execute("UPDATE threads SET updated_at=? WHERE thread_id=?", (OLD, thread_id))
        db.commit()
    finally:
        db.close()


def run():
    def stores(root):
        return (ConversationStore(root / "conversations.sqlite3"),
                MemoryStore(root / "memories.json"))

    with TemporaryDirectory(prefix="ccad-memory-extraction-worker-") as temp:
        root = Path(temp)
        conversations, memories = stores(root)
        seed(conversations)
        enabled = [True]
        observed_prompts = []
        trace_metadata = []

        def provider(prompt):
            observed_prompts.append(prompt)
            source_ref = json.loads(prompt)[0]["source_ref"]
            return json.dumps({"candidates": [{
                "source_ref": source_ref,
                "evidence_quote": "For every CCad project, preserve exact source IDs.",
                "kind": "fact",
            }]})

        @contextmanager
        def observe(thread_id, metadata):
            trace_metadata.append((thread_id, dict(metadata)))
            yield None

        writes = []
        worker = MemoryExtractionWorker(
            conversations, memories, provider=provider, enabled=lambda: enabled[0],
            namespace=lambda: "local-user", on_memory_written=lambda: writes.append(True),
            observe=observe, idle_seconds=60)
        assert worker.run_once(now=NOW) == "succeeded"
        assert len(observed_prompts) == 1 and writes == [True]
        assert len(trace_metadata) == 1
        thread_id, metadata = trace_metadata[0]
        assert thread_id == "thread-a"
        assert metadata["thread_hash"] != thread_id
        assert set(metadata) == {"thread_hash", "event_count", "input_chars"}
        assert "source IDs" not in str(metadata)
        saved = memories.list(tier="episodic", namespace="local-user")
        assert len(saved) == 1
        item = saved[0]
        assert item["content"] == "For every CCad project, preserve exact source IDs."
        assert item["kind"] == "fact" and item["scope"] == "user"
        assert item["provenance"]["authorship"] == "auto_generated"
        assert item["provenance"]["explicit_user_evidence"] is False
        assert item["provenance"]["source_event_ids"] == ["event-a"]
        assert item["provenance"]["source_turn_ids"] == ["turn-a"]

        # Same transcript digest is idempotent and does not call provider again.
        assert worker.run_once(now=NOW) == "idle"
        assert len(observed_prompts) == 1

        stale_root = root / "stale"
        stale_root.mkdir()
        stale_conversations, stale_memories = stores(stale_root)
        seed(stale_conversations, "thread-stale")
        def mutate_during_provider(prompt):
            stale_conversations.append_messages("thread-stale", [HumanMessage(
                content="For every project, keep evidence references.", id="event-b")],
                turn_id="turn-b")
            return json.dumps({"candidates": []})
        stale_worker = MemoryExtractionWorker(
            stale_conversations, stale_memories, provider=mutate_during_provider,
            enabled=lambda: True, namespace=lambda: "local-user",
            on_memory_written=lambda: writes.append(True), idle_seconds=60)
        assert stale_worker.run_once(now=NOW) == "stale"
        assert not stale_memories.list(tier="episodic", namespace="local-user")

        disabled_root = root / "disabled"
        disabled_root.mkdir()
        disabled_conversations, disabled_memories = stores(disabled_root)
        seed(disabled_conversations, "thread-disabled")
        no_calls = []
        disabled = MemoryExtractionWorker(
            disabled_conversations, disabled_memories,
            provider=lambda _prompt: no_calls.append(True) or "{}",
            enabled=lambda: False, namespace=lambda: "local-user",
            on_memory_written=lambda: writes.append(True), idle_seconds=60)
        assert disabled.run_once(now=NOW) == "disabled"
        assert not no_calls

        unavailable_root = root / "provider-unavailable"
        unavailable_root.mkdir()
        unavailable_conversations, unavailable_memories = stores(unavailable_root)
        seed(unavailable_conversations, "thread-provider-unavailable")
        provider_calls = []
        unavailable = MemoryExtractionWorker(
            unavailable_conversations, unavailable_memories,
            provider=lambda prompt: provider_calls.append(prompt) or "{}",
            enabled=lambda: True, provider_ready=lambda: False,
            namespace=lambda: "local-user", on_memory_written=lambda: None,
            idle_seconds=60)
        assert unavailable.run_once(now=NOW) == "provider_unavailable"
        assert provider_calls == []
        assert unavailable_conversations.memory_extraction_job_state()["queued"] == 0

        quota_root = root / "quota"
        quota_root.mkdir()
        quota_conversations, quota_memories = stores(quota_root)
        seed(quota_conversations, "thread-quota")
        def quota_failure(_prompt):
            raise MemoryExtractionError("quota_exhausted")
        quota_worker = MemoryExtractionWorker(
            quota_conversations, quota_memories,
            provider=quota_failure, enabled=lambda: True,
            classify_error=lambda error: getattr(error, "category", "failed"),
            namespace=lambda: "local-user", on_memory_written=lambda: writes.append(True),
            idle_seconds=60)
        assert quota_worker.run_once(now=NOW) == "failed"
        assert quota_conversations.memory_extraction_job_state()["queued"] == 0
        assert quota_conversations.memory_extraction_job_state()["failed"] == 1

        ambiguous_root = root / "ambiguous-limit"
        ambiguous_root.mkdir()
        ambiguous_conversations, ambiguous_memories = stores(ambiguous_root)
        seed(ambiguous_conversations, "thread-ambiguous-limit")
        def ambiguous_failure(_prompt):
            raise MemoryExtractionError("quota_or_rate_limit")
        ambiguous_worker = MemoryExtractionWorker(
            ambiguous_conversations, ambiguous_memories,
            provider=ambiguous_failure,
            enabled=lambda: True, namespace=lambda: "local-user",
            on_memory_written=lambda: None,
            classify_error=lambda error: getattr(error, "category", "failed"),
            idle_seconds=60)
        assert ambiguous_worker.run_once(now=NOW) == "failed"
        ambiguous_state = ambiguous_conversations.memory_extraction_job_state()
        assert ambiguous_state["queued"] == 0 and ambiguous_state["failed"] == 1

        disabled_during_root = root / "disabled-during"
        disabled_during_root.mkdir()
        disabled_during_conversations, disabled_during_memories = stores(disabled_during_root)
        seed(disabled_during_conversations, "thread-disabled-during")
        holder = {}
        def disable_during_provider(prompt):
            holder["worker"].set_enabled(False)
            source_ref = json.loads(prompt)[0]["source_ref"]
            return json.dumps({"candidates": [{
                "source_ref": source_ref,
                "evidence_quote": "For every CCad project, preserve exact source IDs.",
                "kind": "fact",
            }]})
        racing_worker = MemoryExtractionWorker(
            disabled_during_conversations, disabled_during_memories,
            provider=disable_during_provider, enabled=lambda: True,
            namespace=lambda: "local-user", on_memory_written=lambda: writes.append(True),
            idle_seconds=60)
        holder["worker"] = racing_worker
        assert racing_worker.run_once(now=NOW) == "stale"
        assert not disabled_during_memories.list(tier="episodic", namespace="local-user")


if __name__ == "__main__":
    run()
    print("memory extraction worker contracts passed")
