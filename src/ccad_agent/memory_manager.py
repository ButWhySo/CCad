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
                 project_id="project"):
        self.store = store
        self.identities = {"stm": str(run_id), "ltm": str(thread_id),
                           "episodic": str(project_id)}
        self.enabled = {tier: False for tier in self.TIERS}
        self.runtime: dict[str, list[dict[str, Any]]] = {tier: [] for tier in self.TIERS}

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
        now = self._now()
        entries = []
        expired = []
        for entry in self.store.list(tier=tier, namespace=self.identities[tier]):
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
        candidates = []
        query_words = set(self._word.findall(str(query).lower()))
        for tier in self.TIERS:
            if not self.enabled[tier]:
                continue
            for entry in self.runtime[tier]:
                text = f"{entry.get('title', '')} {entry.get('content', '')}".lower()
                overlap = len(query_words.intersection(self._word.findall(text)))
                if query_words and overlap == 0:
                    continue
                recency = entry.get("created_at", "")
                candidates.append((overlap, recency, tier, entry))
        candidates.sort(key=lambda item: (item[0], item[1]), reverse=True)
        return [item[3] for item in candidates[:max(0, min(32, int(limit)))]]

    def context_entries(self, query: str):
        return self.retrieve(query, limit=8)

    def add(self, content: str, *, tier="ltm", title="", scope="project", tags=None,
            expires_at=""):
        self._check_tier(tier)
        if not self.enabled[tier]:
            raise RuntimeError(f"memory tier disabled: {tier}")
        entry = self.store.add(content, title=title, scope=scope, tags=tags,
                               tier=tier, namespace=self.identities[tier],
                               expires_at=expires_at)
        self.runtime[tier].append(entry)
        self.runtime[tier] = self.runtime[tier][-64:]
        return entry

    def reset(self, tier=None, *, persistent=True):
        tiers = self.TIERS if tier is None else (tier,)
        removed = 0
        for current in tiers:
            self._check_tier(current)
            self.runtime[current] = []
            if persistent:
                removed += self.store.clear_tier(current, self.identities[current])
        return removed

    def compact(self, tier: str, limit=64):
        self._check_tier(tier)
        self.runtime[tier] = self.runtime[tier][-max(1, min(64, int(limit))):]
        return len(self.runtime[tier])

    def state(self, tier=None):
        tiers = self.TIERS if tier is None else (tier,)
        result = {}
        for current in tiers:
            self._check_tier(current)
            persistent = len(self.store.list(tier=current, namespace=self.identities[current]))
            result[current] = {"enabled": self.enabled[current],
                               "runtime_entries": len(self.runtime[current]),
                               "persistent_entries": persistent,
                               "loaded_into_process": bool(self.runtime[current]),
                               "namespace_hash": hashlib.sha256(
                                   self.identities[current].encode()).hexdigest()[:16]}
        return result if tier is None else result[tier]

    @classmethod
    def _check_tier(cls, tier):
        if tier not in cls.TIERS:
            raise ValueError("unknown memory tier")
