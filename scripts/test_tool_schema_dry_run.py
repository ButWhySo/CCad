"""Prove provider-facing tool schemas expose dry_run; no network."""

import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
from orchestrator import (ui_add_polygon, ui_add_track, ui_place_footprint,
                          ui_place_symbol)  # noqa: E402

assert "dry_run" in ui_add_track.args_schema.model_fields
assert "dry_run" in ui_add_polygon.args_schema.model_fields
assert "dry_run" in ui_place_footprint.args_schema.model_fields
assert "dry_run" in ui_place_symbol.args_schema.model_fields
print("PASS provider tool schemas expose dry_run; no network")
