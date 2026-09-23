"""Contract for uniform metadata on every native Agent catalog entry."""

from pathlib import Path

source = (Path(__file__).parents[1] / "src" / "ccad_gui" / "review_window.cpp").read_text(
    encoding="utf-8"
)
assert 'entry.insert("side_effect"' in source
assert 'entry.insert("context_requirements"' in source
assert 'entry.insert("result_shape"' in source
assert 'entry.insert("validation"' in source
assert 'entry.insert("examples"' in source
print("PASS native catalog metadata contract; no network")
