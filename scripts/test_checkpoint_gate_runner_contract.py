"""No-network contract for the optional multi-phase checkpoint gate."""

from pathlib import Path


text = (Path(__file__).parents[1] / "scripts" / "run_agent_contract_gate.ps1").read_text(encoding="utf-8")
assert 'if ($IncludeCheckpointRestart)' in text
assert 'CCAD_RESTART_PHASE = "first"' in text
assert 'foreach ($decision in @("second", "denial", "cancel"))' in text
assert '$checkpointTest' in text
assert 'PASS checkpoint restart branches: accept, denial, cancel' in text
print("PASS checkpoint gate runner supplies fresh DB and all decision phases; no network")
