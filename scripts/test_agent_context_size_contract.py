"""No-network contract checks for bounded project context input."""

from pathlib import Path


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
text = SOURCE.read_text(encoding="utf-8")

assert 'CCAD_AGENT_CONTEXT_LIMIT' in text
assert 'def bound_context_text(context):' in text
assert 'def agent_context_limit():' in text
assert 'if not isinstance(context, str):' in text
assert 'CCAD context truncated for provider safety' in text
assert 'context_str = package["content"]' in text
assert '"original_content_size": len(raw_context)' in text
assert '"truncated": context_truncated' in text
assert '"context_limit": context_metadata["context_limit"]' in text
print("PASS agent project-context size boundary contract; no network")
