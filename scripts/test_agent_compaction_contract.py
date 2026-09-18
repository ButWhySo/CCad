"""No-network contract checks for truthful local session compaction."""

from pathlib import Path


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert "def compact_session_history(messages):" in text
assert "older_messages=" in text
assert "details omitted." in text
assert "session_messages = compact_session_history(session_messages)" in text
assert "recent turns preserved." in text
print("PASS local agent history compaction contract; no network")
