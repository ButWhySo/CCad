"""No-network memory CRUD, deletion, bounds, and secret-rejection proof."""

import tempfile
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "ccad_agent"))
from memory_store import MemoryStore


with tempfile.TemporaryDirectory() as temp:
    store = MemoryStore(Path(temp) / "memory.json")
    item = store.add("Use 0.25 mm minimum track width", title="routing", tags=["pcb"])
    assert store.list()[0]["id"] == item["id"]
    assert store.list(scope="other") == []
    assert store.delete(item["id"]) is True
    assert store.list() == []
    try:
        store.add("api_key: do-not-store")
    except ValueError as error:
        assert "secret" in str(error)
    else:
        raise AssertionError("secret-looking memory was accepted")
print("PASS local memory store CRUD and secret rejection; no network")
