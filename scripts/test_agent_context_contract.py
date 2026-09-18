"""No-network contract checks for bounded agent conversation context."""

from pathlib import Path


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert 'CCAD_AGENT_HISTORY_LIMIT' in text
assert 'def bound_session_history(messages):' in text
assert 'min(64, max(4, limit))' in text
assert 'session_messages = bound_session_history(final_state["messages"])' in text
print("PASS agent context history boundary contract; no network")
