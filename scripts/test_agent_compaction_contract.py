"""Source-level guards for the production model/checkpoint integration."""

from pathlib import Path

ROOT = Path(__file__).parents[1]
source = (ROOT / "src" / "ccad_agent" / "orchestrator.py").read_text(
    encoding="utf-8")
helper = (ROOT / "src" / "ccad_agent" / "history_compaction.py").read_text(
    encoding="utf-8")
ui_harness = (ROOT / "src" / "ccad_gui" / "main.cpp").read_text(
    encoding="utf-8")
start = source.index("def compact_session_history(")
end = source.index("# --- Custom Workflows ---", start)
compaction = source[start:end]

assert "compact_history(messages, summarize, plan=plan)" in compaction
assert "model_client.invoke(" in compaction
assert "router_llm" not in compaction and "librarian_llm" not in compaction
assert '"context.compact", "generation"' in compaction
assert '"provider_request_sent": True' in compaction
assert "expected_message_ids=checkpoint_message_ids" in compaction
assert '"stage": "prepared", "provider_request_sent": False' in compaction
assert '"stage": "request_started", "provider_request_sent": True' in compaction
assert '"provider_request_sent": error.provider_request_sent' in source
assert "RemoveMessage(id=message.id)" in helper
assert "thread_has_pending_graph_work" in helper
assert "checkpoint_history_changed" in helper
assert "expected_checkpoint_id=checkpoint_id" in compaction
assert 'HistoryCompactionError("checkpoint_history_empty")' in compaction
assert "checkpoint_restore_failed" in helper
assert "while recent_start > 0" in helper
assert "details omitted." not in source
assert "Context compacted locally:" not in source
assert 'name.startsWith("sprint970-compaction")' in ui_harness
assert '"compact-command-result"' in ui_harness

print("PASS real provider-only compaction, no-tool model boundary, and checkpoint safety")
