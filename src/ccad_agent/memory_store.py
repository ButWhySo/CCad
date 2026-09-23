"""Small local, provider-independent agent memory store.

Memory is user-owned JSON, never sent to a model automatically. Entries are
bounded, tagged, and deletable; credential-looking content is rejected.
"""

import json
import os
import re
import tempfile
import uuid
from datetime import datetime, timezone
from pathlib import Path


SECRET_MARKERS = re.compile(
    r"(api[_-]?key|secret|password|token)\s*[:=]\s*\S+", re.IGNORECASE
)


def memory_path() -> Path:
    configured = os.environ.get("CCAD_AGENT_MEMORY_PATH", "").strip()
    if configured:
        return Path(configured)
    app_data = os.environ.get("APPDATA")
    root = Path(app_data) / "CCad" if app_data else Path.home() / ".ccad"
    return root / "agent_memory.json"


class MemoryStore:
    def __init__(self, path=None):
        self.path = Path(path) if path else memory_path()

    def _read(self):
        if not self.path.exists():
            return []
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
            return data if isinstance(data, list) else []
        except (OSError, json.JSONDecodeError):
            return []

    def _write(self, entries):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        fd, name = tempfile.mkstemp(prefix=".ccad-memory-", dir=self.path.parent)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as handle:
                json.dump(entries, handle, indent=2, ensure_ascii=False)
                handle.write("\n")
            os.replace(name, self.path)
        finally:
            if os.path.exists(name):
                os.unlink(name)

    def add(self, content, *, title="", scope="project", tags=None, tier="ltm",
            namespace="project", expires_at=""):
        entry = self._normalise_entry(content, title=title, scope=scope, tags=tags,
                                      tier=tier, namespace=namespace,
                                      expires_at=expires_at)
        entries = self._read()
        entries.append(entry)
        self._write(entries)
        return entry

    @staticmethod
    def _normalise_entry(content, *, title="", scope="project", tags=None,
                         tier="ltm", namespace="project", expires_at=""):
        content = str(content or "").strip()
        if not content or len(content) > 8000:
            raise ValueError("memory content must contain 1..8000 characters")
        title = str(title or "").strip()[:200]
        scope = str(scope or "project").strip()[:100]
        tier = str(tier or "ltm").strip().lower()
        if tier not in {"stm", "ltm", "episodic"}:
            raise ValueError("memory tier must be stm, ltm, or episodic")
        namespace = str(namespace or "project").strip()[:160]
        clean_tags = [str(tag).strip()[:60] for tag in (tags or []) if str(tag).strip()][:20]
        if any(SECRET_MARKERS.search(value) for value in (content, title, scope, *clean_tags)):
            raise ValueError("memory content appears to contain a secret")
        entry = {
            "id": "mem-" + uuid.uuid4().hex,
            "title": title,
            "content": content,
            "scope": scope,
            "tier": tier,
            "namespace": namespace,
            "tags": clean_tags,
            "created_at": datetime.now(timezone.utc).isoformat(),
        }
        if expires_at:
            entry["expires_at"] = str(expires_at)[:40]
        return entry

    def update(self, entry_id, content, *, title="", scope="project", tags=None,
               tier="ltm", namespace="project", expires_at=""):
        entries = self._read()
        for index, current in enumerate(entries):
            if current.get("id") == entry_id:
                replacement = self._normalise_entry(content, title=title, scope=scope,
                                                    tags=tags, tier=tier,
                                                    namespace=namespace,
                                                    expires_at=expires_at)
                replacement["id"] = entry_id
                replacement["created_at"] = current.get("created_at", replacement["created_at"])
                entries[index] = replacement
                self._write(entries)
                return replacement
        return None

    def list(self, scope=None, *, tier=None, namespace=None):
        entries = self._read()
        return [item for item in entries
                if (scope is None or item.get("scope") == scope)
                and (tier is None or item.get("tier", "ltm") == tier)
                and (namespace is None or item.get("namespace", "project") == namespace)]

    def delete(self, entry_id):
        old = self._read()
        new = [item for item in old if item.get("id") != entry_id]
        if len(new) == len(old):
            return False
        self._write(new)
        return True

    def clear(self, scope=None):
        old = self._read()
        new = [item for item in old if scope is not None and item.get("scope") != scope]
        removed = len(old) - len(new)
        if removed:
            self._write(new)
        return removed

    def clear_tier(self, tier, namespace=None):
        old = self._read()
        new = [item for item in old if not (
            item.get("tier", "ltm") == tier and
            (namespace is None or item.get("namespace", "project") == namespace))]
        removed = len(old) - len(new)
        if removed:
            self._write(new)
        return removed
