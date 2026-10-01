"""No-network memory CRUD, deletion, bounds, and secret-rejection proof."""

import json
import tempfile
import sys
import os
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "ccad_agent"))
from memory_store import MemoryStore


with tempfile.TemporaryDirectory() as temp:
    store_path = Path(temp) / "memory.json"
    store = MemoryStore(store_path)
    assert store.ensure_namespace("ltm", "thread-1") == []
    assert store_path.is_file()
    item = store.add("Use 0.25 mm minimum track width", title="routing", tags=["pcb"],
                     tier="episodic", namespace="local-user", scope="user",
                     kind="preference", provenance={
                         "authorship": "user_authored",
                         "explicit_user_evidence": True,
                         "source_evidence_classes": ["explicit_user_command"],
                         "source_thread_ids": ["thread-a"],
                         "source_turn_ids": ["turn-a"],
                     })
    assert store.list()[0]["id"] == item["id"]
    assert store.list()[0]["kind"] == "preference"
    assert store.list()[0]["importance"] == 3
    assert item["provenance"]["authorship"] == "user_authored"
    assert item["provenance"]["explicit_user_evidence"] is True
    assert item["provenance"]["source_thread_ids"] == ["thread-a"]
    assert item["provenance"]["source_turn_ids"] == ["turn-a"]
    pinned = store.update(item["id"], item["content"], pinned=True)
    assert pinned["provenance"]["pinned"] is True
    unpinned = store.update(item["id"], item["content"], pinned=False)
    assert unpinned["provenance"]["pinned"] is False
    try:
        store.update(item["id"], item["content"], pinned="yes")
    except ValueError:
        pass
    else:
        raise AssertionError("non-boolean memory pin state was accepted")
    assert store.list(scope="other") == []
    updated = store.update(item["id"], "Use 0.30 mm minimum track width", title="updated")
    assert updated["id"] == item["id"]
    assert updated["tier"] == "episodic"
    assert updated["namespace"] == "local-user"
    assert updated["scope"] == "user"
    assert updated["kind"] == "preference"
    assert updated["provenance"] == item["provenance"]
    assert updated["importance"] == 3
    recorded = store.record_usage([item["id"], "missing"],
                                  used_at="2026-09-25T12:00:00+00:00")
    assert list(recorded) == [item["id"]]
    assert recorded[item["id"]]["use_count"] == 1
    assert recorded[item["id"]]["last_used_at"] == "2026-09-25T12:00:00+00:00"
    assert recorded[item["id"]]["content"] == updated["content"]
    updated = store.update(item["id"], "Use 0.28 mm minimum track width")
    assert updated["use_count"] == 1
    assert updated["last_used_at"] == "2026-09-25T12:00:00+00:00"
    updated = store.update(item["id"], "Use 0.28 mm minimum track width",
                           importance=5)
    assert updated["importance"] == 5
    assert store.list()[0]["importance"] == 5
    assert store.list()[0]["use_count"] == 1
    assert updated["updated_at"]
    assert store.list()[0]["content"].startswith("Use 0.28")

    verified = store.verify(
        item["id"], provenance={
            "authorship": "user_authored", "explicit_user_evidence": True,
            "source_evidence_classes": ["explicit_user_command"],
            "source_event_ids": ["verify-event"]})
    assert verified["last_verified_at"]
    assert verified["provenance"]["source_event_ids"] == ["verify-event"]
    replacement = store.normalise(
        "Use 0.25 mm minimum track width", scope="user", tier="episodic",
        namespace="local-user", kind="correction", importance=4,
        provenance={"authorship": "user_authored", "explicit_user_evidence": True,
                    "source_evidence_classes": ["explicit_user_command"],
                    "source_event_ids": ["supersede-event"],
                    "source_memory_ids": [item["id"]]})
    superseding = store.supersede(item["id"], replacement)
    assert superseding["supersedes"] == item["id"]
    assert superseding["last_verified_at"]
    assert store.list(tier="episodic", namespace="local-user") == [superseding]
    history = store.list(tier="episodic", namespace="local-user",
                         include_superseded=True)
    old_record = next(record for record in history if record["id"] == item["id"])
    assert old_record["status"] == "superseded"
    assert old_record["superseded_by"] == superseding["id"]
    assert old_record["content"] == "Use 0.28 mm minimum track width"
    assert old_record["last_verified_at"] == verified["last_verified_at"]
    assert store.update(item["id"], "Do not rewrite preserved history") is None
    assert store.record_usage([item["id"]]) == {}
    legacy_path = Path(temp) / "legacy.json"
    legacy_path.write_text('[{"id":"old","content":"legacy preference","tier":"ltm"}]',
                           encoding="utf-8")
    legacy = MemoryStore(legacy_path).list()[0]
    assert legacy["kind"] == "fact"
    assert legacy["importance"] == 3
    assert legacy["provenance"]["authorship"] == "unknown"
    assert legacy["provenance"]["explicit_user_evidence"] is False
    legacy_disk = legacy_path.read_text(encoding="utf-8")
    assert '"kind"' not in legacy_disk and '"importance"' not in legacy_disk
    assert store.delete(superseding["id"]) is True
    assert store.delete(item["id"]) is True
    assert store.list() == []
    try:
        store.add("Invalid memory classification", kind="speculation")
    except ValueError as error:
        assert "kind" in str(error)
    else:
        raise AssertionError("unknown memory kind was accepted")
    for importance in (0, 6, True, "5", 2.5):
        try:
            store.add("Invalid importance value", importance=importance)
        except ValueError as error:
            assert "importance" in str(error)
        else:
            raise AssertionError("invalid user-controlled importance was accepted")
    try:
        store.add("api_key: do-not-store")
    except ValueError as error:
        assert "secret" in str(error)
    else:
        raise AssertionError("secret-looking memory was accepted")
    for value in ("sk-testabcdef0123456789", "ghp_123456789012345678901234567890", "Bearer abcdef1234567890"):
        try:
            store.add(value)
        except ValueError:
            pass
        else:
            raise AssertionError("provider credential pattern was accepted")
    for kwargs in ({"title": "api_key: hidden"}, {"tags": ["token: hidden"]},
                   {"namespace": "password: hidden"}):
        try:
            store.add("safe content", **kwargs)
        except ValueError:
            pass
        else:
            raise AssertionError("secret-looking memory metadata was accepted")
    try:
        store.add("expired policy", expires_at="not-a-time")
    except ValueError as error:
        assert "ISO-8601" in str(error)
    else:
        raise AssertionError("invalid memory expiry was accepted")
    for provenance in (
            {"authorship": "invented"},
            {"authorship": "system_compaction", "explicit_user_evidence": True},
            {"source_thread_ids": ["api_key:secret"]},
            {"confidence": 1.1},
            {"explicit_user_evidence": "yes"},
    ):
        try:
            store.add("Reject invalid provenance", provenance=provenance)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid memory provenance was accepted")

