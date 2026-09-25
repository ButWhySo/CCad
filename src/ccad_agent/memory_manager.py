"""Runtime memory tiers over the durable, secret-rejecting MemoryStore."""

from __future__ import annotations

import hashlib
import re
from collections import OrderedDict
from datetime import datetime, timezone
from typing import Any

from lexical_retrieval import diversify_ranked, fuse_rankings, rank_documents
from uuid import uuid4

from memory_store import MemoryStore, MemoryStoreError
from semantic_retrieval import EmbeddingError, OllamaEmbeddingBackend


class MemoryManager:
    TIERS = ("stm", "ltm", "episodic")
    MAX_STM_TASKS = 32
    MAX_EMBEDDING_CACHE = 2048
    MAX_SEMANTIC_CANDIDATES = 32
    MIN_SEMANTIC_SIMILARITY = 0.25
    NEAR_DUPLICATE_THRESHOLD = 0.88
    _word = re.compile(r"[a-z0-9_]{3,}", re.IGNORECASE)

    def __init__(self, store: MemoryStore, *, task_id="ccad-task", thread_id="ccad-local",
                 project_id="", user_id="local-user"):
        self.store = store
        self.identities = {"stm": str(task_id), "ltm": str(thread_id),
                           "episodic": str(user_id)}
        self.project_id = str(project_id)
        self.enabled = {tier: False for tier in self.TIERS}
        self.runtime: dict[str, list[dict[str, Any]]] = {tier: [] for tier in self.TIERS}
        self.storage_errors: dict[str, str] = {}
        self._stm_tasks: OrderedDict[str, list[dict[str, Any]]] = OrderedDict()
        self._retain_stm_task = True
        self._embedding_backend_override = None
        self._embedding_backend = None
        self._embedding_config_identity = ""
        self._embedding_cache: OrderedDict[str, list[float]] = OrderedDict()
        self._query_embedding_cache: OrderedDict[str, list[float]] = OrderedDict()
        self.semantic_config = {"enabled": False, "backend": "ollama_local",
                                "base_url": "http://127.0.0.1:11434",
                                "model": "embeddinggemma"}
        self._semantic_status = {"enabled": False, "ready": False,
                                 "status": "disabled", "error": "",
                                 "backend": "ollama_local", "model": "embeddinggemma",
                                 "model_version": "", "cache_entries": 0}

    def set_identities(self, *, task_id: str, thread_id: str, project_id: str,
                       user_id="local-user", retain_stm_task=True):
        identities = {"stm": str(task_id), "ltm": str(thread_id),
                      "episodic": str(user_id)}
        project_changed = str(project_id) != self.project_id
        changed = {tier for tier in self.TIERS
                   if identities[tier] != self.identities[tier]}
        if project_changed:
            changed.add("ltm")
        if changed:
            self._clear_embedding_cache()
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
        semantic = flags.get("semantic", {})
        self._configure_semantic(semantic if isinstance(semantic, dict) else {})
        return failures

    @property
    def embedding_cache_entries(self):
        return len(self._embedding_cache) + len(self._query_embedding_cache)

    def set_embedding_backend(self, backend):
        """Set a backend implementing identity, embed_query, and embed_documents."""
        self._embedding_backend_override = backend

    def _configure_semantic(self, requested):
        config = {
            "enabled": bool(requested.get("enabled", False)),
            "backend": str(requested.get("backend", "ollama_local")),
            "base_url": str(requested.get("base_url", "http://127.0.0.1:11434")),
            "model": str(requested.get("model", "embeddinggemma")),
        }
        config_identity = "|".join((config["backend"], config["base_url"], config["model"]))
        old_identity = getattr(self._embedding_backend, "identity", "")
        self.semantic_config = config
        if not config["enabled"]:
            self._embedding_backend = None
            self._clear_embedding_cache()
            self._semantic_status = {**config, "ready": False, "status": "disabled",
                                     "error": "", "model_version": "",
                                     "cache_entries": self.embedding_cache_entries}
            return
        if config["backend"] != "ollama_local" and self._embedding_backend_override is None:
            self._embedding_backend = None
            self._semantic_status = {**config, "ready": False,
                                     "status": "backend_unsupported",
                                     "error": "backend_unsupported", "model_version": "",
                                     "cache_entries": self.embedding_cache_entries}
            return
        try:
            if self._embedding_backend_override is not None:
                backend = self._embedding_backend_override
                probe = {"ready": True, "status": "ready", "error": "",
                         "model_version": str(getattr(backend, "identity", ""))[:128]}
            else:
                backend = OllamaEmbeddingBackend(config["base_url"], config["model"])
                probe = backend.check_ready()
            new_identity = getattr(backend, "identity", "")
            if ((old_identity and old_identity != new_identity) or
                    (self._embedding_config_identity and
                     self._embedding_config_identity != config_identity)):
                self._embedding_cache.clear()
                self._query_embedding_cache.clear()
            self._embedding_backend = backend
            self._embedding_config_identity = config_identity
            self._semantic_status = {**config, **probe,
                                     "model_version": str(probe.get("model_version", ""))[:128],
                                     "cache_entries": self.embedding_cache_entries}
        except EmbeddingError as error:
            self._embedding_backend = None
            self._embedding_config_identity = config_identity
            self._embedding_cache.clear()
            self._query_embedding_cache.clear()
            self._semantic_status = {**config, "ready": False, "status": error.category,
                                     "error": error.category, "model_version": "",
                                     "cache_entries": 0}

    def semantic_state(self):
        return {**self._semantic_status,
                "cache_entries": self.embedding_cache_entries,
                "model_version": str(self._semantic_status.get("model_version", ""))[:128]}

    @property
    def semantic_embedding_backend(self):
        """Return the backend only while local semantic retrieval is enabled and ready."""
        if (not self.semantic_config.get("enabled") or
                not self._semantic_status.get("ready")):
            return None
        return self._embedding_backend

    def refresh_semantic_readiness(self):
        if not self.semantic_config.get("enabled") or self._embedding_backend_override is not None:
            return self.semantic_state()
        try:
            backend = OllamaEmbeddingBackend(self.semantic_config["base_url"],
                                             self.semantic_config["model"])
            probe = backend.check_ready()
            old_identity = getattr(self._embedding_backend, "identity", "")
            if old_identity and old_identity != backend.identity:
                self._clear_embedding_cache()
            self._embedding_backend = backend
            self._semantic_status = {**self.semantic_config, **probe,
                                     "model_version": str(probe.get("model_version", ""))[:128],
                                     "cache_entries": self.embedding_cache_entries}
        except EmbeddingError as error:
            self._embedding_backend = None
            self._semantic_status.update({"ready": False, "status": error.category,
                                          "error": error.category})
        return self.semantic_state()

    def _clear_embedding_cache(self):
        self._embedding_cache.clear()
        self._query_embedding_cache.clear()

    def enable(self, tier: str):
        self._check_tier(tier)
        if tier != "stm":
            for namespace in self._namespaces(tier):
                self.store.ensure_namespace(tier, namespace)
        loaded = self._load(tier)
        self.runtime[tier] = loaded
        self.enabled[tier] = True
        self.storage_errors.pop(tier, None)
        return self.state(tier)

    def disable(self, tier: str):
        self._check_tier(tier)
        self.enabled[tier] = False
        self.runtime[tier] = []
        self._clear_embedding_cache()
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
        for namespace in self._namespaces(tier):
            for entry in self.store.list(tier=tier, namespace=namespace):
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
                if entry.get("scope") == "project" and tier != "ltm":
                    continue
                if tier == "ltm" and entry.get("scope") == "project":
                    entry["project_id"] = self.project_id
                entries.append(entry)
        for entry_id in expired:
            self.store.delete(entry_id)
        return self._bounded_runtime(tier, entries)

    def _bounded_runtime(self, tier: str, entries=None, limit=64):
        source = self.runtime[tier] if entries is None else entries
        limit = max(1, min(64, int(limit)))
        if tier == "stm":
            return list(source[-limit:])
        by_namespace: dict[str, list[dict[str, Any]]] = {}
        for entry in source:
            namespace = str(entry.get("namespace", self.identities[tier]))
            by_namespace.setdefault(namespace, []).append(entry)
        return [entry for namespace_entries in by_namespace.values()
                for entry in namespace_entries[-limit:]]

    def _namespaces(self, tier: str):
        namespaces = [self.identities[tier]]
        if tier == "ltm" and self.project_id.strip():
            namespaces.append(self._project_namespace())
        return list(dict.fromkeys(namespace for namespace in namespaces if namespace))

    def _project_namespace(self):
        return "project-" + hashlib.sha256(
            self.project_id.strip().encode("utf-8")).hexdigest()

    def namespace_for(self, tier: str, scope: str | None):
        if scope == "project" and tier != "ltm":
            raise ValueError("project-scoped memory must use the durable LTM tier")
        if tier == "ltm" and scope == "project":
            project_id = self.project_id.strip()
            if not project_id:
                raise ValueError("project-scoped memory requires an active project identity")
            return self._project_namespace()
        return self.identities[tier]

    def compaction_identities(self):
        """Expose only active identities needed to invalidate prepared summaries."""
        identities = dict(self.identities)
        if self.project_id.strip():
            identities["ltm:project"] = self._project_namespace()
        return identities

    def retrieve(self, query: str, *, limit=8):
        entries, _ = self.retrieve_with_metadata(query, limit=limit)
        return entries

    def retrieve_with_metadata(self, query: str, *, limit=8):
        """Return ranked entries plus content-free provenance for diagnostics."""
        self._prune_expired()
        candidates = []
        for tier_index, tier in enumerate(self.TIERS):
            if not self.enabled[tier] or self.storage_errors.get(tier):
                continue
            for entry_index, entry in enumerate(self.runtime[tier]):
                if self.store.contains_secret(entry):
                    continue
                title = str(entry.get("title", ""))
                content = str(entry.get("content", ""))
                raw_tags = entry.get("tags", [])
                tag_values = raw_tags if isinstance(raw_tags, (list, tuple)) else ()
                tags = " ".join(tag for tag in tag_values if isinstance(tag, str))
                candidates.append({"tier": tier, "tier_index": tier_index,
                                   "entry_index": entry_index, "entry": entry,
                                   "text": f"{title} {content} {tags}",
                                   "title_text": title, "content_text": content,
                                   "tags_text": tags,
                                   "kind": entry.get("kind", "fact"),
                                   "created_at": str(entry.get("created_at", ""))})
        query_term_count = len(set(self._word.findall(str(query).casefold())))
        lexical = rank_documents(query, candidates,
                                 min_matches=min(2, query_term_count))
        eligible = {item["document"]["entry"]["id"] for item in lexical}
        channels = {}
        for channel, field in (("title", "title_text"),
                               ("content", "content_text"),
                               ("tags", "tags_text")):
            field_docs = [candidate for candidate in candidates
                          if candidate["entry"]["id"] in eligible]
            channels[channel] = rank_documents(query, field_docs,
                                               text_key=field, min_matches=1)
        fused = fuse_rankings(channels, weights={"title": 1.2,
                                                 "content": 1.0,
                                                 "tags": 0.8})
        semantic_ranked = self._semantic_rankings(candidates, str(query))
        semantic_by_id = {item["document"]["entry"]["id"]: item
                          for item in semantic_ranked}
        if semantic_ranked:
            fused = fuse_rankings({**channels, "semantic": semantic_ranked},
                                  weights={"title": 1.2, "content": 1.0,
                                           "tags": 0.8, "semantic": 1.0})
        # Kind only adjusts already-relevant candidates; it cannot introduce a
        # preference/correction that failed the lexical/semantic eligibility gate.
        kind_weights = {"fact": 1.0, "preference": 1.08, "correction": 1.16}
        for item in fused:
            kind = item["document"].get("kind", "fact")
            item["score"] *= kind_weights.get(kind, 1.0)
            item["kind_weight"] = kind_weights.get(kind, 1.0)
        fused.sort(key=lambda item: (-item["score"], item["ordinal"]))
        lexical_by_id = {item["document"]["entry"]["id"]: item
                         for item in lexical}
        diversified = diversify_ranked(fused, limit=max(0, min(32, int(limit))),
                                       text_key="text", relevance_weight=0.7,
                                       similarity_fn=self._candidate_similarity)
        selected = diversified
        entries = [item["document"]["entry"] for item in selected]
        provenance = [{
            "entry_id": item["document"]["entry"]["id"],
            "rank": index + 1,
            "tier": item["document"]["tier"],
            "memory_kind": item["document"].get("kind", "fact"),
            "kind_weight": round(item.get("kind_weight", 1.0), 3),
            "query_overlap_terms": len(item["matched_terms"]),
            "bm25_score": round(lexical_by_id.get(
                item["document"]["entry"]["id"], {}).get("score", 0.0), 6),
            "ranking_method": ("hybrid_bm25_rrf_mmr" if semantic_ranked
                               else "fielded_bm25_rrf_mmr"),
            "channel_ranks": item["channel_ranks"],
            "rrf_score": round(item["score"], 8),
            "diversity_score": item["diversity_score"],
            "redundancy_score": item["redundancy_score"],
            "matched_terms": item["matched_terms"],
            **({"semantic_similarity": round(semantic_by_id[
                item["document"]["entry"]["id"]]["score"], 6)}
               if item["document"]["entry"]["id"] in semantic_by_id else {}),
            "namespace_hash": hashlib.sha256(
                str(item["document"]["entry"].get(
                    "namespace", self.identities[item["document"]["tier"]])).encode()
            ).hexdigest()[:16],
        } for index, item in enumerate(selected)]
        return entries, provenance

    def _semantic_rankings(self, candidates, query):
        backend = self._embedding_backend
        if (not candidates or not query.strip() or
                not self._semantic_status.get("ready") or backend is None):
            return []
        eligible = [item for item in candidates
                    if not self.store.contains_secret(item["entry"])]
        if not eligible:
            return []
        try:
            backend_identity = str(backend.identity)
            query_key = backend_identity + ":q:" + hashlib.sha256(
                query.encode("utf-8")).hexdigest()
            query_vector = self._embedding_cache_get(self._query_embedding_cache, query_key)
            if query_vector is None:
                query_vector = OllamaEmbeddingBackend._normalize_vector(
                    backend.embed_query(query))
                self._cache_embedding(self._query_embedding_cache, query_key, query_vector)
            document_vectors = {}
            missing = []
            for candidate in eligible:
                entry = candidate["entry"]
                content_fingerprint = hashlib.sha256(candidate["text"].encode(
                    "utf-8")).hexdigest()
                namespace_hash = hashlib.sha256(str(entry.get("namespace", "")).encode()
                                                ).hexdigest()[:16]
                key = ":".join((backend_identity, candidate["tier"], namespace_hash,
                                str(entry.get("id", "")), content_fingerprint))
                vector = self._embedding_cache_get(self._embedding_cache, key)
                if vector is None:
                    missing.append((candidate, key))
                else:
                    document_vectors[id(candidate)] = vector
            for offset in range(0, len(missing), OllamaEmbeddingBackend.MAX_TEXTS):
                batch = missing[offset:offset + OllamaEmbeddingBackend.MAX_TEXTS]
                vectors = backend.embed_documents([item[0]["text"] for item in batch])
                if len(vectors) != len(batch):
                    raise EmbeddingError("embedding_invalid_response")
                for (candidate, key), vector in zip(batch, vectors):
                    normalized = OllamaEmbeddingBackend._normalize_vector(vector)
                    self._cache_embedding(self._embedding_cache, key, normalized)
                    document_vectors[id(candidate)] = normalized
            dimensions = len(query_vector)
            ranked = []
            if any(len(vector) != dimensions for vector in document_vectors.values()):
                raise EmbeddingError("embedding_dimension_mismatch")
            for ordinal, candidate in enumerate(eligible):
                vector = document_vectors.get(id(candidate))
                if vector is None:
                    continue
                similarity = self._cosine(query_vector, vector)
                candidate["_semantic_vector"] = vector
                if similarity >= self.MIN_SEMANTIC_SIMILARITY:
                    ranked.append({"document": candidate, "score": similarity,
                                   "matched_terms": [], "ordinal": ordinal})
            ranked.sort(key=lambda item: (-item["score"], item["ordinal"]))
            self._semantic_status.update({"ready": True, "status": "ready", "error": "",
                                          "cache_entries": self.embedding_cache_entries})
            return ranked[:self.MAX_SEMANTIC_CANDIDATES]
        except (EmbeddingError, AttributeError, TypeError, ValueError,
                OverflowError, ArithmeticError) as error:
            category = (error.category if isinstance(error, EmbeddingError)
                        else "embedding_invalid_response")
            self._semantic_status.update({"ready": False, "status": category,
                                          "error": category})
            self._embedding_backend = None
            return []
        except Exception:
            category = "embedding_failed"
            self._semantic_status.update({"ready": False, "status": category,
                                          "error": category})
            self._embedding_backend = None
            return []

    @staticmethod
    def _cosine(left, right):
        if not left or len(left) != len(right):
            return -1.0
        dot = sum(a * b for a, b in zip(left, right))
        norm_left = sum(a * a for a in left) ** 0.5
        norm_right = sum(b * b for b in right) ** 0.5
        return dot / (norm_left * norm_right) if norm_left and norm_right else -1.0

    @classmethod
    def _candidate_similarity(cls, left, right):
        left_vector = left.get("_semantic_vector")
        right_vector = right.get("_semantic_vector")
        if isinstance(left_vector, list) and isinstance(right_vector, list) and (
                len(left_vector) == len(right_vector)):
            return max(0.0, cls._cosine(left_vector, right_vector))
        left_terms = set(cls._word.findall(str(left.get("text", "")).casefold()))
        right_terms = set(cls._word.findall(str(right.get("text", "")).casefold()))
        return len(left_terms & right_terms) / max(1, len(left_terms | right_terms))

    @staticmethod
    def _embedding_cache_get(cache, key):
        vector = cache.pop(key, None)
        if vector is not None:
            cache[key] = vector
        return vector

    def _cache_embedding(self, cache, key, vector):
        normalized = OllamaEmbeddingBackend._normalize_vector(vector)
        cache.pop(key, None)
        cache[key] = normalized
        while len(cache) > self.MAX_EMBEDDING_CACHE:
            cache.popitem(last=False)

    def context_entries(self, query: str):
        return self.retrieve(query, limit=8)

    def add(self, content: str, *, tier="ltm", title="", scope=None, tags=None,
            kind=None,
            expires_at=""):
        self._check_tier(tier)
        if not self.enabled[tier]:
            raise RuntimeError(f"memory tier disabled: {tier}")
        if tier == "stm" and not self._retain_stm_task:
            raise RuntimeError("STM requires an active task; use /task start")
        normalized = " ".join(str(content).casefold().split())
        scope = str(scope or {"stm": "task", "ltm": "conversation",
                              "episodic": "user"}[tier])
        existing = next((item for item in self.list(tier=tier, scope=scope)
                         if " ".join(str(item.get("content", "")).casefold().split()) == normalized), None)
        if existing:
            if kind is not None and existing.get("kind", "fact") != kind:
                return self.update(existing["id"], content, kind=kind)
            return existing
        duplicate = self._near_duplicate(content, tier, scope=scope)
        if duplicate:
            entry, similarity = duplicate
            raise ValueError(
                f"near-duplicate memory exists ({entry['id']}, lexical overlap "
                f"{similarity:.0%}); update that record or add distinct information")
        namespace = self.namespace_for(tier, scope)
        entry = self.store.normalise(content, title=title, scope=scope, tags=tags,
                                     tier=tier, kind=kind or "fact", namespace=namespace,
                                     expires_at=expires_at)
        entry["project_id"] = self.project_id
        if tier != "stm":
            entry = self.store.add(content, title=title, scope=scope, tags=tags,
                                   kind=kind or "fact",
                                   tier=tier, namespace=namespace,
                                   expires_at=expires_at)
            self.store.keep_latest(tier, namespace, 64)
            entry["project_id"] = self.project_id
        self.runtime[tier].append(entry)
        self._clear_embedding_cache()
        self.runtime[tier] = self._bounded_runtime(tier)
        if tier == "stm":
            self._stm_tasks[self.identities[tier]] = self.runtime[tier]
            self._stm_tasks.move_to_end(self.identities[tier])
            while len(self._stm_tasks) > self.MAX_STM_TASKS:
                self._stm_tasks.popitem(last=False)
        return entry

    def _near_duplicate(self, content: str, tier: str, *, exclude_id="", scope=None):
        words = set(self._word.findall(str(content).casefold()))
        if len(words) < 5:
            return None
        best = None
        namespace = self.namespace_for(tier, scope)
        duplicate_scope = "project" if tier == "ltm" and scope == "project" else None
        for entry in self.list(tier=tier, scope=duplicate_scope):
            if tier != "stm" and entry.get("namespace") != namespace:
                continue
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
            durable = ([] if current == "stm" else [
                item for namespace in self._namespaces(current)
                for item in self.store.list(tier=current, namespace=namespace, scope=scope)
                if not self.store.contains_secret(item) and
                not (current != "ltm" and item.get("scope") == "project")])
            if current == "ltm" and scope == "project":
                for item in durable:
                    item["project_id"] = self.project_id
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
                    candidates.extend(item for namespace in self._namespaces(tier)
                                     for item in self.store.list(tier=tier,
                                                                 namespace=namespace))
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
                self._clear_embedding_cache()
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
               expires_at=None, kind=None):
        for tier in self.TIERS:
            if not self.enabled[tier]:
                if tier != "stm" and any(
                        item.get("id") == entry_id
                        for namespace in self._namespaces(tier)
                        for item in self.store.list(tier=tier, namespace=namespace)):
                    raise RuntimeError(f"memory tier disabled: {tier}")
                continue
            entry = next((item for item in self.list(tier=tier)
                          if item.get("id") == entry_id), None)
            if entry is None:
                continue
            target_scope = entry.get("scope") if scope is None else scope
            duplicate = self._near_duplicate(content, tier, exclude_id=entry_id,
                                             scope=target_scope)
            if duplicate:
                other, similarity = duplicate
                raise ValueError(
                    f"near-duplicate memory exists ({other['id']}, lexical overlap "
                    f"{similarity:.0%}); revise to distinct information")
            target_scope = entry.get("scope", "project") if scope is None else scope
            namespace = self.namespace_for(tier, target_scope)
            fields = {"title": entry.get("title", "") if title is None else title,
                      "scope": target_scope,
                      "tags": entry.get("tags", []) if tags is None else tags,
                      "kind": entry.get("kind", "fact") if kind is None else kind,
                      "tier": tier, "namespace": namespace,
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
            self._clear_embedding_cache()
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
                removed = tier == "stm" or self.store.delete(entry_id)
                if removed:
                    self._clear_embedding_cache()
                return removed
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
        removed = self.store.delete(entry_id)
        if removed:
            self._clear_embedding_cache()
        return removed

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
            namespaces = self._namespaces(current)
            if current == "ltm" and scope == "project":
                namespaces = [self.namespace_for(current, scope)]
            persistent_matches = set() if current == "stm" else {
                item.get("id") for namespace in namespaces
                for item in self.store.list(tier=current, namespace=namespace, scope=scope)}
            self.runtime[current] = [item for item in self.runtime[current]
                                     if item.get("scope") != scope]
            if current != "stm":
                for namespace in namespaces:
                    removed += self.store.clear_scope(scope, tier=current,
                                                      namespace=namespace)
            removed += len(runtime_matches - persistent_matches)
        if removed:
            self._clear_embedding_cache()
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
                removed += self.store.clear_tier(current)
                removed += len(runtime_ids - stored_ids)
            else:
                removed += len(runtime_ids)
        if removed:
            self._clear_embedding_cache()
        return removed

    def clear_task(self, task_id: str):
        """Discard one process-only STM task without touching durable tiers."""
        task_id = str(task_id)
        entries = self._stm_tasks.pop(task_id, [])
        if self.identities["stm"] == task_id:
            self.runtime["stm"] = []
        if entries:
            self._clear_embedding_cache()
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
        if str(namespace) != self.namespace_for(tier, scope):
            raise MemoryStoreError("memory_compaction_stale")
        entry = self.store.replace_with_compaction(
            source_entries, summary, tier=tier, namespace=namespace, scope=scope,
            title=title, tags=tags, expires_at=expires_at)
        self.runtime[tier] = self._load(tier)
        self.storage_errors.pop(tier, None)
        entry["project_id"] = self.project_id
        self._clear_embedding_cache()
        return entry

    def compact(self, tier: str, limit=64):
        self._check_tier(tier)
        self.runtime[tier] = self._bounded_runtime(tier, limit=limit)
        if tier == "stm":
            self._stm_tasks[self.identities[tier]] = self.runtime[tier]
        removed = (0 if tier == "stm" else sum(
            self.store.keep_latest(tier, namespace, max(1, min(64, int(limit))))
            for namespace in self._namespaces(tier)))
        if removed:
            self._clear_embedding_cache()
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
                    raw_persistent = [entry for namespace in self._namespaces(current)
                                      for entry in self.store.list(
                                          tier=current, namespace=namespace)]
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
                               "project_entries": (sum(
                                   entry.get("scope") == "project" and
                                   entry.get("namespace") == self._project_namespace() and
                                   not self.store.contains_secret(entry)
                                   for entry in raw_persistent)
                                   if persistent_count_known else None),
                               "namespace_hash": hashlib.sha256("|".join(
                                   self._namespaces(current)).encode()).hexdigest()[:16]}
        if tier is None:
            result["semantic"] = self.semantic_state()
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
