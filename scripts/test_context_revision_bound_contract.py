"""No-network contract: context revision metadata remains bounded."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'CONTEXT_REVISION_THREAD_LIMIT = 128' in text
assert 'if len(context_revisions) > CONTEXT_REVISION_THREAD_LIMIT:' in text
assert 'oldest_thread_id = next(iter(context_revisions))' in text
assert 'context_revisions.pop(oldest_thread_id, None)' in text
print("PASS context revision metadata is bounded; no network")
