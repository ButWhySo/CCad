"""Build bounded, secret-safe provider context from native CCad state."""

import hashlib
import json
import re
from typing import Any, Iterable


_SECRET = re.compile(
    r"(?:api[_-]?key|secret|password|token)\s*[:=]\s*\S+"
    r"|\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b"
    r"|\bAIza[A-Za-z0-9_-]{20,}", re.IGNORECASE)
_MARKER = "\n[CCAD context truncated for provider safety]\n"


def _safe_text(value: Any, limit: int) -> str:
    text = str(value or "").strip()
    if _SECRET.search(text):
        return ""
    return text[:max(0, limit)]


def _project_payload(raw_context: Any) -> tuple[Any, str]:
    text = str(raw_context or "").strip()
    if not text:
        return {}, ""
    try:
        decoded = json.loads(text)
    except json.JSONDecodeError:
        return {"native_context_text": _safe_text(text, 24000)}, ""
    if not isinstance(decoded, dict):
        return {"native_context_text": _safe_text(text, 24000)}, ""
    revision = str(decoded.get("revision") or "")
    return decoded, revision


def _memory_payload(entries: Iterable[dict], per_entry_limit: int = 1000) -> list[dict]:
    result = []
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        content = _safe_text(entry.get("content"), per_entry_limit)
        if not content:
            continue
        result.append({
            "id": _safe_text(entry.get("id"), 100),
            "title": _safe_text(entry.get("title"), 200),
            "scope": _safe_text(entry.get("scope", "project"), 100),
            "tags": [_safe_text(tag, 60) for tag in entry.get("tags", [])[:20]
                     if _safe_text(tag, 60)],
            "content": content,
        })
    return result


def build_context_package(raw_context: Any, memory_entries: Iterable[dict],
                          history: Iterable[Any], *, char_limit: int) -> dict:
    """Return actual provider content plus non-content metadata for one turn."""
    limit = min(131072, max(1024, int(char_limit)))
    project, native_revision = _project_payload(raw_context)
    memories = _memory_payload(memory_entries)
    history_items = list(history)
    envelope = {
        "schema_version": 2,
        "context_kind": "ccad_provider_context",
        "project": project,
        "memory": memories,
        "conversation": {"recent_message_count": len(history_items)},
        "constraints": {
            "read_only_by_default": True,
            "approval_required_for_mutation": True,
            "secret_values_excluded": True,
        },
    }
    encoded = "[CCAD_CONTEXT_V2]\n" + json.dumps(
        envelope, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    truncated = len(encoded) > limit
    if truncated:
        encoded = encoded[:max(0, limit - len(_MARKER))] + _MARKER
    # Conversation advances every turn; it must not invalidate a project/action
    # revision.  Fall back only to project and retrieved-memory identity.
    revision_material = json.dumps(
        {"project": project, "memory": memories}, ensure_ascii=False,
        sort_keys=True, separators=(",", ":"))
    revision = native_revision or hashlib.sha256(
        revision_material.encode("utf-8")).hexdigest()[:24]
    sources = []
    if project:
        sources.append("project_snapshot")
    if memories:
        sources.append("project_memory")
    if history_items:
        sources.append("recent_conversation")
    return {
        "content": encoded,
        "metadata": {
            "schema_version": 2,
            "project_revision": revision,
            "content_size": len(encoded),
            "context_limit": limit,
            "truncated": truncated,
            "sources": sources,
            "memory_entry_count": len(memories),
            "history_message_count": len(history_items),
            "content_emitted": False,
            "secret_value_visible": False,
        },
    }