with tempfile.TemporaryDirectory() as temp:
    legacy_path = Path(temp) / "legacy-secrets.json"
    secret_sentinel = "SPRINT1023_LEGACY_SECRET_SENTINEL_7f3a9c"
    legacy_records = [
        {
            "id": "legacy-content-secret",
            "content": f"provider api_key: {secret_sentinel}",
            "title": "Unsafe content",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": "legacy-thread",
            "tags": [],
        },
        {
            "id": "legacy-title-secret",
            "content": "Safe body",
            "title": f"password={secret_sentinel}",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": "legacy-thread",
            "tags": [],
        },
        {
            "id": "legacy-tag-secret",
            "content": "Safe body",
            "title": "Unsafe tag",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": "legacy-thread",
            "tags": [f"token:{secret_sentinel}"],
        },
        {
            "id": f"token:{secret_sentinel}",
            "content": "Safe body",
            "title": "Unsafe identifier",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": "legacy-thread",
            "tags": [],
        },
        {
            "id": "legacy-extra-field-secret",
            "content": "Safe body",
            "title": "Unsafe custom field",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": "legacy-thread",
            "tags": [],
            "metadata": {"credentials": {"api_key": secret_sentinel,
                                           "credential": "legacy-value"}},
        },
        {
            "id": "legacy-namespace-secret",
            "content": "Safe body",
            "title": "Unsafe namespace",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": f"api_key={secret_sentinel}",
            "tags": [],
        },
        {
            "id": "legacy-safe",
            "content": "Keep the verified 0.25 mm clearance.",
            "title": "Safe legacy preference",
            "scope": "conversation",
            "tier": "ltm",
            "namespace": "legacy-thread",
            "tags": ["pcb"],
        },
    ]
    legacy_path.write_text(json.dumps(legacy_records), encoding="utf-8")
    store = MemoryStore(legacy_path)
    original_bytes = legacy_path.read_bytes()

    visible = store.list(tier="ltm", namespace="legacy-thread")
    assert [entry["id"] for entry in visible] == ["legacy-safe"]
    assert store.ensure_namespace("ltm", "legacy-thread") == visible
    assert store.record_usage(
        ["legacy-content-secret", "legacy-title-secret", "legacy-tag-secret",
         f"token:{secret_sentinel}", "legacy-extra-field-secret",
         "legacy-namespace-secret"],
        used_at="2026-09-27T00:00:00+00:00") == {}
    assert store.update("legacy-title-secret", "safe replacement") is None
    assert store.update(f"token:{secret_sentinel}", "safe replacement") is None
    assert legacy_path.read_bytes() == original_bytes
    assert secret_sentinel not in json.dumps(visible)

    try:
        store.validate_compaction_sources(
            [legacy_records[0], legacy_records[-1]], tier="ltm",
            namespace="legacy-thread", scope="conversation")
    except RuntimeError as error:
        assert getattr(error, "category", "") == "memory_secret_record_excluded"
    else:
        raise AssertionError("secret-bearing legacy source entered compaction")

    store.add("A safe current preference", tier="ltm", namespace="legacy-thread",
              scope="conversation")
    after_write = json.loads(legacy_path.read_text(encoding="utf-8"))
    assert len(after_write) == len(legacy_records) + 1
    assert any(secret_sentinel in json.dumps(record) for record in after_write)
    assert all(secret_sentinel not in json.dumps(record) for record in store.list())

with tempfile.TemporaryDirectory() as temp:
    corrupt_path = Path(temp) / "memory.json"
    corrupt_path.write_text("{not valid JSON", encoding="utf-8")
    store = MemoryStore(corrupt_path)
    try:
        store.ensure_namespace("ltm", "thread-1")
    except RuntimeError as error:
        assert getattr(error, "category", "") == "memory_store_corrupt"
    else:
        raise AssertionError("corrupt memory store was reported as an empty namespace")
    try:
        store.add("do not replace damaged records", tier="ltm", namespace="thread-1")
    except RuntimeError as error:
        assert getattr(error, "category", "") == "memory_store_corrupt"
    else:
        raise AssertionError("memory add overwrote a corrupt backing store")
    assert corrupt_path.read_text(encoding="utf-8") == "{not valid JSON"

with tempfile.TemporaryDirectory() as temp:
    os.environ["APPDATA"] = temp
    os.environ.pop("CCAD_AGENT_MEMORY_PATH", None)
    assert MemoryStore().path == Path(temp) / "CCad" / "agent_memory.json"
print("PASS local memory store CRUD and secret rejection; no network")
