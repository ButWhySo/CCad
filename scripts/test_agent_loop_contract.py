"""No-network contract checks for the agent run safety boundary."""

from pathlib import Path


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert 'CCAD_AGENT_RECURSION_LIMIT' in text
assert 'run_config["recursion_limit"]' in text
assert 'min(32, max(4, recursion_limit))' in text
assert 'execute_tool", "supervisor"' in text
print("PASS agent loop recursion boundary contract; no network")
