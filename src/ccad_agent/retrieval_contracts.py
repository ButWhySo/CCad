"""Backend-independent, typed contracts shared by local retrieval channels."""

from __future__ import annotations

import math
from copy import deepcopy
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Mapping, Protocol


class RetrievalChannel(str, Enum):
    EXACT = "exact"
    LEXICAL = "lexical"
    SEMANTIC = "semantic"
    GRAPH = "graph"
    SPATIAL = "spatial"


class RetrievalStatus(str, Enum):
    READY = "ready"
    PARTIAL = "partial"
    DISABLED = "disabled"
    UNAVAILABLE = "unavailable"
    STALE = "stale"
    FAILED = "failed"


_DEFAULT_CHANNELS = tuple(RetrievalChannel)
_MAX_REQUEST_QUERY = 3072
_MAX_FILTER_VALUES = 128


def _strings(name: str, values, *, limit: int = _MAX_FILTER_VALUES) -> tuple[str, ...]:
    if isinstance(values, (str, bytes)):
        raise ValueError(f"{name}_must_be_a_sequence")
    try:
        result = tuple(dict.fromkeys(values))
    except TypeError as error:
        raise ValueError(f"{name}_must_be_a_sequence") from error
    if len(result) > limit or any(not isinstance(item, str) or not item.strip() or
                                  len(item) > 256 for item in result):
        raise ValueError(f"{name}_invalid")
    return tuple(item.strip() for item in result)


@dataclass(frozen=True)
class RetrievalRequest:
    """Bounded domain-neutral query; ``fields`` limits returned source fields."""

    query: str
    project_id: str | None = None
    thread_id: str | None = None
    requested_revision: str | None = None
    scope: str = "project"
    entity_types: tuple[str, ...] = ()
    fields: tuple[str, ...] = ()
    bbox: tuple[float, float, float, float] | None = None
    net_ids: tuple[str, ...] = ()
    layer_ids: tuple[str, ...] = ()
    selected_object_ids: tuple[str, ...] = ()
    top_k: int = 10
    candidate_budget: int = 100
    token_budget: int | None = None
    channels: tuple[RetrievalChannel | str, ...] = _DEFAULT_CHANNELS
    include_provenance: bool = True

    def __post_init__(self):
        if not isinstance(self.query, str) or len(self.query) > _MAX_REQUEST_QUERY:
            raise ValueError("retrieval_query_invalid")
        object.__setattr__(self, "query", self.query.strip())
        for name in ("project_id", "thread_id", "requested_revision"):
            value = getattr(self, name)
            if value is not None and (not isinstance(value, str) or len(value) > 256):
                raise ValueError(f"retrieval_{name}_invalid")
        if not isinstance(self.scope, str) or not self.scope.strip() or len(self.scope) > 64:
            raise ValueError("retrieval_scope_invalid")
        object.__setattr__(self, "scope", self.scope.strip())
        for name in ("entity_types", "fields", "net_ids", "layer_ids",
                     "selected_object_ids"):
            object.__setattr__(self, name, _strings(name, getattr(self, name)))
        if self.bbox is not None:
            if len(self.bbox) != 4 or any(isinstance(value, bool) or
                                          not isinstance(value, (int, float)) or
                                          not math.isfinite(value) for value in self.bbox):
                raise ValueError("retrieval_bbox_invalid")
            x1, y1, x2, y2 = self.bbox
            if x1 > x2 or y1 > y2:
                raise ValueError("retrieval_bbox_invalid")
            object.__setattr__(self, "bbox", tuple(float(value) for value in self.bbox))
        if (isinstance(self.top_k, bool) or not isinstance(self.top_k, int) or
                not 1 <= self.top_k <= 512):
            raise ValueError("retrieval_top_k_invalid")
        if (isinstance(self.candidate_budget, bool) or
                not isinstance(self.candidate_budget, int) or
                not self.top_k <= self.candidate_budget <= 100_000):
            raise ValueError("retrieval_candidate_budget_invalid")
        if (self.token_budget is not None and
                (isinstance(self.token_budget, bool) or
                 not isinstance(self.token_budget, int) or
                 not 1 <= self.token_budget <= 32_768)):
            raise ValueError("retrieval_token_budget_invalid")
        channels = tuple(dict.fromkeys(
            channel if isinstance(channel, RetrievalChannel) else RetrievalChannel(channel)
            for channel in self.channels))
        if not channels:
            raise ValueError("retrieval_channels_empty")
        object.__setattr__(self, "channels", channels)
        if not isinstance(self.include_provenance, bool):
            raise ValueError("retrieval_provenance_flag_invalid")


