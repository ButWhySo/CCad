"""Runtime memory tiers over the durable, secret-rejecting MemoryStore."""

from __future__ import annotations

import hashlib
import re
from datetime import datetime, timezone
from typing import Any

from memory_store import MemoryStore


class MemoryManager:
    TIERS = ("stm", "ltm", "episodic")
    _word = re.compile(r"[a-z0-9_]{3,}", re.IGNORECASE)

    def __init__(self, store: MemoryStore, *, run_id="ccad-run", thread_id="ccad-local",
                 project_id="project", user_id="local-user"):
        self.store = store
        self.identities = {"stm": str(run_id), "ltm": str(thread_id),
                           "episodic": str(user_id)}
        self.project_id = str(project_id)
        self.enabled = {tier: False for tier in self.TIERS}
        self.runtime: dict[str, list[dict[str, Any]]] = {tier: [] for tier in self.TIERS}

    def set_identities(self, *, run_id: str, thread_id: str, project_id: str,
                       user_id="local-user"):
        identities = {"stm": str(run_id), "ltm": str(thread_id),
                      "episodic": str(user_id)}
        changed = {tier for tier in self.TIERS
                   if identities[tier] != self.identities[tier]}
        self.identities = identities
        self.project_id = str(project_id)
        for tier in changed:
            self.runtime[tier] = self._load(tier) if self.enabled[tier] else []

    @staticmethod
    def _now():
        return datetime.now(timezone.utc)

    def configure(self, flags: dict[str, Any] | None):
        flags = flags if isinstance(flags, dict) else {}
        for tier in self.TIERS:
            if bool(flags.get(tier, False)):
                self.enable(tier)
            else:
                self.disable(tier)

    def enable(self, tier: str):
        self._check_tier(tier)
        self.enabled[tier] = True
        self.runtime[tier] = self._load(tier)
        return self.state(tier)

    def disable(self, tier: str):
        self._check_tier(tier)
        self.enabled[tier] = False
        self.runtime[tier] = []
        return self.state(tier)

    def _load(self, tier: str):
        if tier == "stm":
            return []
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
            if not self.enabled[tier]:
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
        scope = str(scope or {"stm": "task", "ltm": "conversation",
                              "episodic": "user"}[tier])
        normalized = " ".join(str(content).casefold().split())
        existing = next((item for item in self.list(tier=tier)
                         if " ".join(str(item.get("content", "")).casefold().split()) == normalized), None)
        if existing:
            return existing
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
        return entry

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
                candidates.extend(self.store.list(tier=tier, namespace=namespace))
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
                for entry_id in expired_ids:
                    if entry_id:
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
            return replacement
        return None

    def delete(self, entry_id):
        for tier in self.TIERS:
            matching = any(item.get("id") == entry_id for item in self.list(tier=tier))
            if matching:
                self.runtime[tier] = [item for item in self.runtime[tier]
                                      if item.get("id") != entry_id]
                return tier == "stm" or self.store.delete(entry_id)
        return False

    def clear_scope(self, scope, *, tier=None):
        tiers = self.TIERS if tier is None else (tier,)
        removed = 0
        for current in tiers:
            self._check_tier(current)
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
            if persistent:
                stored_ids = {item.get("id") for item in self.store.list(tier=current)}
                removed += self.store.clear_tier(current, None)
                removed += len(runtime_ids - stored_ids)
            else:
                removed += len(runtime_ids)
        return removed

    def compact(self, tier: str, limit=64):
        self._check_tier(tier)
        self.runtime[tier] = self.runtime[tier][-max(1, min(64, int(limit))):]
        removed = (0 if tier == "stm" else self.store.keep_latest(
            tier, self.identities[tier], max(1, min(64, int(limit)))))
        return {"runtime_entries": len(self.runtime[tier]), "persistent_removed": removed}

    def state(self, tier=None):
        self._prune_expired()
        tiers = self.TIERS if tier is None else (tier,)
        result = {}
        for current in tiers:
            self._check_tier(current)
            raw_persistent = ([] if current == "stm" else self.store.list(
                tier=current, namespace=self.identities[current]))
            persistent = sum(not self.store.contains_secret(entry)
                             for entry in raw_persistent)
            result[current] = {"enabled": self.enabled[current],
                               "runtime_entries": len(self.runtime[current]),
                               "persistent_entries": persistent,
                               "loaded_into_process": self.enabled[current],
                               "unsafe_persistent_entries_omitted": len(raw_persistent) - persistent,
                               "namespace_hash": hashlib.sha256(
                                   self.identities[current].encode()).hexdigest()[:16]}
        return result if tier is None else result[tier]

    @classmethod
    def _check_tier(cls, tier):
        if tier not in cls.TIERS:
            raise ValueError("unknown memory tier")
