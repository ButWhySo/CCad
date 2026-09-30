"""Translate current CCad indexes into backend-independent retrieval results."""

from __future__ import annotations

import hashlib
import json
import math
from typing import Any, cast

from retrieval_contracts import (RetrievalChannel, RetrievalHit, RetrievalRequest,
                                 RetrievalResult, RetrievalStatus)


_ALL_CHANNELS = frozenset(RetrievalChannel)
_PROJECT_METADATA = (
    "available", "reason", "revision", "characters", "stats",
    "relationship_semantics", "board_net_semantics", "logical_net_semantics",
    "power_passive_association_semantics", "spatial_semantics",
    "geometry_relationship_semantics", "semantic_status",
    "explicit_reference_semantics", "search_method")
_PROJECT_RETRIEVAL_CHANNEL = {
    "exact": RetrievalChannel.EXACT,
    "lexical": RetrievalChannel.LEXICAL,
    "semantic": RetrievalChannel.SEMANTIC,
    "hybrid": RetrievalChannel.LEXICAL,
    "relationship": RetrievalChannel.GRAPH,
    "spatial": RetrievalChannel.SPATIAL,
}


def _text_preview(entity: dict) -> str:
    fields = ("reference", "name", "value", "title", "text", "description",
              "footprint_name", "part", "net_id", "layer_id")
    return " | ".join(str(entity[key]).strip() for key in fields
                      if isinstance(entity.get(key), str) and entity[key].strip())[:480]


def _handle(source: str, scope: str, revision: str, canonical_id: str) -> str:
    identity = json.dumps((source, scope, revision, canonical_id),
                          ensure_ascii=True, separators=(",", ":"))
    return hashlib.sha256(identity.encode("utf-8")).hexdigest()


def _matches_request(entity: dict, request: RetrievalRequest) -> bool:
    if request.entity_types and entity.get("kind") not in request.entity_types:
        return False
    if request.net_ids:
        nets = _entity_ids(entity, "net_id", "candidate_net_ids", "related_net_ids")
        if not nets.intersection(request.net_ids):
            return False
    if request.layer_ids:
        layers = _entity_ids(entity, "layer_id", "layer_ids", "start_layer_id",
                             "end_layer_id")
        if not layers.intersection(request.layer_ids):
            return False
    if request.bbox is not None:
        box = entity.get("bounds_mm")
        if isinstance(box, dict):
            bounds = (box.get("min_x_mm"), box.get("min_y_mm"),
                      box.get("max_x_mm"), box.get("max_y_mm"))
        else:
            point = entity.get("position_mm")
            bounds = ((point.get("x"), point.get("y"),
                       point.get("x"), point.get("y")) if isinstance(point, dict) else ())
        if (len(bounds) != 4 or any(isinstance(value, bool) or
                                     not isinstance(value, (int, float)) or
                                     not math.isfinite(value)
                                     for value in bounds)):
            return False
        numeric_bounds = cast(tuple[int | float, int | float, int | float, int | float],
                              bounds)
        x1, y1, x2, y2 = request.bbox
        left, top, right, bottom = numeric_bounds
        if right < x1 or left > x2 or bottom < y1 or top > y2:
            return False
    return True


def _entity_ids(entity: dict, *keys: str) -> set[str]:
    values = set()
    for key in keys:
        value = entity.get(key)
        if isinstance(value, str):
            values.add(value)
        elif isinstance(value, (list, tuple, set)):
            values.update(item for item in value if isinstance(item, str))
    return values


def _requested_content(entity: dict, fields: tuple[str, ...]) -> dict:
    if not fields:
        return entity
    # Identity and retrieval evidence remain available even for a narrow payload.
    mandatory = {"id", "kind", "retrieval", "rank", "semantic_similarity",
                 "relationship", "relationships", "distance_mm", "source_revision"}
    selected = mandatory.union(fields)
    return {key: value for key, value in entity.items() if key in selected}


