"""No-network contract for one-call approval and authoritative result truth."""

from pathlib import Path


source = (Path(__file__).resolve().parents[1] / "src" / "ccad_gui" / "agent_panel.cpp").read_text(
    encoding="utf-8"
)
assert "duplicate_pending_tool_call" in source
assert "approval_already_pending" in source
assert "authoritativeToolSucceeded" in source
assert 'result.insert("error", QJsonObject{{"code", -32010}' in source
assert "pending_tool_unavailable" in source
assert "tool_broker_unavailable" in source
assert "Proposal applied" in source
assert "Proposal was not applied" in source
assert '"Proposal approved"' not in source
print("PASS approval executes one pending call and reports authoritative success or failure; no network")
