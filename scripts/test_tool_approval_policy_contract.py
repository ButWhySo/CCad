"""No-network contract for centralized tool approval decisions."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'def tool_approval_decision(tool_name: str, args: dict):' in text
assert 'required = tool_name.startswith("ui.") and not dry_run' in text
assert 'tool_approval_decision("ui.place_via", args)["required"]' in text
assert 'tool_approval_decision("ui.route_track", args)["required"]' in text
assert 'tool_approval_decision("ui.add_zone", args)["required"]' in text
assert '"approval_reason": approval["reason"]' in text
assert '"approval_reason": "project_mutation"' in text
print("PASS centralized tool approval policy contract; no network")