class ProjectIndexRetriever:
    """Domain adapter; ProjectIndex may change without changing ContextBroker."""

    channels = _ALL_CHANNELS

    def __init__(self, index, snapshot: Any, *, project_id: str = "",
                 active_layer: str = "", active_net: str = "",
                 selected_objects=(), embedding_backend=None):
        self.index = index
        self.snapshot = snapshot
        self.project_id = str(project_id)
        self.active_layer = str(active_layer)
        self.active_net = str(active_net)
        self.selected_objects = tuple(selected_objects or ())
        self.embedding_backend = embedding_backend

    def retrieve(self, request: RetrievalRequest) -> RetrievalResult:
        if request.scope != "project":
            return RetrievalResult(RetrievalStatus.FAILED, reason="scope_not_supported")
        if request.project_id and request.project_id != self.project_id:
            return RetrievalResult(RetrievalStatus.FAILED, reason="project_scope_mismatch")
        raw = self.index.retrieve(
            self.snapshot, request.query,
            active_layer=self.active_layer or (request.layer_ids[0]
                                               if request.layer_ids else ""),
            active_net=self.active_net or (request.net_ids[0] if request.net_ids else ""),
            selected_objects=tuple(dict.fromkeys(
                self.selected_objects + request.selected_object_ids)),
            limit=request.candidate_budget, embedding_backend=self.embedding_backend,
            channels=request.channels)
        if not raw.get("available"):
            return RetrievalResult(RetrievalStatus.UNAVAILABLE,
                                   revision=str(raw.get("revision", "")),
                                   reason=str(raw.get("reason", "project_index_unavailable"))[:128],
                                   metadata={key: raw[key] for key in _PROJECT_METADATA
                                             if key in raw})
        revision = str(raw.get("revision", ""))
        hits = []
        channel_ranks = {}
        for entity in raw.get("entities", ()):
            if not isinstance(entity, dict) or not _matches_request(entity, request):
                continue
            entity = _requested_content(entity, request.fields)
            mode = str(entity.get("retrieval", ""))
            primary = _PROJECT_RETRIEVAL_CHANNEL.get(mode)
            if primary is None:
                continue
            relationships = entity.get("relationships", ())
            relationships = tuple(value for value in relationships if isinstance(value, str)) \
                if isinstance(relationships, (list, tuple)) else ()
            relation = entity.get("relationship")
            if isinstance(relation, str) and relation not in relationships:
                relationships += (relation,)
            similarity = entity.get("semantic_similarity")
            distance = entity.get("distance_mm")
            evidence = {primary}
            if relationships:
                evidence.add(RetrievalChannel.GRAPH)
            if similarity is not None:
                evidence.add(RetrievalChannel.SEMANTIC)
            if distance is not None:
                evidence.add(RetrievalChannel.SPATIAL)
            channel = next((item for item in request.channels if item in evidence), None)
            if channel is None:
                continue
            channel = RetrievalChannel(channel)
            kind = str(entity.get("kind", "unknown"))
            object_id = str(entity.get("id", entity.get("reference", "")))
            canonical_id = f"{kind}:{object_id}"
            features = {"retrieval_mode": mode,
                        "exact_match": primary == RetrievalChannel.EXACT,
                        "graph_relationship": bool(relationships),
                        "semantic_match": similarity is not None,
                        "spatial_match": distance is not None,
                        "channel_rank_scope": "filtered_candidate_order"}
            provenance = {key: entity[key] for key in (
                "identity_source", "retrieval", "rank", "relationship", "relationships")
                         if key in entity} if request.include_provenance else {}
            hits.append(RetrievalHit(
                canonical_id=canonical_id, source_type=kind,
                source_scope=request.project_id or self.project_id or "project",
                source_revision=revision, channel=channel,
                channel_rank=channel_ranks.get(channel, 0) + 1,
                channel_score=None, normalized_features=features,
                matched_terms=(), semantic_similarity=similarity,
                relationship_path=relationships, spatial_distance=distance,
                provenance=provenance,
                content_handle=_handle("project", request.project_id or self.project_id,
                                        revision, canonical_id),
                text_preview=_text_preview(entity), content=entity))
            channel_ranks[channel] = channel_ranks.get(channel, 0) + 1
            if len(hits) >= request.top_k:
                break
        stats = dict(raw.get("stats", {})) if isinstance(raw.get("stats"), dict) else {}
        status = RetrievalStatus.READY
        semantic_state = str(raw.get("semantic_status", "disabled"))
        semantic_status = (RetrievalStatus.READY if semantic_state == "ready" else
                           RetrievalStatus.DISABLED if semantic_state == "disabled" else
                           RetrievalStatus.UNAVAILABLE)
        channel_statuses = {channel: RetrievalStatus.READY for channel in (
            RetrievalChannel.EXACT, RetrievalChannel.LEXICAL,
            RetrievalChannel.GRAPH, RetrievalChannel.SPATIAL)}
        channel_statuses[RetrievalChannel.SEMANTIC] = semantic_status
        if (RetrievalChannel.SEMANTIC in request.channels and
                semantic_status == RetrievalStatus.UNAVAILABLE):
            status = RetrievalStatus.PARTIAL
        metadata = {key: raw[key] for key in _PROJECT_METADATA if key in raw}
        metadata["stats"] = stats
        return RetrievalResult(status, tuple(hits), revision,
                               channel_statuses=channel_statuses, metadata=metadata)

    @staticmethod
    def to_context_payload(result: RetrievalResult) -> dict:
        """Preserve existing ContextBroker/package JSON while hiding backend types."""
        payload = dict(result.metadata)
        payload.update({"available": result.status not in {
                            RetrievalStatus.UNAVAILABLE, RetrievalStatus.FAILED,
                            RetrievalStatus.STALE},
                        "reason": result.reason, "revision": result.revision,
                        "entities": [dict(hit.content) for hit in result.hits]})
        payload["characters"] = sum(len(json.dumps(entity, ensure_ascii=False,
                                                    separators=(",", ":")))
                                    for entity in payload["entities"])
        return payload


