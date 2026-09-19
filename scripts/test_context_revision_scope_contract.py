"""No-network contract: context revisions are isolated by agent thread."""

from pathlib import Path


text = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
assert 'context_revisions = {}' in text
assert 'requested_thread = str(params.get("thread_id") or' in text
assert 'context_thread_id = requested_thread' in text
assert 'previous_context_revision = context_revisions.get(context_thread_id, "")' in text
assert 'context_revisions[context_thread_id] = current_context_revision' in text
assert 'previous_context_revision = last_context_revision' not in text
print("PASS context revision is thread-scoped; no network")
