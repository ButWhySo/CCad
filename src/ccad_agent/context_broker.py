"""Deterministic, bounded per-turn context retrieval and provenance."""

from __future__ import annotations

import hashlib
import json
import re
from collections import OrderedDict
from typing import Any

from project_index import ProjectIndex, project_model


_WORDS = re.compile(r"[a-z0-9_]{3,}", re.IGNORECASE)
_IDENTIFIERS = re.compile(
    r"(?<![a-z0-9])(?:[urcjtpqdl]|fb|ic|sw)\d+[a-z]?(?![a-z0-9])|"
    r"(?<![a-z0-9])(?:f|b|in|user|edge)\.(?:cu|silk|mask|paste|adhes|fab|crtyd|courtyard)(?![a-z0-9])|"
    r"(?<![a-z0-9])(?:gnd|vcc|vdd|vss|gn[d]|3v3|5v|usb[_-]?[a-z0-9]+)(?![a-z0-9])",
    re.IGNORECASE)
_SECRET = re.compile(
    r"(?:api[_-]?key|secret|password|token)\s*[:=]\s*\S+|"
    r"\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b|\bAIza[A-Za-z0-9_-]{20,}",
    re.IGNORECASE)
_SECRET_VALUE = re.compile(r"((?:api[_-]?key|secret|password|token)\s*[:=]\s*)\S+",
                           re.IGNORECASE)


def _safe_signal(value: Any, limit: int) -> str:
    text = str(value or "").strip()[:limit]
    return _SECRET_VALUE.sub(r"\1[redacted]", text)


def extract_context_signals(user_request: str, *, goal: str = "", project_id: str = "",
                            active_editor: str = "", selected_objects=(),
                            workflow: str = "", task: str = "", recent_turns=(),
                            recent_context=()) -> dict:
    """Create stable retrieval signals; never retain unbounded prompt text."""
    fields = [
        ("request", _safe_signal(user_request, 1400)),
        ("goal", _safe_signal(goal, 240)),
        ("project", _safe_signal(project_id, 100)),
        ("editor", _safe_signal(active_editor, 40)),
        ("workflow", _safe_signal(workflow, 80)),
        ("task", _safe_signal(task, 120)),
    ]
    objects = sorted({_safe_signal(item, 80).casefold() for item in selected_objects
                      if _safe_signal(item, 80)})[:32]
    turns = []
    for record in list(recent_turns)[-3:]:
        if not isinstance(record, dict):
            continue
        summary = _safe_signal(record.get("user_request_summary", ""), 180)
        summary += " " + _safe_signal(record.get("assistant_summary", ""), 180)
        if summary.strip():
            turns.append(summary.strip())
    recent = [_safe_signal(value, 240) for value in list(recent_context)[-8:]
              if _safe_signal(value, 240)]
    source = " ".join(value for _, value in fields if value)
    source += " " + " ".join(objects + turns + recent)
    if _SECRET.search(source):
        source = _SECRET_VALUE.sub(r"\1[redacted]", source)
        source = re.sub(r"\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b|"
                        r"\bAIza[A-Za-z0-9_-]{20,}", "[redacted]", source,
                        flags=re.IGNORECASE)
    identifiers = sorted({match.casefold() for match in _IDENTIFIERS.findall(source)})[:64]
    terms = list(dict.fromkeys(term.casefold() for term in _WORDS.findall(source)
                               if not _SECRET.search(term)))[:96]
    query_parts = [source[:2048], " ".join(identifiers)]
    query = " ".join(part for part in query_parts if part).strip()[:3072]
    canonical = json.dumps({"terms": terms, "identifiers": identifiers},
                           sort_keys=True, separators=(",", ":"))
    return {"version": 1, "terms": terms, "identifiers": identifiers,
            "domains": [name for name in ("pcb", "schematic", "library", "routing")
                        if re.search(rf"\b{name}\b", source, re.IGNORECASE)],
            "query": query, "digest": hashlib.sha256(canonical.encode()).hexdigest()[:24]}