class MemoryManagerRetriever:
    """Adapt tier-authorized MemoryManager hits to the common retrieval contract."""

    channels = frozenset((RetrievalChannel.LEXICAL, RetrievalChannel.SEMANTIC))

    def __init__(self, manager):
        self.manager = manager

    def retrieve(self, request: RetrievalRequest) -> RetrievalResult:
        if request.scope != "memory":
            return RetrievalResult(RetrievalStatus.FAILED, reason="scope_not_supported")
        if request.project_id and request.project_id != self.manager.project_id:
            return RetrievalResult(RetrievalStatus.FAILED, reason="project_scope_mismatch")
        if (request.thread_id and request.thread_id !=
                self.manager.identities.get("ltm", "")):
            return RetrievalResult(RetrievalStatus.FAILED, reason="thread_scope_mismatch")
        supported_channels = tuple(channel for channel in request.channels
                                   if channel in self.channels)
        if not supported_channels:
            return RetrievalResult(RetrievalStatus.FAILED,
                                   reason="unsupported_retrieval_channel")
        entries, provenance_rows = self.manager.retrieve_with_metadata(
            request.query, limit=request.candidate_budget,
            channels=tuple(channel.value for channel in supported_channels))
        by_id = {str(row.get("entry_id")): row for row in provenance_rows
                 if isinstance(row, dict)}
        hits = []
        lexical_channels = {"title", "content", "tags"}
        for entry in entries:
            entry_id = str(entry.get("id", ""))
            provenance = by_id.get(entry_id, {})
            ranks = provenance.get("channel_ranks", {})
            has_lexical = isinstance(ranks, dict) and bool(
                lexical_channels.intersection(ranks))
            has_semantic = isinstance(provenance.get("semantic_similarity"), (int, float))
            evidence = set()
            if has_lexical:
                evidence.add(RetrievalChannel.LEXICAL)
            if has_semantic:
                evidence.add(RetrievalChannel.SEMANTIC)
            channel = next((item for item in supported_channels if item in evidence), None)
            if channel is None:
                continue
            channel = RetrievalChannel(channel)
            channel_ranks = ranks if isinstance(ranks, dict) else {}
            lexical_ranks: list[int] = []
            for name in lexical_channels:
                candidate_rank = channel_ranks.get(name)
                if isinstance(candidate_rank, int) and candidate_rank > 0:
                    lexical_ranks.append(candidate_rank)
            rank = (channel_ranks.get("semantic") if channel == RetrievalChannel.SEMANTIC
                    else min(lexical_ranks, default=None))
            if not isinstance(rank, int) or rank < 1:
                rank = max(1, int(provenance.get("rank", len(hits) + 1)))
            tier = str(provenance.get("tier", entry.get("tier", "")))
            safe_provenance = dict(provenance) if request.include_provenance else {}
            hits.append(RetrievalHit(
                canonical_id=entry_id, source_type="memory", source_scope=tier,
                source_revision="", channel=channel,
                channel_rank=rank,
                normalized_features={key: provenance[key] for key in (
                    "importance_weight", "kind_weight", "recency_weight", "usage_weight")
                                    if key in provenance},
                matched_terms=tuple(provenance.get("matched_terms", ())),
                semantic_similarity=provenance.get("semantic_similarity"),
                provenance=safe_provenance,
                content_handle=_handle("memory", tier, "", entry_id),
                text_preview=_text_preview({"title": entry.get("title", ""),
                                            "text": entry.get("content", "")}),
                content=entry))
            if len(hits) >= request.top_k:
                break
        enabled = bool(any(self.manager.enabled.values()))
        errors = bool(getattr(self.manager, "storage_errors", {}))
        if not enabled:
            status = RetrievalStatus.DISABLED
        elif errors:
            status = RetrievalStatus.PARTIAL if hits else RetrievalStatus.UNAVAILABLE
        else:
            status = RetrievalStatus.READY
        semantic = self.manager.semantic_state()
        semantic_status = (RetrievalStatus.READY if semantic.get("ready") else
                           RetrievalStatus.DISABLED if not semantic.get("enabled") else
                           RetrievalStatus.UNAVAILABLE)
        channel_statuses = {RetrievalChannel.LEXICAL: (
            RetrievalStatus.READY if enabled and not errors else
            RetrievalStatus.PARTIAL if enabled and errors and hits else
            RetrievalStatus.UNAVAILABLE if enabled and errors else
            RetrievalStatus.DISABLED),
            RetrievalChannel.SEMANTIC: semantic_status}
        if (enabled and RetrievalChannel.SEMANTIC in supported_channels and
                semantic_status == RetrievalStatus.UNAVAILABLE):
            status = RetrievalStatus.PARTIAL if hits else RetrievalStatus.UNAVAILABLE
        return RetrievalResult(status, tuple(hits), reason="storage_unavailable" if errors else "",
                               channel_statuses=channel_statuses)
