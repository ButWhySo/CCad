"""Small local, provider-independent agent memory store.

Memory is user-owned JSON, never sent to a model automatically. Entries are
bounded, tagged, and deletable; credential-looking content is rejected.
"""

import hashlib
import json
import os
import re
import tempfile
import uuid
from datetime import datetime, timezone
from pathlib import Path


SECRET_MARKERS = re.compile(
    r"(api[_-]?key|secret|password|token)\s*[:=]\s*\S+|"
    r"\bsk-[A-Za-z0-9_-]{12,}\b|\bgh[pousr]_[A-Za-z0-9_]{20,}\b|"
    r"\bBearer\s+[A-Za-z0-9._~-]{12,}|\bAIza[0-9A-Za-z_-]{30,}\b|"
    r"\bya29\.[0-9A-Za-z_-]{20,}", re.IGNORECASE
)


class MemoryStoreError(RuntimeError):
    """Safe storage failure category without exposing paths or stored values."""

    def __init__(self, category):
        self.category = str(category)
        super().__init__(self.category)


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
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
        except FileNotFoundError:
            return []
        except json.JSONDecodeError as error:
            raise MemoryStoreError("memory_store_corrupt") from error
        except OSError as error:
            raise MemoryStoreError("memory_store_unavailable") from error
        if not isinstance(data, list) or any(not isinstance(item, dict) for item in data):
            raise MemoryStoreError("memory_store_corrupt")
        for item in data:
            item.setdefault("kind", "fact")
            if item["kind"] not in {"fact", "preference", "correction"}:
                raise MemoryStoreError("memory_store_corrupt")
            item.setdefault("importance", 3)
            if (isinstance(item["importance"], bool) or
                    not isinstance(item["importance"], int) or
                    not 1 <= item["importance"] <= 5):
                raise MemoryStoreError("memory_store_corrupt")
        return data

    def _write(self, entries):
        name = ""
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            fd, name = tempfile.mkstemp(prefix=".ccad-memory-", dir=self.path.parent)
            with os.fdopen(fd, "w", encoding="utf-8") as handle:
                json.dump(entries, handle, indent=2, ensure_ascii=False)
                handle.write("\n")
            os.replace(name, self.path)
        except OSError as error:
            raise MemoryStoreError("memory_store_unavailable") from error
        finally:
            if name and os.path.exists(name):
                os.unlink(name)

    def ensure_namespace(self, tier, namespace):
        """Open the single JSON backing store and validate this logical namespace."""
        if tier not in {"ltm", "episodic"}:
            raise ValueError("only durable memory tiers have persistent namespaces")
        namespace = str(namespace or "").strip()
        if not namespace:
            raise ValueError("memory namespace identity is required")
        entries = self._read()
        if not self.path.is_file():
            self._write(entries)
        return [entry for entry in entries
                if entry.get("tier", "ltm") == tier
                and entry.get("namespace", "project") == namespace]

    def add(self, content, *, title="", scope="project", tags=None, tier="ltm", kind="fact",
            namespace="project", expires_at="", importance=3):
        entry = self._normalise_entry(content, title=title, scope=scope, tags=tags,
                                      tier=tier, kind=kind, namespace=namespace,
                                      expires_at=expires_at, importance=importance)
        entries = self._read()
        entries.append(entry)
        self._write(entries)
        return entry

    @staticmethod
    def normalise(content, *, title="", scope="project", tags=None, tier="ltm", kind="fact",
                  namespace="project", expires_at="", importance=3):
        return MemoryStore._normalise_entry(
            content, title=title, scope=scope, tags=tags, tier=tier, kind=kind,
            namespace=namespace, expires_at=expires_at, importance=importance)

    @staticmethod
    def _normalise_entry(content, *, title="", scope="project", tags=None,
                         tier="ltm", kind="fact", namespace="project", expires_at="",
                         importance=3):
        content = str(content or "").strip()
        if not content or len(content) > 8000:
            raise ValueError("memory content must contain 1..8000 characters")
        title = str(title or "").strip()[:200]
        scope = str(scope or "project").strip()[:100]
        tier = str(tier or "ltm").strip().lower()
        if tier not in {"stm", "ltm", "episodic"}:
            raise ValueError("memory tier must be stm, ltm, or episodic")
        kind = str(kind or "fact").strip().lower()
        if kind not in {"fact", "preference", "correction"}:
            raise ValueError("memory kind must be fact, preference, or correction")
        if (isinstance(importance, bool) or not isinstance(importance, int)
                or not 1 <= importance <= 5):
            raise ValueError("memory importance must be an integer from 1 to 5")
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
            "kind": kind,
            "importance": importance,
            "namespace": namespace,
            "tags": clean_tags,
            "created_at": datetime.now(timezone.utc).isoformat(),
        }
        if expires_at:
            try:
                expiry = datetime.fromisoformat(str(expires_at).replace("Z", "+00:00"))
            except ValueError as error:
                raise ValueError("memory expiry must be an ISO-8601 timestamp") from error
            if expiry.tzinfo is None:
                raise ValueError("memory expiry must include a timezone")
            entry["expires_at"] = expiry.astimezone(timezone.utc).isoformat()
        return entry

    def update(self, entry_id, content, *, title=None, scope=None, tags=None,
               tier=None, namespace=None, expires_at=None, kind=None, importance=None):
        entries = self._read()
        for index, current in enumerate(entries):
            if current.get("id") == entry_id:
                replacement = self._normalise_entry(
                    content,
                    title=current.get("title", "") if title is None else title,
                    scope=current.get("scope", "project") if scope is None else scope,
                    tags=current.get("tags", []) if tags is None else tags,
                    tier=current.get("tier", "ltm") if tier is None else tier,
                    kind=current.get("kind", "fact") if kind is None else kind,
                    namespace=current.get("namespace", "project") if namespace is None else namespace,
                    expires_at=current.get("expires_at", "") if expires_at is None else expires_at,
                    importance=current.get("importance", 3) if importance is None else importance)
                replacement["id"] = entry_id
                replacement["created_at"] = current.get("created_at", replacement["created_at"])
                replacement["updated_at"] = datetime.now(timezone.utc).isoformat()
                for field in ("last_used_at", "use_count"):
                    if field in current:
                        replacement[field] = current[field]
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

    def record_usage(self, entry_ids, *, used_at=None):
        """Persist bounded retrieval-use metadata without changing memory content."""
        ids = {str(entry_id) for entry_id in entry_ids if str(entry_id)}
        if not ids:
            return {}
        timestamp = used_at or datetime.now(timezone.utc).isoformat()
        entries = self._read()
        updated = {}
        for entry in entries:
            if entry.get("id") not in ids:
                continue
            count = entry.get("use_count", 0)
            count = count if isinstance(count, int) and not isinstance(count, bool) else 0
            entry["use_count"] = min(1_000_000, max(0, count) + 1)
            entry["last_used_at"] = str(timestamp)
            updated[entry["id"]] = entry
        if updated:
            self._write(entries)
        return updated

    @staticmethod
    def contains_secret(entry):
        return any(SECRET_MARKERS.search(str(value or "")) for value in (
            entry.get("content", ""), entry.get("title", ""),
            entry.get("scope", ""), *entry.get("tags", [])))

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

    def clear_scope(self, scope, *, tier=None, namespace=None):
        old = self._read()
        new = [item for item in old if not (
            item.get("scope") == scope and
            (tier is None or item.get("tier", "ltm") == tier) and
            (namespace is None or item.get("namespace", "project") == namespace))]
        removed = len(old) - len(new)
        if removed:
            self._write(new)
        return removed

    def keep_latest(self, tier, namespace, limit):
        entries = self._read()
        selected = [item for item in entries if item.get("tier", "ltm") == tier
                    and item.get("namespace", "project") == namespace]
        selected.sort(key=lambda item: item.get("created_at", ""), reverse=True)
        keep_ids = {item.get("id") for item in selected[:max(1, int(limit))]}
        new = [item for item in entries if item.get("tier", "ltm") != tier
               or item.get("namespace", "project") != namespace
               or item.get("id") in keep_ids]
        removed = len(entries) - len(new)
        if removed:
            self._write(new)
        return removed

    @staticmethod
    def _compaction_fingerprint(entry):
        fields = {key: entry.get(key) for key in (
            "id", "title", "content", "scope", "tier", "namespace",
            "tags", "created_at", "expires_at") if key in entry}
        return hashlib.sha256(json.dumps(
            fields, ensure_ascii=False, sort_keys=True,
            separators=(",", ":")).encode("utf-8")).hexdigest()

    def replace_with_compaction(self, source_entries, summary, *, tier,
                                namespace, scope, title, tags, expires_at=""):
        """Atomically replace unchanged durable records with one reviewed summary."""
        if tier not in {"ltm", "episodic"}:
            raise ValueError("only durable memory tiers can be compacted")
        namespace, scope = str(namespace), str(scope)
        sources = list(source_entries)
        source_ids = [str(entry.get("id", "")) for entry in sources]
        if len(sources) < 2 or not all(source_ids) or len(set(source_ids)) != len(source_ids):
            raise ValueError("memory compaction requires distinct durable source records")
        if any(entry.get("tier") != tier or entry.get("namespace") != namespace
               or entry.get("scope") != scope for entry in sources):
            raise ValueError("memory compaction source scope changed")

        entries = self._read()
        self._validate_compaction_sources(entries, sources, tier, namespace, scope)

        replacement = self._normalise_entry(
            summary, title=title, scope=scope, tags=tags, tier=tier,
            namespace=namespace, expires_at=expires_at)
        source_id_set = set(source_ids)
        updated = [entry for entry in entries
                   if str(entry.get("id", "")) not in source_id_set]
        updated.append(replacement)
        self._write(updated)
        return replacement

    def validate_compaction_sources(self, source_entries, *, tier, namespace, scope):
        """Refuse to export a reviewed snapshot after its durable records change."""
        sources = list(source_entries)
        if (len(sources) < 2 or any(
                entry.get("tier") != tier or entry.get("namespace") != namespace
                or entry.get("scope") != scope for entry in sources)):
            raise MemoryStoreError("memory_compaction_stale")
        self._validate_compaction_sources(self._read(), sources, tier, namespace, scope)

    def _validate_compaction_sources(self, entries, sources, tier, namespace, scope):
        source_ids = [str(entry.get("id", "")) for entry in sources]
        if len(sources) < 2 or not all(source_ids) or len(set(source_ids)) != len(source_ids):
            raise MemoryStoreError("memory_compaction_stale")
        current = {str(entry.get("id", "")): entry for entry in entries}
        for source in sources:
            stored = current.get(str(source["id"]))
            if (stored is None or stored.get("tier", "ltm") != tier
                    or stored.get("namespace", "project") != namespace
                    or stored.get("scope", "project") != scope
                    or self._compaction_fingerprint(stored) !=
                    self._compaction_fingerprint(source)):
                raise MemoryStoreError("memory_compaction_stale")
