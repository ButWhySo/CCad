"""Contract check for the production provider context package; no network."""

import json
from pathlib import Path
import sys


ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from context_package import build_context_package


raw = json.dumps({
    "schema_version": 1,
    "context_kind": "ccad_agent_context",
    "revision": "native-revision",
    "project": {"project_id": "demo", "project_file": "demo.ccad.json",
                "has_board": True, "active_layer": "F.Cu", "active_net": "GND"},
    "constraints": {"read_only_by_default": True,
                    "approval_required_for_mutation": True,
                    "secret_values_excluded": True},
})
history = ["first prior turn", "second prior turn"]
memories = [
    {"id": "mem-good", "title": "Board intent", "scope": "project",
     "content": "Keep GND return path short.", "tags": ["layout"]},
    {"id": "mem-secret", "title": "Do not use", "scope": "project",
     "content": "api_key=fixture-not-a-real-secret", "tags": []},
]

turn_record = {
    "turn_id": "turn-source-1",
    "source_message_ids": ["message-user-1", "message-assistant-1"],
    "user_request_summary": "Inspect U3 on F.Cu",
    "assistant_summary": "U3 is on the front copper layer.",
    "outcome": "completed",
    "tool_ids": ["project.inspect"],
}
recap = {"source_turn_ids": ["turn-source-1"], "turns": [{
    **turn_record,
    "explicit_constraints": ["Keep U3 in place."],
    "referenced_entities": {"user_reference": ["U3"]},
    "important_findings": [],
}]}
package = build_context_package(raw, memories, history, char_limit=1300,
                                 turn_records=[turn_record], thread_recap=recap)
metadata = package["metadata"]
assert package["content"].startswith("[CCAD_CONTEXT_V3]\n")
assert metadata["schema_version"] == 3
assert metadata["project_revision"] == "native-revision"
assert len(metadata["package_digest"]) == 24
assert metadata["estimated_token_count"] == (metadata["content_size"] + 3) // 4
assert metadata["project_counts"] == {}
assert metadata["history_message_count"] == 2
assert metadata["memory_entry_count"] == 1
assert metadata["prior_turn_count"] == 1
assert metadata["thread_recap_turn_count"] == 1
assert "retrieved_conversation_turns" in metadata["sources"]
assert "thread_recap" in metadata["sources"]
assert metadata["secret_value_visible"] is False
assert metadata["content_emitted"] is False
assert "project_snapshot" in metadata["sources"]
assert "retrieved_memory" in metadata["sources"]
assert "fixture-not-a-real-secret" not in package["content"]
assert "message-user-1" in package["content"]
assert "Keep U3 in place." in package["content"]
assert len(package["content"]) <= 1300

truncated = build_context_package(raw + ("x" * 9000), [], [], char_limit=1024)
assert truncated["metadata"]["truncated"] is True
assert len(truncated["content"]) <= 1024
assert truncated["metadata"]["package_digest"] != metadata["package_digest"]

print("PASS agent context package contract; no network")
