"""Runtime memory tiers over the durable, secret-rejecting MemoryStore."""

from __future__ import annotations

import hashlib
import re
from collections import OrderedDict
from datetime import datetime, timezone
from typing import Any
from uuid import uuid4

from memory_store import MemoryStore, MemoryStoreError


class MemoryManager:
    TIERS = ("stm", "ltm", "episodic")
    MAX_STM_TASKS = 32
    NEAR_DUPLICATE_THRESHOLD = 0.88
    _word = re.compile(r"[a-z0-9_]{3,}", re.IGNORECASE)

    def __init__(self, store: MemoryStore, *, task_id="ccad-task", thread_id="ccad-local",
                 project_id="project", user_id="local-user"):
        self.store = store
        self.identities = {"stm": str(task_id), "ltm": str(thread_id),
                           "episodic": str(user_id)}
        self.project_id = str(project_id)
        self.enabled = {tier: False for tier in self.TIERS}
        self.runtime: dict[str, list[dict[str, Any]]] = {tier: [] for tier in self.TIERS}
        self.storage_errors: dict[str, str] = {}
        self._stm_tasks: OrderedDict[str, list[dict[str, Any]]] = OrderedDict()
        self._retain_stm_task = True

    def set_identities(self, *, task_id: str, thread_id: str, project_id: str,
                       user_id="local-user", retain_stm_task=True):
        identities = {"stm": str(task_id), "ltm": str(thread_id),
                      "episodic": str(user_id)}
        changed = {tier for tier in self.TIERS
                   if identities[tier] != self.identities[tier]}
        self.identities = identities
        self.project_id = str(project_id)
        self._retain_stm_task = bool(retain_stm_task)
        if not self._retain_stm_task:
            self.runtime["stm"] = []
        for tier in changed:
            if self.enabled[tier]:
                try:
                    loaded = self._load(tier)
                except MemoryStoreError as error:
                    self.enabled[tier] = False
                    self.runtime[tier] = []
                    self.storage_errors[tier] = error.category
                else:
                    self.runtime[tier] = loaded
                    self.storage_errors.pop(tier, None)
            else:
                self.runtime[tier] = []

    @staticmethod
    def _now():
        return datetime.now(timezone.utc)

    def configure(self, flags: dict[str, Any] | None):
        flags = flags if isinstance(flags, dict) else {}
        failures = {}
        for tier in self.TIERS:
            try:
                if bool(flags.get(tier, False)):
                    self.enable(tier)
                else:
                    self.disable(tier)
            except MemoryStoreError as error:
                failures[tier] = error.category
                self.enabled[tier] = False
                self.runtime[tier] = []
                self.storage_errors[tier] = error.category
        return failures

    def enable(self, tier: str):
        self._check_tier(tier)
        if tier != "stm":
            self.store.ensure_namespace(tier, self.identities[tier])
        loaded = self._load(tier)
        self.runtime[tier] = loaded
        self.enabled[tier] = True
        self.storage_errors.pop(tier, None)
        return self.state(tier)

    def disable(self, tier: str):
        self._check_tier(tier)
        self.enabled[tier] = False
        self.runtime[tier] = []
        if tier == "stm":
            self._stm_tasks.clear()
        return self.state(tier)

    def _load(self, tier: str):
        if tier == "stm":
            if not self._retain_stm_task:
                return []
            namespace = self.identities[tier]
            entries = self._stm_tasks.pop(namespace, [])
            self._stm_tasks[namespace] = entries
            while len(self._stm_tasks) > self.MAX_STM_TASKS:
                self._stm_tasks.popitem(last=False)
            return entries
        now = self._now()
        entries = []
        expired = []
        for entry in self.store.list(tier=tier, namespace=self.identities[tier]):
            if self.store.contains_secret(entry):
                continue
            expires = entry.get("expires_at", "")
            if expires:
                try:
                    if datetime.fromisoformat(expires.replace("Z", "+00:00")) <= now:
                        expired.append(entry["id"])
                        continue
                except ValueError:
                    pass
            entries.append(entry)
        for entry_id in expired:
            self.store.delete(entry_id)
        return entries[-64:]

    def retrieve(self, query: str, *, limit=8):
        entries, _ = self.retrieve_with_metadata(query, limit=limit)
        return entries

    def retrieve_with_metadata(self, query: str, *, limit=8):
        """Return ranked entries plus content-free provenance for diagnostics."""
        self._prune_expired()
        candidates = []
        query_words = set(self._word.findall(str(query).lower()))
        for tier_index, tier in enumerate(self.TIERS):
            if not self.enabled[tier] or self.storage_errors.get(tier):
                continue
            for entry_index, entry in enumerate(self.runtime[tier]):
                text = f"{entry.get('title', '')} {entry.get('content', '')}".lower()
                overlap = len(query_words.intersection(self._word.findall(text)))
                if query_words and overlap == 0:
                    continue
                recency = entry.get("created_at", "")
                candidates.append((overlap, recency, -tier_index, entry_index,
                                   tier, entry))
        candidates.sort(key=lambda item: item[:4], reverse=True)
        selected = candidates[:max(0, min(32, int(limit)))]
        entries = [item[5] for item in selected]
        provenance = [{
            "entry_id": item[5]["id"],
            "rank": index + 1,
            "tier": item[4],
            "query_overlap_terms": item[0],
            "namespace_hash": hashlib.sha256(
                self.identities[item[4]].encode()).hexdigest()[:16],
        } for index, item in enumerate(selected)]
        return entries, provenance

    def context_entries(self, query: str):
        return self.retrieve(query, limit=8)

    def add(self, content: str, *, tier="ltm", title="", scope=None, tags=None,
            expires_at=""):
        self._check_tier(tier)
        if not self.enabled[tier]:
            raise RuntimeError(f"memory tier disabled: {tier}")
        if tier == "stm" and not self._retain_stm_task:
            raise RuntimeError("STM requires an active task; use /task start")
        normalized = " ".join(str(content).casefold().split())
        existing = next((item for item in self.list(tier=tier)
                         if " ".join(str(item.get("content", "")).casefold().split()) == normalized), None)
        if existing:
            return existing
        duplicate = self._near_duplicate(content, tier)
        if duplicate:
            entry, similarity = duplicate
            raise ValueError(
                f"near-duplicate memory exists ({entry['id']}, lexical overlap "
                f"{similarity:.0%}); update that record or add distinct information")
        scope = str(scope or {"stm": "task", "ltm": "conversation",
                              "episodic": "user"}[tier])
        entry = self.store.normalise(content, title=title, scope=scope, tags=tags,
                                     tier=tier, namespace=self.identities[tier],
                                     expires_at=expires_at)
        entry["project_id"] = self.project_id
        if tier != "stm":
            entry = self.store.add(content, title=title, scope=scope, tags=tags,
                                   tier=tier, namespace=self.identities[tier],
                                   expires_at=expires_at)
            self.store.keep_latest(tier, self.identities[tier], 64)
            entry["project_id"] = self.project_id
        self.runtime[tier].append(entry)
        self.runtime[tier] = self.runtime[tier][-64:]
        if tier == "stm":
            self._stm_tasks[self.identities[tier]] = self.runtime[tier]
            self._stm_tasks.move_to_end(self.identities[tier])
            while len(self._stm_tasks) > self.MAX_STM_TASKS:
                self._stm_tasks.popitem(last=False)
        return entry

    def _near_duplicate(self, content: str, tier: str, *, exclude_id=""):
        words = set(self._word.findall(str(content).casefold()))
        if len(words) < 5:
            return None
        best = None
        for entry in self.list(tier=tier):
            if entry.get("id") == exclude_id:
                continue
            existing = set(self._word.findall(
                str(entry.get("content", "")).casefold()))
            if len(existing) < 5:
                continue
            similarity = len(words & existing) / len(words | existing)
            if similarity >= self.NEAR_DUPLICATE_THRESHOLD and (
                    best is None or similarity > best[1]):
                best = (entry, similarity)
        return best

    def list(self, *, tier=None, scope=None):
        self._prune_expired()
        tiers = self.TIERS if tier is None else (tier,)
        entries = []
        for current in tiers:
            self._check_tier(current)
            durable = ([] if current == "stm" else [item for item in self.store.list(
                tier=current, namespace=self.identities[current], scope=scope)
                if not self.store.contains_secret(item)])
            runtime = [item for item in self.runtime[current]
                       if scope is None or item.get("scope") == scope]
            by_id = {item.get("id"): item for item in durable}
            by_id.update((item.get("id"), item) for item in runtime)
            entries.extend(by_id.values())
        return entries

    def _prune_expired(self):
        now = self._now()
        for tier in self.TIERS:
            namespace = self.identities[tier]
            candidates = list(self.runtime[tier])
            if tier != "stm":
                try:
                    candidates.extend(self.store.list(tier=tier, namespace=namespace))
                    self.storage_errors.pop(tier, None)
                except MemoryStoreError as error:
                    self.storage_errors[tier] = error.category
                    self.enabled[tier] = False
                    self.runtime[tier] = []
                    continue
            expired_ids = set()
            for entry in candidates:
                value = entry.get("expires_at", "")
                if not value:
                    continue
                try:
                    expired = datetime.fromisoformat(value.replace("Z", "+00:00")) <= now
                except (TypeError, ValueError):
                    expired = True
                if expired:
                    expired_ids.add(entry.get("id"))
            if expired_ids:
                self.runtime[tier] = [item for item in self.runtime[tier]
                                      if item.get("id") not in expired_ids]
                if tier == "stm":
                    for namespace, entries in list(self._stm_tasks.items()):
                        self._stm_tasks[namespace] = [
                            item for item in entries
                            if item.get("id") not in expired_ids]
                for entry_id in expired_ids:
                    if entry_id and tier != "stm":
                        self.store.delete(entry_id)

    def update(self, entry_id, content, *, title=None, scope=None, tags=None,
               expires_at=None):
        for tier in self.TIERS:
            if not self.enabled[tier]:
                if tier != "stm" and any(
                        item.get("id") == entry_id for item in self.store.list(
                            tier=tier, namespace=self.identities[tier])):
                    raise RuntimeError(f"memory tier disabled: {tier}")
                continue
            entry = next((item for item in self.list(tier=tier)
                          if item.get("id") == entry_id), None)
            if entry is None:
                continue
            duplicate = self._near_duplicate(content, tier, exclude_id=entry_id)
            if duplicate:
                other, similarity = duplicate
                raise ValueError(
                    f"near-duplicate memory exists ({other['id']}, lexical overlap "
                    f"{similarity:.0%}); revise to distinct information")
            fields = {"title": entry.get("title", "") if title is None else title,
                      "scope": entry.get("scope", "project") if scope is None else scope,
                      "tags": entry.get("tags", []) if tags is None else tags,
                      "tier": tier, "namespace": self.identities[tier],
                      "expires_at": entry.get("expires_at", "") if expires_at is None else expires_at}
            if tier == "stm":
                replacement = self.store.normalise(content, **fields)
                replacement.update(id=entry_id, project_id=self.project_id,
                                   created_at=entry.get("created_at", ""))
            else:
                replacement = self.store.update(entry_id, content, **fields)
                if replacement is None:
                    return None
                replacement["project_id"] = self.project_id
            self.runtime[tier] = [replacement if item.get("id") == entry_id else item
                                  for item in self.runtime[tier]]
            if tier == "stm":
                self._stm_tasks[self.identities[tier]] = self.runtime[tier]
            return replacement
        return None

    def delete(self, entry_id):
        for tier in self.TIERS:
            matching = any(item.get("id") == entry_id for item in self.list(tier=tier))
            if matching:
                self.runtime[tier] = [item for item in self.runtime[tier]
                                      if item.get("id") != entry_id]
                if tier == "stm":
                    for namespace, entries in list(self._stm_tasks.items()):
                        self._stm_tasks[namespace] = [
                            item for item in entries
                            if item.get("id") != entry_id]
                return tier == "stm" or self.store.delete(entry_id)
        # A memory row may remain visible while a conversation/thread switch
        # occurs inside the confirmation dialog's nested event loop. The
        # selected ID is globally unique, so honor that explicit delete even
        # when its old thread namespace is no longer the active one.
        stored_entry = next((item for item in self.store.list()
                             if item.get("id") == entry_id), None)
        if stored_entry is None:
            return False
        tier = str(stored_entry.get("tier", "ltm"))
        if tier not in self.TIERS:
            return False
        self.runtime[tier] = [item for item in self.runtime[tier]
                              if item.get("id") != entry_id]
        if tier == "stm":
            for namespace, entries in list(self._stm_tasks.items()):
                self._stm_tasks[namespace] = [
                    item for item in entries if item.get("id") != entry_id]
            return True
        return self.store.delete(entry_id)

    def clear_scope(self, scope, *, tier=None):
        tiers = self.TIERS if tier is None else (tier,)
        removed = 0
        for current in tiers:
            self._check_tier(current)
            if current == "stm":
                runtime_matches = {item.get("id") for entries in self._stm_tasks.values()
                                   for item in entries if item.get("scope") == scope}
                for namespace, entries in list(self._stm_tasks.items()):
                    self._stm_tasks[namespace] = [
                        item for item in entries if item.get("scope") != scope]
            else:
                runtime_matches = {item.get("id") for item in self.runtime[current]
                                   if item.get("scope") == scope}
            persistent_matches = set() if current == "stm" else {
                item.get("id") for item in self.store.list(
                    tier=current, namespace=self.identities[current], scope=scope)}
            self.runtime[current] = [item for item in self.runtime[current]
                                     if item.get("scope") != scope]
            if current != "stm":
                removed += self.store.clear_scope(scope, tier=current,
                                                  namespace=self.identities[current])
            removed += len(runtime_matches - persistent_matches)
        return removed

    def reset(self, tier=None, *, persistent=True):
        tiers = self.TIERS if tier is None else (tier,)
        removed = 0
        for current in tiers:
            self._check_tier(current)
            runtime_ids = {item.get("id") for item in self.runtime[current]}
            self.runtime[current] = []
            if current == "stm":
                runtime_ids.update(item.get("id") for entries in self._stm_tasks.values()
                                   for item in entries)
                self._stm_tasks.clear()
            if persistent:
                stored_ids = {item.get("id") for item in self.store.list(tier=current)}
                removed += self.store.clear_tier(current, None)
                removed += len(runtime_ids - stored_ids)
            else:
                removed += len(runtime_ids)
        return removed

    def clear_task(self, task_id: str):
        """Discard one process-only STM task without touching durable tiers."""
        task_id = str(task_id)
        entries = self._stm_tasks.pop(task_id, [])
        if self.identities["stm"] == task_id:
            self.runtime["stm"] = []
        return len(entries)

    def apply_compaction(self, *, tier: str, namespace: str, scope: str,
                         source_entries: list[dict[str, Any]], summary: str,
                         title: str, tags: list[str], expires_at=""):
        """Commit a reviewed durable summary iff every source is unchanged."""
        self._check_tier(tier)
        if tier == "stm":
            raise ValueError("short-term task memory is not durable and cannot be compacted")
        if not self.enabled[tier] or self.storage_errors.get(tier):
            raise RuntimeError(f"memory tier is unavailable: {tier}")
        if str(namespace) != self.identities[tier]:
            raise MemoryStoreError("memory_compaction_stale")
        entry = self.store.replace_with_compaction(
            source_entries, summary, tier=tier, namespace=namespace, scope=scope,
            title=title, tags=tags, expires_at=expires_at)
        self.runtime[tier] = self._load(tier)
        self.storage_errors.pop(tier, None)
        entry["project_id"] = self.project_id
        return entry

    def compact(self, tier: str, limit=64):
        self._check_tier(tier)
        self.runtime[tier] = self.runtime[tier][-max(1, min(64, int(limit))):]
        if tier == "stm":
            self._stm_tasks[self.identities[tier]] = self.runtime[tier]
        removed = (0 if tier == "stm" else self.store.keep_latest(
            tier, self.identities[tier], max(1, min(64, int(limit)))))
        return {"runtime_entries": len(self.runtime[tier]), "persistent_removed": removed}

    def state(self, tier=None):
        tiers = self.TIERS if tier is None else (tier,)
        self._prune_expired()
        result = {}
        for current in tiers:
            self._check_tier(current)
            raw_persistent = []
            persistent_count_known = True
            if current != "stm":
                try:
                    raw_persistent = self.store.list(
                        tier=current, namespace=self.identities[current])
                    self.storage_errors.pop(current, None)
                except MemoryStoreError as error:
                    self.storage_errors[current] = error.category
                    self.enabled[current] = False
                    self.runtime[current] = []
                    persistent_count_known = False
            persistent = (sum(not self.store.contains_secret(entry)
                              for entry in raw_persistent)
                          if persistent_count_known else None)
            result[current] = {"enabled": self.enabled[current],
                               "runtime_entries": len(self.runtime[current]),
                               "persistent_entries": persistent,
                               "persistent_count_known": persistent_count_known,
                               "storage_error": self.storage_errors.get(current, ""),
                               "loaded_into_process": self.enabled[current] and
                               (current != "stm" or self._retain_stm_task) and
                               not self.storage_errors.get(current),
                               "unsafe_persistent_entries_omitted": (
                                   len(raw_persistent) - (persistent or 0)
                                   if persistent_count_known else None),
                               "namespace_hash": hashlib.sha256(
                                   self.identities[current].encode()).hexdigest()[:16]}
        return result if tier is None else result[tier]

    @classmethod
    def _check_tier(cls, tier):
        if tier not in cls.TIERS:
            raise ValueError("unknown memory tier")


