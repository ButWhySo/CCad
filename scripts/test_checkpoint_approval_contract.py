"""No-network contract: durable approval interrupts retain approval authority."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"kind": "ccad_tool_call", "tool": tool_name' in text
assert '"approval_required": True})' in text
print("PASS checkpointed tool interrupt preserves approval requirement; no network")
