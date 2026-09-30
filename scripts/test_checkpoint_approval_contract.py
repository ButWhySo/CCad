"""No-network contract: durable approval interrupts retain approval authority."""

from pathlib import Path


root = Path(__file__).parents[1]
text = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"kind": "ccad_tool_call", "tool": tool_name' in text
assert '"approval_required": approval["required"]' in text
assert '"approval_reason": approval["reason"]' in text
assert 'checkpoint_resume_value(' in text
assert 'checkpointed_tool_output(decision)' in text
assert 'response_format="content_and_artifact"' in text
assert '"audit": req.get("audit")' in text

panel = (root / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
for decision in ('"approved"', '"rejected"', '"cancelled"'):
    assert f'makeAgentApprovalMetadata(' in panel
    assert decision in panel
assert 'result.insert("audit", approval_audit)' in panel
assert 'pending_proposal_id_ = QUuid::createUuid()' in panel
print("PASS Qt approval IDs/decisions traverse standard and checkpointed tool output as private artifacts")