class MemoryTaskScopes:
    """Bounded active-task IDs keyed by durable chat session identity."""

    def __init__(self, memory: MemoryManager, *, max_sessions=32):
        self.memory = memory
        self.max_sessions = max(1, int(max_sessions))
        self._active: OrderedDict[str, str] = OrderedDict()

    def current(self, session_id: str):
        session_id = str(session_id).strip()
        task_id = self._active.get(session_id)
        if task_id:
            self._active.move_to_end(session_id)
        return task_id

    def is_active(self, task_id: str):
        return bool(task_id) and task_id in self._active.values()

    def start(self, session_id: str):
        session_id = self._require_session(session_id)
        ended_id = self._active.pop(session_id, None)
        cleared = self.memory.clear_task(ended_id) if ended_id else 0
        task_id = uuid4().hex
        self._active[session_id] = task_id
        while len(self._active) > self.max_sessions:
            _, expired_task = self._active.popitem(last=False)
            cleared += self.memory.clear_task(expired_task)
        return task_id, cleared

    def end(self, session_id: str):
        session_id = self._require_session(session_id)
        task_id = self._active.pop(session_id, None)
        return (task_id, self.memory.clear_task(task_id)) if task_id else (None, 0)

    @staticmethod
    def _require_session(session_id: str):
        session_id = str(session_id).strip()
        if not session_id:
            raise ValueError("durable session identity is required for task memory")
        return session_id
