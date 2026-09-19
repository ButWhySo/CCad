"""Static contract: approval pane appears only for pending changes; no GUI launch."""

from pathlib import Path

source = (Path(__file__).parents[1] / "src" / "ccad_gui" / "agent_panel.cpp").read_text(encoding="utf-8")
assert "approval_card->hide();" in source
assert "if (approval_preview_) approval_preview_->show();" in source
assert "if (approval_preview_) approval_preview_->hide();" in source
assert 'contains("\\\"error\\\":\\\"approval_required\\\"")' in source
print("PASS approval pane visibility contract; no GUI launch")
