"""Small local, provider-independent agent memory store.

Memory is user-owned JSON, never sent to a model automatically. Entries are
bounded, tagged, and deletable; credential-looking writes are rejected and
legacy credential-bearing records are excluded from every public read result.
"""

import hashlib
import json
import math
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
SECRET_FIELD_MARKERS = re.compile(
    r"api[_-]?key|secret|password|token|credential|authorization|"
    r"private[_-]?key|auth[_-]", re.IGNORECASE
)
_PROVENANCE_AUTHORS = {"user_authored", "auto_generated", "system_compaction", "unknown"}
_PROVENANCE_EVIDENCE = {"explicit_user_command", "memory_manager_ui",
                        "reviewed_compaction", "automatic_extraction", "legacy_unknown"}
_PROVENANCE_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._:-]{0,159}$")
_PROVENANCE_FIELDS = {"authorship", "explicit_user_evidence", "source_evidence_classes",
                      "source_thread_ids", "source_turn_ids", "source_event_ids",
                      "source_memory_ids", "confidence", "pinned"}


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
            try:
                item["provenance"] = self._normalise_provenance(
                    item.get("provenance"), legacy=item.get("provenance") is None)
            except ValueError as error:
                raise MemoryStoreError("memory_store_corrupt") from error
        return data

    @staticmethod
    def _normalise_provenance(value=None, *, legacy=False):
        if value is None:
            value = {"authorship": "unknown" if legacy else "unknown",
                     "source_evidence_classes": ["legacy_unknown"] if legacy else []}
        if not isinstance(value, dict) or set(value) - _PROVENANCE_FIELDS:
            raise ValueError("invalid memory provenance")
        authorship = value.get("authorship", "unknown")
        explicit = value.get("explicit_user_evidence", False)
        pinned = value.get("pinned", False)
        if authorship not in _PROVENANCE_AUTHORS or type(explicit) is not bool or type(pinned) is not bool:
            raise ValueError("invalid memory provenance")
        confidence = value.get("confidence")
        if confidence is not None and (isinstance(confidence, bool) or
                                       not isinstance(confidence, (int, float)) or
                                       not math.isfinite(confidence) or not 0 <= confidence <= 1):
            raise ValueError("invalid memory provenance")
        result = {"authorship": authorship, "explicit_user_evidence": explicit,
                  "source_evidence_classes": [], "source_thread_ids": [],
                  "source_turn_ids": [], "source_event_ids": [],
                  "source_memory_ids": [], "confidence": confidence, "pinned": pinned}
        for field in ("source_evidence_classes", "source_thread_ids", "source_turn_ids",
                      "source_event_ids", "source_memory_ids"):
            items = value.get(field, [])
            limit = 64 if field == "source_memory_ids" else 16
            if not isinstance(items, (list, tuple)) or len(items) > limit:
                raise ValueError("invalid memory provenance")
            allowed = _PROVENANCE_EVIDENCE if field == "source_evidence_classes" else None
            clean = []
            for item in items:
                if not isinstance(item, str) or not _PROVENANCE_ID.fullmatch(item):
                    raise ValueError("invalid memory provenance")
                if allowed is not None and item not in allowed:
                    raise ValueError("invalid memory provenance")
                if SECRET_MARKERS.search(item) or SECRET_FIELD_MARKERS.search(item):
                    raise ValueError("invalid memory provenance")
                if item not in clean:
                    clean.append(item)
            result[field] = clean
        if explicit and authorship != "user_authored":
            raise ValueError("invalid memory provenance")
        return result

    @classmethod
    def _merge_provenance(cls, existing, incoming):
        old = cls._normalise_provenance(existing)
        new = cls._normalise_provenance(incoming)
        merged = dict(old)
        for field in ("source_evidence_classes", "source_thread_ids", "source_turn_ids",
                      "source_event_ids", "source_memory_ids"):
            limit = 64 if field == "source_memory_ids" else 16
            merged[field] = list(dict.fromkeys(old[field] + new[field]))[:limit]
        merged["explicit_user_evidence"] = old["explicit_user_evidence"] or new["explicit_user_evidence"]
        if merged["explicit_user_evidence"]:
            merged["authorship"] = "user_authored"
        elif new["authorship"] != "unknown":
            merged["authorship"] = new["authorship"]
        merged["pinned"] = old["pinned"] or new["pinned"]
        if new["confidence"] is not None:
            merged["confidence"] = new["confidence"]
        return merged

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
                and entry.get("namespace", "project") == namespace
                and not self.contains_secret(entry)]

    def add(self, content, *, title="", scope="project", tags=None, tier="ltm", kind="fact",
            namespace="project", expires_at="", importance=3, provenance=None):
        if str(tier).strip().lower() in {"stm", "working_memory"}:
            raise ValueError("Working Memory is process-only and cannot be persisted")
        entry = self._normalise_entry(content, title=title, scope=scope, tags=tags,
                                      tier=tier, kind=kind, namespace=namespace,
                                      expires_at=expires_at, importance=importance,
                                      provenance=provenance)
        entries = self._read()
        entries.append(entry)
        self._write(entries)
        return entry

    @staticmethod
    def normalise(content, *, title="", scope="project", tags=None, tier="ltm", kind="fact",
                  namespace="project", expires_at="", importance=3, provenance=None):
        return MemoryStore._normalise_entry(
            content, title=title, scope=scope, tags=tags, tier=tier, kind=kind,
            namespace=namespace, expires_at=expires_at, importance=importance,
            provenance=provenance)

    @staticmethod
    def _normalise_entry(content, *, title="", scope="project", tags=None,
                         tier="ltm", kind="fact", namespace="project", expires_at="",
                         importance=3, provenance=None):
        content = str(content or "").strip()
        if not content or len(content) > 8000:
            raise ValueError("memory content must contain 1..8000 characters")
        title = str(title or "").strip()[:200]
        scope = str(scope or "project").strip()[:100]
        tier = str(tier or "ltm").strip().lower()
        if tier == "stm":
            tier = "working_memory"
        if tier not in {"working_memory", "ltm", "episodic"}:
            raise ValueError("memory tier must be working_memory, ltm, or episodic")
        kind = str(kind or "fact").strip().lower()
        if kind not in {"fact", "preference", "correction"}:
            raise ValueError("memory kind must be fact, preference, or correction")
        if (isinstance(importance, bool) or not isinstance(importance, int)
                or not 1 <= importance <= 5):
            raise ValueError("memory importance must be an integer from 1 to 5")
        namespace = str(namespace or "project").strip()[:160]
        clean_tags = [str(tag).strip()[:60] for tag in (tags or []) if str(tag).strip()][:20]
        if any(SECRET_MARKERS.search(value) for value in (
                content, title, scope, namespace, *clean_tags)):
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
            "provenance": MemoryStore._normalise_provenance(provenance),
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
               tier=None, namespace=None, expires_at=None, kind=None, importance=None,
               provenance=None, pinned=None):
        if SECRET_MARKERS.search(str(entry_id or "")):
            return None
        if pinned is not None and type(pinned) is not bool:
            raise ValueError("memory pinned state must be boolean")
        entries = self._read()
        for index, current in enumerate(entries):
            if current.get("id") == entry_id:
                if self.contains_secret(current):
                    return None
                replacement = self._normalise_entry(
                    content,
                    title=current.get("title", "") if title is None else title,
                    scope=current.get("scope", "project") if scope is None else scope,
                    tags=current.get("tags", []) if tags is None else tags,
                    tier=current.get("tier", "ltm") if tier is None else tier,
                    kind=current.get("kind", "fact") if kind is None else kind,
                    namespace=current.get("namespace", "project") if namespace is None else namespace,
                    expires_at=current.get("expires_at", "") if expires_at is None else expires_at,
                    importance=current.get("importance", 3) if importance is None else importance,
                    provenance=self._merge_provenance(current.get("provenance"), provenance)
                    if provenance is not None else current.get("provenance"))
                replacement["id"] = entry_id
                replacement["created_at"] = current.get("created_at", replacement["created_at"])
                replacement["updated_at"] = datetime.now(timezone.utc).isoformat()
                for field in ("last_used_at", "use_count"):
                    if field in current:
                        replacement[field] = current[field]
                replacement["provenance"] = self._merge_provenance(
                    current.get("provenance"), replacement.get("provenance"))
                if pinned is not None:
                    replacement["provenance"]["pinned"] = pinned
                entries[index] = replacement
                self._write(entries)
                return replacement
        return None

    def list(self, scope=None, *, tier=None, namespace=None):
        """Return only safe records; keep rejected legacy bytes untouched on disk."""
        entries = self._read()
        return [item for item in entries
                if (scope is None or item.get("scope") == scope)
                and (tier is None or item.get("tier", "ltm") == tier)
                and (namespace is None or item.get("namespace", "project") == namespace)
                and not self.contains_secret(item)]

    def record_usage(self, entry_ids, *, used_at=None):
        """Persist bounded retrieval-use metadata without changing memory content."""
        ids = {str(entry_id) for entry_id in entry_ids if str(entry_id)}
        if not ids:
            return {}
        timestamp = used_at or datetime.now(timezone.utc).isoformat()
        entries = self._read()
        updated = {}
        for entry in entries:
            if entry.get("id") not in ids or self.contains_secret(entry):
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
        if not isinstance(entry, dict):
            return True
        pending = [entry]
        visited = set()
        while pending:
            value = pending.pop()
            if isinstance(value, dict):
                identity = id(value)
                if identity in visited:
                    continue
                visited.add(identity)
                for key, child in value.items():
                    if SECRET_FIELD_MARKERS.search(str(key)):
                        return True
                    pending.append(child)
            elif isinstance(value, (list, tuple, set)):
                identity = id(value)
                if identity in visited:
                    continue
                visited.add(identity)
                pending.extend(value)
            elif SECRET_MARKERS.search(str(value or "")):
                return True
        return False

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
        stored_by_id = {str(entry.get("id", "")): entry for entry in entries}
        persisted_sources = [stored_by_id[source_id] for source_id in source_ids]

        replacement = self._normalise_entry(
            summary, title=title, scope=scope, tags=tags, tier=tier,
            namespace=namespace, expires_at=expires_at, provenance={
                "authorship": "system_compaction",
                "source_evidence_classes": ["reviewed_compaction"],
                "source_memory_ids": source_ids,
                "source_thread_ids": list(dict.fromkeys(
                    value for entry in persisted_sources for value in
                    entry.get("provenance", {}).get("source_thread_ids", [])))[:16],
                "source_turn_ids": list(dict.fromkeys(
                    value for entry in persisted_sources for value in
                    entry.get("provenance", {}).get("source_turn_ids", [])))[:16],
            })
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
            if self.contains_secret(source):
                raise MemoryStoreError("memory_secret_record_excluded")
            stored = current.get(str(source["id"]))
            if stored is not None and self.contains_secret(stored):
                raise MemoryStoreError("memory_secret_record_excluded")
            if (stored is None or stored.get("tier", "ltm") != tier
                    or stored.get("namespace", "project") != namespace
                    or stored.get("scope", "project") != scope
                    or self._compaction_fingerprint(stored) !=
                    self._compaction_fingerprint(source)):
                raise MemoryStoreError("memory_compaction_stale")