class ContextBroker:
    """Cache exact memory retrieval for a thread/revision/signal generation."""

    MAX_CACHE_ENTRIES = 32
    MAX_MEMORY_ENTRIES = 8
    MAX_MEMORY_CHARS = 4000

    def __init__(self, *, memory_token_budget=1000):
        self.memory_token_budget = max(64, min(8000, int(memory_token_budget)))
        self._cache: OrderedDict[str, dict] = OrderedDict()
        self._versions: dict[str, int] = {}
        self._project_indexes: OrderedDict[str, ProjectIndex] = OrderedDict()

    def invalidate_thread(self, thread_id: str):
        """Immediately discard process-held memory/context for a thread."""
        thread_id = str(thread_id)
        for key, value in list(self._cache.items()):
            if value.get("thread_id") == thread_id:
                del self._cache[key]

    @staticmethod
    def _memory_generation(manager) -> str:
        rows: list[tuple[str, ...]] = [
            (tier, str(bool(manager.enabled[tier])), str(manager.identities[tier]))
            for tier in manager.TIERS]
        for tier in manager.TIERS:
            if not manager.enabled[tier]:
                continue
            for entry in manager.runtime[tier]:
                rows.append((tier, entry.get("id", ""), entry.get("updated_at", ""),
                             hashlib.sha256(str(entry.get("content", "")).encode()).hexdigest()))
        encoded = json.dumps(rows, sort_keys=True, separators=(",", ":"))
        return hashlib.sha256(encoded.encode()).hexdigest()[:24]

    @staticmethod
    def _manifest(manager, historical_turn_count: int, states=None) -> dict:
        states = states or manager.state()
        tiers = {}
        for tier in manager.TIERS:
            state = states[tier]
            tiers[tier] = {
                "enabled": bool(state["enabled"]),
                "available_count": (state["persistent_entries"] if tier != "stm"
                                    else state["runtime_entries"]),
                "loaded_count": int(state["runtime_entries"]),
                "scope": {"stm": "active_task", "ltm": "current_thread",
                          "episodic": "local_user"}[tier],
            }
        return {"version": 1, "tiers": tiers,
                "available_tier_count": sum(bool(item["enabled"] and
                                                    item["available_count"])
                                             for item in tiers.values()),
                "historical_thread_summary_count": max(0, int(historical_turn_count)),
                "project_scope_available": False,
                "project_memory_count": None,
                "semantic_retrieval_ready": False,
                "contents_included": False}

    def _select(self, entries, provenance):
        meta_by_id = {str(item.get("entry_id")): item for item in provenance
                      if isinstance(item, dict)}
        selected, selected_meta, chars = [], [], 0
        token_chars = self.memory_token_budget * 4
        for entry in entries:
            if not isinstance(entry, dict):
                continue
            size = len(json.dumps(entry, ensure_ascii=False, sort_keys=True,
                                  separators=(",", ":")))
            if len(selected) >= self.MAX_MEMORY_ENTRIES or chars + size > min(
                    self.MAX_MEMORY_CHARS, token_chars):
                continue
            selected.append(entry)
            chars += size
            detail = meta_by_id.get(str(entry.get("id", "")))
            if detail:
                selected_meta.append(detail)
        return selected, selected_meta, chars

    @staticmethod
    def _comparison_texts(recent_turns, recent_context, thread_recap):
        corpus = []
        for record in list(recent_turns or ())[-6:]:
            if isinstance(record, dict):
                corpus.extend((record.get("user_request_summary", ""),
                               record.get("assistant_summary", "")))
        corpus.extend(list(recent_context or ())[-8:])
        if isinstance(thread_recap, dict):
            for record in list(thread_recap.get("turns", ()))[-6:]:
                if not isinstance(record, dict):
                    continue
                corpus.extend((record.get("user_request_summary", ""),
                               record.get("assistant_summary", "")))
                corpus.extend(record.get("explicit_constraints", ())[:8]
                              if isinstance(record.get("explicit_constraints"), list)
                              else ())
                corpus.extend(record.get("important_findings", ())[:4]
                              if isinstance(record.get("important_findings"), list)
                              else ())
        return [_safe_signal(value, 300) for value in corpus if _safe_signal(value, 300)]

    @staticmethod
    def _deduplicate_against_context(entries, provenance, comparison_texts):
        token_sets = [set(_WORDS.findall(value.casefold())) for value in comparison_texts]
        token_sets = [terms for terms in token_sets if len(terms) >= 5]
        if not token_sets:
            return entries, provenance
        meta = {str(item.get("entry_id")): item for item in provenance
                if isinstance(item, dict)}
        kept, kept_meta = [], []
        for entry in entries:
            content_terms = set(_WORDS.findall(str(entry.get("content", "")).casefold()))
            all_terms = set(_WORDS.findall(
                f"{entry.get('title', '')} {entry.get('content', '')}".casefold()))
            duplicated = any(
                len(content_terms & previous) / max(1, len(content_terms)) >= 0.92
                or len(all_terms & previous) / max(1, len(all_terms)) >= 0.92
                for previous in token_sets if len(content_terms) >= 5)
            if duplicated:
                continue
            kept.append(entry)
            if str(entry.get("id", "")) in meta:
                kept_meta.append(meta[str(entry["id"])])
        return kept, kept_meta

    @staticmethod
    def _summary(entries):
        parts = []
        for entry in entries:
            title = _safe_signal(entry.get("title", ""), 80)
            if _SECRET.search(title):
                continue
            if title:
                parts.append(title)
        return ("Relevant stored topics: " + "; ".join(parts))[:400] if parts else ""

    def prepare(self, manager, *, thread_id: str, project_revision: str,
                user_request: str, goal: str = "", project_id: str = "",
                active_editor: str = "", selected_objects=(), workflow: str = "",
                task: str = "", recent_turns=(), historical_turn_count=0,
                force_refresh=False, signals: dict | None = None,
                recent_context=(), thread_recap: dict | None = None,
                project_snapshot=None, active_layer="", active_net=""):
        signals = signals or extract_context_signals(
            user_request, goal=goal, project_id=project_id, active_editor=active_editor,
            selected_objects=selected_objects, workflow=workflow, task=task,
            recent_turns=recent_turns, recent_context=recent_context)
        states = manager.state()
        generation = self._memory_generation(manager)
        query_digest = hashlib.sha256(str(signals["query"]).encode()).hexdigest()[:24]
        comparison_texts = self._comparison_texts(recent_turns, recent_context, thread_recap)
        comparison_digest = hashlib.sha256(json.dumps(
            comparison_texts, ensure_ascii=False, separators=(",", ":")).encode()
        ).hexdigest()[:24]
        key_material = json.dumps([str(thread_id), str(project_revision), signals["digest"],
                                   query_digest, int(historical_turn_count), generation,
                                   comparison_digest,
                                   int(self.memory_token_budget)],
                                  separators=(",", ":"))
        key = hashlib.sha256(key_material.encode()).hexdigest()
        if not force_refresh and key in self._cache:
            cached = self._cache.pop(key)
            self._cache[key] = cached
            return {**cached, "cache_hit": True}
        entries, provenance = manager.retrieve_with_metadata(signals["query"],
                                                              limit=self.MAX_MEMORY_ENTRIES)
        entries, provenance = self._deduplicate_against_context(
            entries, provenance, comparison_texts)
        entries, provenance, chars = self._select(entries, provenance)
        manifest = self._manifest(manager, historical_turn_count, states)
        project_retrieval = {"available": False, "entities": [], "characters": 0,
                             "revision": "", "stats": {"index_state": "unavailable",
                             "total_entities": 0, "omitted_count": 0},
                             "relationship_semantics": "shared_net_association_only"}
        if project_snapshot:
            project_model_data = project_model(project_snapshot)
            project_key = str(project_model_data.get("id") or
                              project_model_data.get("name") or
                              project_id or project_revision)
            project_index = self._project_indexes.get(project_key)
            if project_index is None:
                project_index = ProjectIndex()
                self._project_indexes[project_key] = project_index
            project_result = project_index.retrieve(
                project_snapshot, signals["query"], active_layer=active_layer,
                active_net=active_net, selected_objects=selected_objects)
            project_retrieval = project_result
            if project_result.get("available"):
                # Retain populated indexes for incremental updates across turns.
                self._project_indexes.move_to_end(project_key)
                while len(self._project_indexes) > 1:
                    self._project_indexes.popitem(last=False)
        thread = str(thread_id)
        version = self._versions.get(thread, 0) + 1
        self._versions[thread] = version
        result = {"version": version, "cache_hit": False, "thread_id": thread,
                  "project_revision": str(project_revision), "signal_digest": signals["digest"],
                  "memory_generation": generation, "signals": signals,
                  "memories": entries, "memory_retrieval": provenance,
                  "dedup_context": comparison_texts,
                  "memory_summary": self._summary(entries), "manifest": manifest,
                  "project_retrieval": project_retrieval,
                  "memory_chars": chars, "memory_token_budget": self.memory_token_budget,
                  "historical_turn_count": max(0, int(historical_turn_count)),
                  "change_reason": "initial_context" if version == 1 else "context_changed"}
        self._cache[key] = result
        while len(self._cache) > self.MAX_CACHE_ENTRIES:
            self._cache.popitem(last=False)
        return dict(result)

    def refresh_memory(self, manager, context: dict, query: str, *, reason: str):
        """Expand only memory retrieval; preserve prior valid candidates and IDs."""
        extra = extract_context_signals(query)["query"]
        merged_query = (str(context.get("signals", {}).get("query", "")) + " " + extra).strip()
        entries, provenance = manager.retrieve_with_metadata(merged_query,
                                                              limit=self.MAX_MEMORY_ENTRIES)
        entries, provenance = self._deduplicate_against_context(
            entries, provenance, list(context.get("dedup_context", ())))
        old_entries = {str(item.get("id")): item for item in context.get("memories", [])}
        old_provenance = {str(item.get("entry_id")): item
                          for item in context.get("memory_retrieval", [])}
        ordered_entries, ordered_provenance = [], []
        for entry, detail in zip(entries, provenance):
            entry_id = str(entry.get("id"))
            if entry_id not in {str(item.get("id")) for item in ordered_entries}:
                ordered_entries.append(old_entries.get(entry_id, entry))
                ordered_provenance.append(old_provenance.get(entry_id, detail))
        for entry_id, entry in old_entries.items():
            if entry_id not in {str(item.get("id")) for item in ordered_entries}:
                ordered_entries.append(entry)
                if entry_id in old_provenance:
                    ordered_provenance.append(old_provenance[entry_id])
        selected, selected_meta, chars = self._select(
            ordered_entries, ordered_provenance)
        thread = str(context.get("thread_id", ""))
        version = max(self._versions.get(thread, int(context.get("version", 0))),
                      int(context.get("version", 0))) + 1
        self._versions[thread] = version
        refreshed_signals = extract_context_signals(merged_query)
        refreshed = {**context, "version": version, "cache_hit": False,
                     "memories": selected, "memory_retrieval": selected_meta,
                     "memory_summary": self._summary(selected), "memory_chars": chars,
                     "signals": refreshed_signals,
                     "signal_digest": refreshed_signals["digest"],
                     "manifest": self._manifest(manager,
                                                context.get("historical_turn_count", 0)),
                     "memory_generation": self._memory_generation(manager),
                     "change_reason": _safe_signal(reason, 80) or "targeted_memory_refresh"}
        self._cache.clear()
        return refreshed
