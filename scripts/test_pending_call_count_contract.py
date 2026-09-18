"""No-network contract: one correlated pending call counts once across stores."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert '"process_call_ids": process_calls' in text
assert '"checkpoint_call_ids": sorted(set(checkpoint_calls))' in text
assert '"count": len(set(process_calls).union(checkpoint_calls))' in text
assert '"count": len(process_calls) + len(set(checkpoint_calls))' not in text
print("PASS pending-call count deduplicates process/checkpoint IDs; no network")
