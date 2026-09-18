"""No-network contract checks for bounded project context input."""

from pathlib import Path


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert 'CCAD_AGENT_CONTEXT_LIMIT' in text
assert 'def bound_context_text(context):' in text
assert 'if not isinstance(context, str):' in text
assert 'CCAD context truncated for provider safety' in text
assert 'context_str = bound_context_text(' in text
print("PASS agent project-context size boundary contract; no network")
