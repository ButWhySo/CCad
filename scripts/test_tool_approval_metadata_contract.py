"""No-network contract: mutating tool calls expose approval requirement."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"approval_required": bool(await_result and approval["required"])' in text
assert 'tool_approval_decision("ui.place_via", args)["required"]' in text
assert 'tool_approval_decision("ui.place_via", args)["reason"]' in text
assert 'def ui_add_track(x1: float, y1: float, x2: float, y2: float, dry_run: bool = False)' in text
assert 'if broker_wait_enabled and not dry_run:' in text
assert 'def ui_add_polygon(points: List[List[float]], layer: str, dry_run: bool = False)' in text
assert '"approval_required",\n                    "approval_reason"]' in text
assert '"approval_required": True' in text
assert '"tool", "args", "call_id", "approval_required"' in text
print("PASS tool approval metadata is explicit and dry-run aware; no network")