@dataclass(frozen=True)
class RetrievalHit:
    canonical_id: str
    source_type: str
    source_scope: str
    source_revision: str
    channel: RetrievalChannel
    channel_rank: int
    content_handle: str
    channel_score: float | None = None
    normalized_features: Mapping[str, Any] = field(default_factory=dict)
    matched_terms: tuple[str, ...] = ()
    semantic_similarity: float | None = None
    relationship_path: tuple[str, ...] = ()
    spatial_distance: float | None = None
    provenance: Mapping[str, Any] = field(default_factory=dict)
    text_preview: str | None = None
    content: Mapping[str, Any] = field(default_factory=dict, repr=False, compare=False)

    def __post_init__(self):
        for name in ("canonical_id", "source_type", "source_scope", "content_handle"):
            value = getattr(self, name)
            if not isinstance(value, str) or not value.strip() or len(value) > 512:
                raise ValueError(f"retrieval_hit_{name}_invalid")
            object.__setattr__(self, name, value.strip())
        if not isinstance(self.source_revision, str) or len(self.source_revision) > 256:
            raise ValueError("retrieval_hit_revision_invalid")
        if not isinstance(self.channel, RetrievalChannel):
            object.__setattr__(self, "channel", RetrievalChannel(self.channel))
        if isinstance(self.channel_rank, bool) or not isinstance(self.channel_rank, int) or \
                self.channel_rank < 1:
            raise ValueError("retrieval_hit_rank_invalid")
        for name in ("channel_score", "semantic_similarity", "spatial_distance"):
            value = getattr(self, name)
            if value is not None and (isinstance(value, bool) or
                                      not isinstance(value, (int, float)) or
                                      not math.isfinite(value)):
                raise ValueError(f"retrieval_hit_{name}_invalid")
        if self.semantic_similarity is not None and not -1.0 <= self.semantic_similarity <= 1.0:
            raise ValueError("retrieval_hit_semantic_similarity_invalid")
        if self.spatial_distance is not None and self.spatial_distance < 0:
            raise ValueError("retrieval_hit_spatial_distance_invalid")
        if self.text_preview is not None and (not isinstance(self.text_preview, str) or
                                               len(self.text_preview) > 480):
            raise ValueError("retrieval_hit_text_preview_invalid")
        object.__setattr__(self, "matched_terms", _strings(
            "matched_terms", self.matched_terms, limit=96))
        object.__setattr__(self, "relationship_path", _strings(
            "relationship_path", self.relationship_path, limit=32))
        if not isinstance(self.normalized_features, Mapping) or \
                not isinstance(self.provenance, Mapping) or not isinstance(self.content, Mapping):
            raise ValueError("retrieval_hit_mapping_invalid")
        object.__setattr__(self, "normalized_features", deepcopy(dict(self.normalized_features)))
        object.__setattr__(self, "provenance", deepcopy(dict(self.provenance)))
        object.__setattr__(self, "content", deepcopy(dict(self.content)))


@dataclass(frozen=True)
class RetrievalResult:
    status: RetrievalStatus
    hits: tuple[RetrievalHit, ...] = ()
    revision: str = ""
    reason: str = ""
    channel_statuses: Mapping[RetrievalChannel, RetrievalStatus] = field(default_factory=dict)
    metadata: Mapping[str, Any] = field(default_factory=dict)

    def __post_init__(self):
        if not isinstance(self.status, RetrievalStatus):
            object.__setattr__(self, "status", RetrievalStatus(self.status))
        hits = tuple(self.hits)
        if any(not isinstance(hit, RetrievalHit) for hit in hits):
            raise ValueError("retrieval_result_hit_invalid")
        object.__setattr__(self, "hits", hits)
        if not isinstance(self.revision, str) or len(self.revision) > 256:
            raise ValueError("retrieval_result_revision_invalid")
        if not isinstance(self.reason, str) or len(self.reason) > 128:
            raise ValueError("retrieval_result_reason_invalid")
        if not isinstance(self.channel_statuses, Mapping) or not isinstance(self.metadata, Mapping):
            raise ValueError("retrieval_result_mapping_invalid")
        statuses = {}
        for channel, status in self.channel_statuses.items():
            channel = channel if isinstance(channel, RetrievalChannel) else RetrievalChannel(channel)
            statuses[channel] = status if isinstance(status, RetrievalStatus) else RetrievalStatus(status)
        object.__setattr__(self, "channel_statuses", statuses)
        object.__setattr__(self, "metadata", deepcopy(dict(self.metadata)))


class Retriever(Protocol):
    """A channel implementation accepts domain requests and emits canonical hits."""

    channels: frozenset[RetrievalChannel]

    def retrieve(self, request: RetrievalRequest) -> RetrievalResult: ...
