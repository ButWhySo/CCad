"""Contract for lossless provider:model command parsing."""

from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'provider_name, separator, model_name = cmd_args.partition(":")' in source
assert 'if separator and provider_name.strip() and model_name.strip():' in source
assert 'cmd_args.split(":")' not in source
print("PASS provider:model parsing preserves delimiters in model IDs; no network")
