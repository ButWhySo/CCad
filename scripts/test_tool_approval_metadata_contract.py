"""A dispatched mutating call carries its actual approval metadata."""

import json
import sys
from pathlib import Path
from unittest.mock import patch


root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
import orchestrator  # noqa: E402

events = []
original_wait = orchestrator.broker_wait_enabled
original_catalog = orchestrator.native_tool_catalog
orchestrator.broker_wait_enabled = False
orchestrator.native_tool_catalog = [{"method": "ui.add_zone", "read_only": False}]
try:
    with patch.object(orchestrator, "emit", side_effect=events.append):
        content, artifact = orchestrator.dispatch_client_tool_output(
            "ui.add_zone", {"start_x_mm": 0, "start_y_mm": 0, "dry_run": False})
finally:
    orchestrator.broker_wait_enabled = original_wait
    orchestrator.native_tool_catalog = original_catalog

call = next(event["params"] for event in events if event.get("method") == "tool_call")
assert call["tool"] == "ui.add_zone"
assert call["approval_required"] is True
assert call["approval_reason"] == "project_mutation"
assert json.loads(content)["error"] == "broker_wait_unavailable"
assert artifact == {}
print("PASS dispatched tool-call approval metadata; no network or project action")
