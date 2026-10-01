"""Durable memory-extraction job claim, lease, retry, and exclusion contracts."""

import sys
import threading
from pathlib import Path
from tempfile import TemporaryDirectory

from langchain_core.messages import AIMessage, HumanMessage, ToolMessage

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src" / "ccad_agent"))
from conversation_store import ConversationStore  # noqa: E402


def run():
    with TemporaryDirectory(prefix="ccad-memory-jobs-") as temp:
        conversation_db = Path(temp) / "conversations.sqlite3"
        store = ConversationStore(conversation_db)
        store.ensure_thread("root-thread", session_id="root-session")
        store.append_messages("root-thread", [
            HumanMessage(content="For every CCad project, preserve source IDs.", id="u-1"),
            AIMessage(content="Understood.", id="a-1"),
            HumanMessage(content='Example only: {"api_key": "sensitive-value"}', id="u-secret"),
            HumanMessage(content="/memory add transient command", id="u-command"),
            ToolMessage(content='{"api_key":"redacted"}', name="tool",
                         tool_call_id="call-1", id="tool-1"),
        ], turn_id="turn-1", session_id="root-session")
        store.record_turn("root-thread", "turn-1",
                          "For every CCad project, preserve source IDs.",
                          [HumanMessage(content="For every CCad project, preserve source IDs.", id="u-1"),
                           AIMessage(content="Understood.", id="a-1")],
                          outcome="completed", project_id="project-a")

        store.ensure_thread("unfinished", session_id="unfinished")
        store.append_messages("unfinished", [HumanMessage(
            content="Always keep my preferences safe.", id="u-open")], turn_id="open-turn")
        store.ensure_thread("failed-turn", session_id="failed-turn")
        store.append_messages("failed-turn", [HumanMessage(
            content="Always keep my preferences safe.", id="u-failed")], turn_id="failed-turn-id")
        store.record_turn("failed-turn", "failed-turn-id", "Always keep my preferences safe.",
                          [HumanMessage(content="Always keep my preferences safe.", id="u-failed")],
                          outcome="provider_error")

        idle_timestamp = "2020-01-01T00:00:00+00:00"
        db = store._connect()
        try:
            db.execute("UPDATE threads SET updated_at=?", (idle_timestamp,))
            db.commit()
        finally:
            db.close()

        assert store.enqueue_idle_memory_jobs(
            now="2020-01-02T00:00:00+00:00", idle_seconds=60,
            max_jobs=8) == 1
        # A new store instance represents process restart; queued work and its
        # source binding must remain durable and claimable without re-enqueueing.
        store = ConversationStore(conversation_db)
        assert store.memory_extraction_job_state()["queued"] == 1
        extraction_digest = store.memory_extraction_source_digest("root-thread")
        assert extraction_digest
        store.append_messages("root-thread", [
            AIMessage(content="Assistant-only change", id="a-later"),
            ToolMessage(content="Tool-only change", name="tool",
                        tool_call_id="call-later", id="tool-later"),
        ], turn_id="turn-1", session_id="root-session")
        assert store.memory_extraction_source_digest("root-thread") == extraction_digest
        assert store.enqueue_idle_memory_jobs(
            now="2020-01-02T00:00:00+00:00", idle_seconds=60,
            max_jobs=8) == 0
        state = store.memory_extraction_job_state()
        assert state == {"queued": 1, "claimed": 0, "succeeded": 0,
                         "no_memory": 0, "stale": 0, "failed": 0}

        concurrency_store = ConversationStore(Path(temp) / "concurrency.sqlite3")
        for thread_id in ("concurrent-a", "concurrent-b"):
            concurrency_store.ensure_thread(thread_id, session_id=thread_id)
            user_text = "For every project, preserve source IDs."
            message = HumanMessage(content=user_text,
                                   id=f"user-{thread_id}")
            turn_id = f"turn-{thread_id}"
            concurrency_store.append_messages(thread_id, [message], turn_id=turn_id)
            concurrency_store.record_turn(thread_id, turn_id, user_text,
                                          [message], outcome="completed", project_id="p")
        db = concurrency_store._connect()
        try:
            db.execute("UPDATE threads SET updated_at=?", (idle_timestamp,))
            db.commit()
        finally:
            db.close()
        assert concurrency_store.enqueue_idle_memory_jobs(
            now="2020-01-02T00:00:00+00:00", idle_seconds=60, max_jobs=8) == 2
        assert concurrency_store.claim_memory_extraction_job(
            now="2020-01-02T00:00:00+00:00") is not None
        assert concurrency_store.claim_memory_extraction_job(
            now="2020-01-02T00:00:00+00:00") is None
        state = concurrency_store.memory_extraction_job_state()
        assert state["claimed"] == 1 and state["queued"] == 1

        claims = []
        barrier = threading.Barrier(3)

        def claim():
            barrier.wait()
            claims.append(store.claim_memory_extraction_job(
                now="2020-01-02T00:00:00+00:00", lease_seconds=60,
                max_attempts=3))

        workers = [threading.Thread(target=claim) for _ in range(2)]
        for worker in workers:
            worker.start()
        barrier.wait()
        for worker in workers:
            worker.join(timeout=5)
            assert not worker.is_alive()
        claimed = [item for item in claims if item is not None]
        assert len(claimed) == 1, "one queued job must have exactly one owner"
        job = claimed[0]
        assert job["thread_id"] == "root-thread"
        assert job["attempt_count"] == 1
        assert store.memory_extraction_source_digest("root-thread") == job["source_digest"]

        transcript = store.load_memory_extraction_messages("root-thread")
        assert [item["message_id"] for item in transcript] == [
            "u-1", "u-secret"]
        assert "sensitive-value" not in str(transcript)
        assert "Assistant-only" not in str(transcript)
        assert "Tool-only" not in str(transcript)

        assert not store.complete_memory_extraction_job(
            job["job_id"], "stale-claim-token", "succeeded", candidate_count=1)
        assert store.fail_memory_extraction_job(
            job["job_id"], job["claim_token"], "provider_timeout",
            now="2020-01-02T00:00:00+00:00", max_attempts=3) == "queued"
        assert store.claim_memory_extraction_job(
            now="2020-01-02T00:00:29+00:00", lease_seconds=60,
            max_attempts=3) is None
        retry = store.claim_memory_extraction_job(
            now="2020-01-02T00:01:00+00:00", lease_seconds=60,
            max_attempts=3)
        assert retry is not None and retry["attempt_count"] == 2
        expired_lease = store.claim_memory_extraction_job(
            now="2020-01-02T00:02:01+00:00", lease_seconds=30,
            max_attempts=3)
        assert expired_lease is not None and expired_lease["attempt_count"] == 3
        assert store.claim_memory_extraction_job(
            now="2020-01-02T00:03:00+00:00", lease_seconds=30,
            max_attempts=3) is None
        assert store.memory_extraction_job_state()["failed"] == 1
        # A new canonical user event changes digest and admits one fresh job.
        store.append_messages("root-thread", [
            HumanMessage(content="I prefer compact reports in every project.", id="u-2")],
            turn_id="turn-2")
        store.record_turn("root-thread", "turn-2",
                          "I prefer compact reports in every project.",
                          [HumanMessage(content="I prefer compact reports in every project.", id="u-2")],
                          outcome="completed", project_id="project-a")
        db = store._connect()
        try:
            db.execute("UPDATE threads SET updated_at=? WHERE thread_id=?",
                       (idle_timestamp, "root-thread"))
            db.commit()
        finally:
            db.close()
        assert store.enqueue_idle_memory_jobs(
            now="2020-01-02T00:00:00+00:00", idle_seconds=60,
            max_jobs=8, max_queue=1) == 1
        assert store.enqueue_idle_memory_jobs(
            now="2020-01-02T00:00:00+00:00", idle_seconds=60,
            max_jobs=8, max_queue=1) == 0
        final_job = store.claim_memory_extraction_job(
            now="2020-01-02T00:00:00+00:00", lease_seconds=60,
            max_attempts=3)
        assert final_job is not None and final_job["attempt_count"] == 1
        assert store.complete_memory_extraction_job(
            final_job["job_id"], final_job["claim_token"], "no_memory",
            candidate_count=0)
        assert store.memory_extraction_job_state()["no_memory"] == 1

        store.ensure_thread("ephemeral", session_id="ephemeral", no_memory=True)
        store.append_messages("ephemeral", [HumanMessage(content="Remember this", id="e-1")])
        db = store._connect()
        try:
            db.execute("UPDATE threads SET updated_at=? WHERE thread_id=?",
                       (idle_timestamp, "ephemeral"))
            db.commit()
        finally:
            db.close()
        assert store.enqueue_idle_memory_jobs(
            now="2020-01-02T00:00:00+00:00", idle_seconds=60,
            max_jobs=8) == 0


if __name__ == "__main__":
    run()
    print("memory extraction durable-job contracts passed")
