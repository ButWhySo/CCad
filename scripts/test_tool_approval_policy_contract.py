"""Runtime contract: read-only UI methods bypass approval; mutations do not."""

import sys
from pathlib import Path


root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
import orchestrator  # noqa: E402

schema = {"type": "object", "properties": {}, "additionalProperties": False}
original_catalog = orchestrator.native_tool_catalog
orchestrator.native_tool_catalog = orchestrator.validate_native_tool_catalog([
    {"method": method, "description": method, "read_only": read_only,
     "inputSchema": schema}
    for method, read_only in (("ui.map", True), ("ui.add_zone", False),
                              ("ui.place_via", False))])
try:
    assert orchestrator.tool_approval_decision("ui.map", {}) == {
        "required": False, "reason": "read_only"}
    assert orchestrator.tool_approval_decision("ui.add_zone", {}) == {
        "required": True, "reason": "project_mutation"}
    assert orchestrator.tool_approval_decision("ui.place_via", {"dry_run": True}) == {
        "required": False, "reason": "dry_run"}
    assert orchestrator.tool_approval_decision("ui.unknown", {}) == {
        "required": True, "reason": "project_mutation"}
    assert orchestrator.tool_approval_decision("project.drc", {}) == {
        "required": False, "reason": "not_mutating"}
    assert orchestrator.tool_calls_require_approval([
        {"name": orchestrator.tool_provider_name("ui.map"), "args": {}}]) is False
    assert orchestrator.tool_calls_require_approval([
        {"name": orchestrator.tool_provider_name("ui.add_zone"), "args": {}}]) is True
    assert orchestrator.tool_calls_require_approval([
        {"name": orchestrator.tool_provider_name("ui.add_zone"),
         "args": {"dry_run": True}}]) is False
finally:
    orchestrator.native_tool_catalog = original_catalog
print("PASS centralized tool approval policy contract; no network")
