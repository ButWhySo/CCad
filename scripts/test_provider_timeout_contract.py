"""No-network contract checks for bounded provider request timeout."""

from pathlib import Path


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert 'CCAD_PROVIDER_TIMEOUT_SECONDS' in text
assert 'def provider_timeout_seconds():' in text
assert 'return min(120, max(1, value))' in text
assert '"timeout": provider_timeout_seconds()' in text
print("PASS provider timeout boundary contract; no network")
