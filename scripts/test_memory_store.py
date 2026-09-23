"""No-network memory CRUD, deletion, bounds, and secret-rejection proof."""

import tempfile
import sys
import os
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "ccad_agent"))
from memory_store import MemoryStore


with tempfile.TemporaryDirectory() as temp:
    store = MemoryStore(Path(temp) / "memory.json")
    item = store.add("Use 0.25 mm minimum track width", title="routing", tags=["pcb"],
                     tier="episodic", namespace="local-user", scope="user")
    assert store.list()[0]["id"] == item["id"]
    assert store.list(scope="other") == []
    updated = store.update(item["id"], "Use 0.30 mm minimum track width", title="updated")
    assert updated["id"] == item["id"]
    assert updated["tier"] == "episodic"
    assert updated["namespace"] == "local-user"
    assert updated["scope"] == "user"
    assert store.list()[0]["content"].startswith("Use 0.30")
    assert store.delete(item["id"]) is True
    assert store.list() == []
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
    for kwargs in ({"title": "api_key: hidden"}, {"tags": ["token: hidden"]}):
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

with tempfile.TemporaryDirectory() as temp:
    os.environ["APPDATA"] = temp
    os.environ.pop("CCAD_AGENT_MEMORY_PATH", None)
    assert MemoryStore().path == Path(temp) / "CCad" / "agent_memory.json"
print("PASS local memory store CRUD and secret rejection; no network")
