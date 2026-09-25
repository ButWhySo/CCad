"""Build bounded, secret-safe provider context from native CCad state."""

import hashlib
import json
import math
import re
from typing import Any, Iterable


_SECRET = re.compile(
    r"(?:api[_-]?key|secret|password|token)\s*[:=]\s*\S+"
    r"|\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b"
    r"|\bAIza[A-Za-z0-9_-]{20,}", re.IGNORECASE)
_SENSITIVE_PROPERTY = re.compile(
    r"api[_-]?key|secret|password|token|authorization|credential", re.IGNORECASE)
_PREFIX = "[CCAD_CONTEXT_V3]\n"
_RETRIEVAL_SAFE_FIELDS = (
    "rank", "tier", "query_overlap_terms", "bm25_score", "ranking_method",
    "channel_ranks", "rrf_score", "diversity_score", "redundancy_score",
    "matched_terms", "namespace_hash")


def _safe_retrieval_metadata(item: dict, *, include_bm25: bool) -> dict:
    fields = _RETRIEVAL_SAFE_FIELDS if include_bm25 else tuple(
        key for key in _RETRIEVAL_SAFE_FIELDS if key != "bm25_score")
    safe = {key: item[key] for key in fields if key in item}
    for key, ceiling in (("kind_weight", 1.16), ("recency_weight", 1.15),
                         ("usage_weight", 1.10)):
        value = item.get(key)
        if (isinstance(value, (int, float)) and not isinstance(value, bool)
                and math.isfinite(value) and 1.0 <= value <= ceiling):
            safe[key] = value
    persistence = item.get("usage_persistence")
    if persistence in {"durable", "process", "process_only", "unavailable"}:
        safe["usage_persistence"] = persistence
    return safe


def _safe_text(value: Any, limit: int) -> str:
    text = str(value or "").strip()
    if _SECRET.search(text):
        return ""
    return text[:max(0, limit)]


def _project_payload(raw_context: Any) -> tuple[Any, str]:
    text = str(raw_context or "").strip()
    if not text:
        return {}, ""
    try:
        decoded = json.loads(text)
    except json.JSONDecodeError:
        return {"native_context_text": _safe_text(text, 24000)}, ""
    if not isinstance(decoded, dict):
        return {"native_context_text": _safe_text(text, 24000)}, ""
    revision = str(decoded.get("revision") or "")
    return decoded, revision


def _memory_payload(entries: Iterable[dict], per_entry_limit: int = 1000) -> list[dict]:
    result = []
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        kind = entry.get("kind", "fact")
        if kind not in {"fact", "preference", "correction"}:
            continue
        content = _safe_text(entry.get("content"), per_entry_limit)
        if not content:
            continue
        result.append({
            "id": _safe_text(entry.get("id"), 100),
            "title": _safe_text(entry.get("title"), 200),
            "tier": _safe_text(entry.get("tier", "ltm"), 32),
            "kind": kind,
            "scope": _safe_text(entry.get("scope", "project"), 100),
            "tags": [_safe_text(tag, 60) for tag in entry.get("tags", [])[:20]
                     if _safe_text(tag, 60)],
            "content": content,
        })
    return result


def _memory_summary(entries: Iterable[dict], supplied: str = "") -> str:
    """Create a bounded, deterministic summary from only included safe entries."""
    if supplied:
        text = _safe_text(supplied, 1200)
        if text:
            return text
    parts = []
    for entry in entries:
        title = _safe_text(entry.get("title"), 80)
        content = _safe_text(entry.get("content"), 180)
        if content:
            kind = entry.get("kind", "fact")
            prefix = {"preference": "User preference: ",
                      "correction": "User correction: ",
                      "fact": "Known fact: "}.get(kind, "")
            parts.append(prefix + (f"{title}: " if title else "") + content)
    return "; ".join(parts)[:1200]


def _memory_manifest(value: dict | None) -> dict:
    """Allowlist compact availability facts; never accept manifest content."""
    source = value if isinstance(value, dict) else {}
    source_tiers = source.get("tiers", {})
    tiers = {}
    for tier in ("stm", "ltm", "episodic"):
        item = source_tiers.get(tier, {}) if isinstance(source_tiers, dict) else {}
        if not isinstance(item, dict):
            continue
        tiers[tier] = {
            "enabled": bool(item.get("enabled", False)),
            "available_count": max(0, int(item.get("available_count", 0) or 0)),
            "loaded_count": max(0, int(item.get("loaded_count", 0) or 0)),
            "scope": _safe_text(item.get("scope"), 32),
        }
    return {"version": 1, "tiers": tiers,
            "available_tier_count": max(0, int(source.get("available_tier_count", 0) or 0)),
            "historical_thread_summary_count": max(
                0, int(source.get("historical_thread_summary_count", 0) or 0)),
            "project_scope_available": bool(source.get("project_scope_available", False)),
            "project_memory_count": (max(0, int(source["project_memory_count"]))
                                     if isinstance(source.get("project_memory_count"), int)
                                     else None),
            "semantic_retrieval_ready": bool(source.get("semantic_retrieval_ready", False)),
            "contents_included": False}


def _project_retrieval_payload(value: dict | None) -> dict:
    """Copy a bounded allowlist of typed project search results into context."""
    source = value if isinstance(value, dict) else {}
    entities = []
    used = 0
    for entity in source.get("entities", [])[:12]:
        if not isinstance(entity, dict):
            continue
        item: dict[str, Any] = {}
        for key in ("id", "kind", "reference", "value", "name", "part", "sheet_path",
                    "pin_name", "pin_number", "type", "net_id", "membership_kind", "layer_id",
                    "provenance", "source_group_id", "source_sheet_id", "source_revision",
                    "member_count",
                    "start_layer_id", "end_layer_id",
                    "component_id", "symbol_id", "position_mm", "bounds_mm", "retrieval",
                    "rank", "relationship", "distance_mm", "semantic_similarity",
                    "design_rules"):
            if key in entity and isinstance(entity[key], (str, int, float, dict)):
                item[key] = entity[key]
        for key in ("description", "library_description", "footprint_name", "lib_id",
                    "title", "text", "notes", "target"):
            if key in entity:
                content = _safe_text(entity[key], 480)
                if content:
                    item[key] = content
        for key in ("members", "source_member_ids", "related_net_ids"):
            values = entity.get(key)
            if isinstance(values, list):
                item[key] = [clean for raw in values[:64]
                             if (clean := _safe_text(raw, 120))]
        sheet_path = entity.get("sheet_path")
        if isinstance(sheet_path, str):
            normalized_path = sheet_path.replace("\\", "/")
            if normalized_path.startswith("/") or re.match(r"^[A-Za-z]:", normalized_path):
                item.pop("sheet_path", None)
            else:
                item["sheet_path"] = normalized_path[:240]
        properties = entity.get("properties")
        if isinstance(properties, list):
            safe_properties = []
            for prop in properties[:32]:
                if not isinstance(prop, dict):
                    continue
                name = _safe_text(prop.get("name"), 80)
                property_value = _safe_text(prop.get("value"), 160)
                if not name or not property_value or _SENSITIVE_PROPERTY.search(name):
                    continue
                safe_property = {"name": name, "value": property_value}
                if isinstance(prop.get("visible"), bool):
                    safe_property["visible"] = prop["visible"]
                safe_properties.append(safe_property)
            if safe_properties:
                item["properties"] = safe_properties
        layer_ids = entity.get("layer_ids")
        if isinstance(layer_ids, list):
            safe_layers = [_safe_text(value, 120) for value in layer_ids[:16]
                           if isinstance(value, str) and _safe_text(value, 120)]
            if safe_layers:
                item["layer_ids"] = list(dict.fromkeys(safe_layers))
        if isinstance(entity.get("relationships"), list):
            item["relationships"] = [_safe_text(value, 40) for value in
                                      entity["relationships"][:8]
                                      if isinstance(value, str)]
        encoded_size = len(json.dumps(item, ensure_ascii=False, separators=(",", ":")))
        if not item or used + encoded_size > 6000:
            continue
        entities.append(item)
        used += encoded_size
    stats = source.get("stats", {})
    stats = stats if isinstance(stats, dict) else {}
    stats_payload = {key: max(0, int(stats.get(key, 0) or 0)) for key in
                     ("total_entities", "exact_match_count", "lexical_match_count",
                     "relationship_match_count", "spatial_match_count",
                     "near_component_match_count", "region_member_match_count",
                     "semantic_match_count",
                     "omitted_count")}
    for relation, key in (("near_component", "near_component_match_count"),
                          ("region_member", "region_member_match_count")):
        stats_payload[key] = sum(
            relation in entity.get("relationships", ()) or
            entity.get("relationship") == relation for entity in entities)
    stats_payload["semantic_match_count"] = sum(
        isinstance(entity.get("semantic_similarity"), (int, float))
        for entity in entities)
    return {
        "available": bool(source.get("available", False)),
        "revision": _safe_text(source.get("revision"), 32),
        "search_method": _safe_text(source.get("search_method"), 80),
        "semantic_status": _safe_text(source.get("semantic_status"), 48) or "disabled",
        "relationship_semantics": "shared_net_association_only",
        "board_net_semantics":
            "native_net_id_association_not_physical_continuity",
        "logical_net_semantics":
            "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity",
        "spatial_semantics": "axis_aligned_bounds_intersection_or_distance_only",
        "geometry_relationship_semantics":
            "pcb_coordinates_only; near_component_measures_anchor_position_to_footprint_bounds; region_member_means_axis_aligned_bounds_intersection",
        "entities": entities,
        "stats": stats_payload,
    }


def _turn_payload(records: Iterable[dict]) -> list[dict]:
    """Keep retrieved history concise and traceable to durable source messages."""
    result = []
    for record in records:
        if not isinstance(record, dict):
            continue
        sources = [str(value)[:100] for value in record.get("source_message_ids", [])
                   if isinstance(value, str) and value][:32]
        if not sources:
            continue
        summary = {}
        for field, name in (("user_request_summary", "request"),
                            ("assistant_summary", "result"), ("outcome", "outcome")):
            text = _safe_text(record.get(field), 600)
            if text:
                summary[name] = text
        tools = [str(value)[:120] for value in record.get("tool_ids", [])
                 if isinstance(value, str)][:24]
        result.append({"thread_id": _safe_text(record.get("thread_id"), 100),
                       "turn_id": _safe_text(record.get("turn_id"), 100),
                       "source_message_ids": sources, "summary": summary, "tools": tools})
    return result[:12]


def _recap_payload(recap: dict | None) -> dict:
    if not isinstance(recap, dict):
        return {"source_turn_ids": [], "turns": []}
    turns = []
    for item in recap.get("turns", [])[-6:]:
        if not isinstance(item, dict):
            continue
        turn_id = _safe_text(item.get("turn_id"), 100)
        if not turn_id:
            continue
        turns.append({
            "turn_id": turn_id,
            "request": _safe_text(item.get("user_request_summary"), 300),
            "result": _safe_text(item.get("assistant_summary"), 300),
            "outcome": _safe_text(item.get("outcome"), 40),
            "constraints": [_safe_text(value, 180) for value in
                            item.get("explicit_constraints", [])[:8]
                            if _safe_text(value, 180)],
            "tools": [str(value)[:100] for value in item.get("tool_ids", [])[:8]
                      if isinstance(value, str)],
            "entities": item.get("referenced_entities", {}),
            "findings": item.get("important_findings", [])[:4],
            "revision_before": _safe_text(item.get("project_revision_before"), 100),
            "revision_after": _safe_text(item.get("project_revision_after"), 100),
        })
    return {"thread_id": _safe_text(recap.get("thread_id"), 100),
            "source_turn_ids": [str(value)[:100] for value in
            recap.get("source_turn_ids", [])[:6] if isinstance(value, str)],
            "turns": turns}


def _project_counts(project: Any) -> dict[str, int]:
    """Expose counts only; never export native design payload through metadata."""
    if not isinstance(project, dict):
        return {}
    project = project.get("project", project)
    if not isinstance(project, dict):
        return {}
    counts: dict[str, int] = {}
    for key in ("components", "footprints", "pads", "pins", "nets", "tracks",
                "vias", "zones", "layers", "rules", "libraries", "selection"):
        value = project.get(key)
        if isinstance(value, (list, tuple, dict)):
            counts[key] = len(value)
    for key in ("board", "schematic", "tool_state"):
        value = project.get(key)
        if isinstance(value, dict):
            counts[f"{key}_fields"] = len(value)
    return counts


def build_context_package(raw_context: Any, memory_entries: Iterable[dict],
                          history: Iterable[Any], *, char_limit: int,
                          memory_retrieval: Iterable[dict] = (),
                          memory_runtime: dict | None = None,
                          turn_records: Iterable[dict] = (),
                          thread_recap: dict | None = None,
                          memory_summary: str = "",
                          memory_manifest: dict | None = None,
                          turn_context: dict | None = None,
                          project_retrieval: dict | None = None) -> dict:
    """Return actual provider content plus non-content metadata for one turn."""
    limit = min(131072, max(1024, int(char_limit)))
    project, native_revision = _project_payload(raw_context)
    memories = _memory_payload(memory_entries)
    summary = _memory_summary(memories, memory_summary)
    manifest = _memory_manifest(memory_manifest)
    project_matches = _project_retrieval_payload(project_retrieval)
    turn_context = turn_context if isinstance(turn_context, dict) else {}
    prior_turns = _turn_payload(turn_records)
    recap = _recap_payload(thread_recap)
    history_items = list(history)
    project_counts = _project_counts(project)
    project_source_chars = len(json.dumps(
        project, ensure_ascii=False, sort_keys=True, separators=(",", ":"))) if project else 0
    envelope = {
        "schema_version": 3,
        "context_kind": "ccad_provider_context",
        "project": project,
        "memory": memories,
        "memory_summary": summary,
        "memory_manifest": manifest,
        "turn_context": {
            "version": max(0, int(turn_context.get("version", 0) or 0)),
            "change_reason": _safe_text(turn_context.get("change_reason"), 80),
            "signal_digest": _safe_text(turn_context.get("signal_digest"), 24),
        },
        "conversation": {"recent_message_count": len(history_items)},
        "prior_turns": prior_turns,
        "thread_recap": recap,
        "constraints": {
            "read_only_by_default": True,
            "approval_required_for_mutation": True,
            "secret_values_excluded": True,
        },
    }
    if project_matches["entities"]:
        envelope["project_retrieval"] = project_matches
    encode = lambda: _PREFIX + json.dumps(
        envelope, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    encoded = encode()
    truncated = False
    omitted_memory_count = 0
    omitted_turn_count = 0
    omitted_recap_turn_count = 0
    if len(encoded) > limit:
        # Never cut serialized JSON mid-token. If the full design snapshot is
        # too large, send a valid summary and preserve the highest-ranked
        # memories that fit; the prompt directs the model to project.state for
        # precise design details rather than treating damaged JSON as state.
        truncated = True
        if project:
            envelope["project"] = {
                "snapshot_omitted": True,
                "revision": native_revision,
                "source_chars": project_source_chars,
                "object_counts": project_counts,
                "detail_source": "project.state",
            }
            envelope["constraints"]["project_snapshot_omitted"] = True
        envelope["constraints"]["context_truncated"] = True
        # The summary repeats titles only, while entries carry the actual
        # source-backed facts. Keep the manifest's tier availability compact
        # before sacrificing a relevant memory record.
        if len(encoded := encode()) > limit and envelope["memory_summary"]:
            envelope["memory_summary"] = ""
        if len(encoded := encode()) > limit:
            current_manifest = envelope["memory_manifest"]
            envelope["memory_manifest"] = {
                "version": 1,
                "tiers": {tier: {"enabled": item["enabled"],
                                 "available_count": item["available_count"]}
                          for tier, item in current_manifest["tiers"].items()
                          if item["enabled"] or item["available_count"]},
                "available_tier_count": current_manifest["available_tier_count"],
                "historical_thread_summary_count": current_manifest[
                    "historical_thread_summary_count"],
                "project_scope_available": current_manifest[
                    "project_scope_available"],
                "project_memory_count": current_manifest["project_memory_count"],
                "semantic_retrieval_ready": current_manifest[
                    "semantic_retrieval_ready"],
                "contents_included": False,
            }
        if len(encoded) > limit:
            while len(encoded := encode()) > limit and envelope["prior_turns"]:
                envelope["prior_turns"].pop()
                omitted_turn_count += 1
        if omitted_turn_count:
            envelope["constraints"]["omitted_prior_turn_count"] = omitted_turn_count
            encoded = encode()
        if len(encoded) > limit:
            while len(encoded := encode()) > limit and envelope["thread_recap"]["turns"]:
                removed = envelope["thread_recap"]["turns"].pop(0)
                envelope["thread_recap"]["source_turn_ids"].remove(removed["turn_id"])
                omitted_recap_turn_count += 1
        if omitted_recap_turn_count:
            envelope["constraints"]["omitted_thread_recap_turn_count"] = omitted_recap_turn_count
            encoded = encode()
        # Preserve relevant memory ahead of older conversation material: the
        # current request and its retrieved design facts are more actionable.
        while len(encoded := encode()) > limit and envelope["memory"]:
            envelope["memory"].pop()
            omitted_memory_count += 1
        envelope["memory_summary"] = _memory_summary(envelope["memory"])
        if omitted_memory_count:
            envelope["constraints"]["omitted_memory_entry_count"] = omitted_memory_count
            encoded = encode()
        if len(encoded) > limit:
            envelope["memory"] = []
            envelope["memory_summary"] = ""
            omitted_memory_count = len(memories)
            envelope["constraints"]["omitted_memory_entry_count"] = omitted_memory_count
            encoded = encode()
        if len(encoded) > limit:
            while (len(encoded := encode()) > limit and
                   envelope.get("project_retrieval", {}).get("entities")):
                envelope["project_retrieval"]["entities"].pop()
                envelope["project_retrieval"]["stats"]["omitted_count"] += 1
        if len(encoded) > limit:
            raise ValueError("context limit is too small for the safe summary envelope")
    included_retrieval_stats = envelope.get("project_retrieval", {}).get("stats", {})
    if isinstance(included_retrieval_stats, dict):
        included_entities = envelope.get("project_retrieval", {}).get("entities", [])
        for relation, key in (("near_component", "near_component_match_count"),
                              ("region_member", "region_member_match_count")):
            included_retrieval_stats[key] = sum(
                relation in entity.get("relationships", ()) or
                entity.get("relationship") == relation
                for entity in included_entities if isinstance(entity, dict))
        encoded = encode()
    # Conversation advances every turn; it must not invalidate a project/action
    # revision.  Fall back only to project and retrieved-memory identity.
    revision_material = json.dumps(
        {"project": project, "memory": memories}, ensure_ascii=False,
        sort_keys=True, separators=(",", ":"))
    revision = native_revision or hashlib.sha256(
        revision_material.encode("utf-8")).hexdigest()[:24]
    package_digest = hashlib.sha256(encoded.encode("utf-8")).hexdigest()[:24]
    project_snapshot_omitted = bool(envelope["project"].get("snapshot_omitted", False))
    included_memories = envelope["memory"]
    sources = []
    if project and not project_snapshot_omitted:
        sources.append("project_snapshot")
    elif project_snapshot_omitted:
        sources.append("project_summary")
    included_project_retrieval = envelope.get("project_retrieval", {
        "revision": "", "search_method": "", "entities": [],
        "stats": {"near_component_match_count": 0,
                  "region_member_match_count": 0}})
    project_retrieval_kinds: dict[str, int] = {}
    for entity in included_project_retrieval["entities"]:
        kind = entity.get("kind") if isinstance(entity, dict) else None
        if isinstance(kind, str) and re.fullmatch(r"[a-z_]{1,40}", kind):
            project_retrieval_kinds[kind] = project_retrieval_kinds.get(kind, 0) + 1
    project_retrieval_layer_ids = sorted({
        layer_id for entity in included_project_retrieval["entities"]
        if isinstance(entity, dict)
        for layer_id in entity.get("layer_ids", [])
        if isinstance(layer_id, str) and re.fullmatch(r"[A-Za-z0-9_.-]{1,120}", layer_id)
    }, key=str.casefold)
    if included_project_retrieval["entities"]:
        sources.append("project_retrieval")
    if included_memories:
        sources.append("retrieved_memory")
    if envelope["memory_summary"]:
        sources.append("memory_summary")
    if envelope["memory_manifest"]["tiers"]:
        sources.append("memory_manifest")
    if envelope["prior_turns"]:
        sources.append("retrieved_conversation_turns")
    included_recap = envelope["thread_recap"]
    if included_recap["turns"]:
        sources.append("thread_recap")
    memory_tier_counts: dict[str, int] = {}
    memory_tier_chars: dict[str, int] = {}
    for entry in included_memories:
        tier = entry["tier"] if entry["tier"] in ("stm", "ltm", "episodic") else "other"
        memory_tier_counts[tier] = memory_tier_counts.get(tier, 0) + 1
        memory_tier_chars[tier] = memory_tier_chars.get(tier, 0) + len(json.dumps(
            entry, ensure_ascii=False, sort_keys=True, separators=(",", ":")))
    project_chars = (len(json.dumps(
        envelope["project"], ensure_ascii=False, sort_keys=True,
        separators=(",", ":"))) if project and not project_snapshot_omitted else 0)
    retrieval_by_id = {item.get("entry_id"): item for item in memory_retrieval
                       if isinstance(item, dict)}
    included_retrieval = []
    for entry in included_memories:
        item = retrieval_by_id.get(entry["id"])
        if item is not None:
            included_retrieval.append(_safe_retrieval_metadata(item, include_bm25=True))
    return {
        "content": encoded,
        "metadata": {
            "schema_version": 3,
            "project_revision": revision,
            "content_size": len(encoded),
            "package_digest": package_digest,
            "estimated_token_count": (len(encoded) + 3) // 4,
            "context_limit": limit,
            "truncated": truncated,
            "sources": sources,
            "memory_entry_count": len(included_memories),
            "prior_turn_count": len(envelope["prior_turns"]),
            "omitted_prior_turn_count": omitted_turn_count,
            "thread_recap_turn_count": len(included_recap["turns"]),
            "omitted_thread_recap_turn_count": omitted_recap_turn_count,
            "omitted_memory_entry_count": omitted_memory_count,
            "memory_tier_counts": memory_tier_counts,
            "memory_tier_chars": memory_tier_chars,
            "memory_retrieval": included_retrieval,
            "memory_summary_chars": len(envelope["memory_summary"]),
            "memory_manifest": envelope["memory_manifest"],
            "turn_context_version": envelope["turn_context"]["version"],
            "turn_context_change_reason": envelope["turn_context"]["change_reason"],
            "turn_context_signal_digest": envelope["turn_context"]["signal_digest"],
            "memory_runtime": {
                tier: {
                    "enabled": bool((memory_runtime or {}).get(tier, {}).get("enabled", False)),
                    "runtime_entries": int((memory_runtime or {}).get(tier, {}).get("runtime_entries", 0)),
                    "persistent_entries": int((memory_runtime or {}).get(tier, {}).get("persistent_entries", 0)),
                    "loaded_into_process": bool((memory_runtime or {}).get(tier, {}).get("loaded_into_process", False)),
                    "namespace_hash": str((memory_runtime or {}).get(tier, {}).get("namespace_hash", "")),
                } for tier in ("stm", "ltm", "episodic")},
            "project_snapshot_chars": project_chars,
            "project_summary_chars": (
                len(json.dumps(envelope["project"], ensure_ascii=False,
                               sort_keys=True, separators=(",", ":")))
                if project_snapshot_omitted else 0),
            "project_source_chars": project_source_chars,
            "project_snapshot_omitted": project_snapshot_omitted,
            "history_message_count": len(history_items),
            "history_in_context_package": False,
            "history_sent_as_provider_messages": bool(history_items),
            "project_counts": project_counts,
            "project_retrieval_revision": included_project_retrieval["revision"],
            "project_retrieval_method": included_project_retrieval["search_method"],
            "project_retrieval_semantic_status": included_project_retrieval.get(
                "semantic_status", "disabled"),
            "project_retrieval_stats": included_project_retrieval["stats"],
            "project_retrieval_semantic_count": included_project_retrieval[
                "stats"].get("semantic_match_count", 0),
            "project_retrieval_kinds": project_retrieval_kinds,
            "project_retrieval_count": len(included_project_retrieval["entities"]),
            "project_retrieval_block_net_count": sum(
                entity.get("relationship") == "block_net_member"
                for entity in included_project_retrieval["entities"]
                if isinstance(entity, dict)),
            "project_retrieval_layer_ids": project_retrieval_layer_ids,
            "project_retrieval_layer_count": len(project_retrieval_layer_ids),
            "project_retrieval_chars": len(json.dumps(
                included_project_retrieval["entities"], ensure_ascii=False,
                separators=(",", ":"))),
            "content_emitted": False,
            "secret_value_visible": False,
        },
    }


def _message_payload_size(message: Any) -> tuple[int, int, int, int]:
    """Count text/tool-call payloads and flag multimodal blocks we cannot estimate."""
    content = getattr(message, "content", "")
    if isinstance(content, str):
        text_chars, non_text_blocks = len(content), 0
    elif not isinstance(content, list):
        text_chars, non_text_blocks = len(str(content or "")), 0
    else:
        text_chars = 0
        non_text_blocks = 0
        for block in content:
            if isinstance(block, str):
                text_chars += len(block)
            elif isinstance(block, dict) and isinstance(block.get("text"), str):
                text_chars += len(block["text"])
                if block.get("type") not in (None, "text"):
                    non_text_blocks += 1
            else:
                non_text_blocks += 1
    tool_calls = getattr(message, "tool_calls", [])
    if not isinstance(tool_calls, list):
        tool_calls = []
    try:
        tool_call_chars = (len(json.dumps(tool_calls, ensure_ascii=False,
                                          sort_keys=True, default=lambda _: ""))
                           if tool_calls else 0)
    except (TypeError, ValueError):
        tool_call_chars = 0
    return text_chars, non_text_blocks, tool_call_chars, len(tool_calls)


def _tool_schema_payload(tool: Any) -> dict:
    """Serialize the live LangChain tool's name, description, and input schema."""
    args_schema = getattr(tool, "args_schema", None)
    schema_builder = getattr(args_schema, "model_json_schema", None)
    legacy_schema_builder = getattr(args_schema, "schema", None)
    if callable(schema_builder):
        schema = schema_builder()
    elif callable(legacy_schema_builder):
        schema = legacy_schema_builder()
    elif isinstance(args_schema, dict):
        schema = args_schema
    else:
        schema = {}
    return {"name": str(getattr(tool, "name", "")),
            "description": str(getattr(tool, "description", "")),
            "parameters": schema}


def build_provider_request_report(system_text: str, messages: Iterable[Any],
                                  tools: Iterable[Any], *, provider: str,
                                  model: str, context_content: str,
                                  context_metadata: dict,
                                  model_context_limit: int | None = None,
                                  large_context_threshold: Any = 4096) -> dict:
    """Report safe measurements for the live prompt and bound tool schemas.

    Token counts are deliberately estimates: providers tokenize and wrap these
    same structures differently. The report contains counts and identifiers,
    never prompt text, memory content, tool arguments, or design payloads.
    """
    message_items = list(messages)
    tool_items = list(tools)
    message_chars = 0
    prior_tool_call_chars = 0
    tool_call_count = 0
    non_text_blocks = 0
    for message in message_items:
        chars, blocks, call_chars, call_count = _message_payload_size(message)
        message_chars += chars
        non_text_blocks += blocks
        prior_tool_call_chars += call_chars
        tool_call_count += call_count
    tool_chars = sum(len(json.dumps(_tool_schema_payload(tool), ensure_ascii=False,
                                    sort_keys=True, separators=(",", ":")))
                     for tool in tool_items)
    context_chars = len(context_content)
    full_system_chars = len(system_text)
    context_marker_chars = len("\nContext: ")
    instruction_chars = max(0, full_system_chars - context_chars - context_marker_chars)
    project_chars = min(context_chars, max(0, int(
        context_metadata.get("project_snapshot_chars", 0))))
    project_summary_chars = min(context_chars, max(0, int(
        context_metadata.get("project_summary_chars", 0))))
    memory_chars = min(max(0, context_chars - project_chars), sum(
        max(0, int(size)) for size in
        context_metadata.get("memory_tier_chars", {}).values()))
    context_overhead_chars = max(
        0, context_chars - project_chars - project_summary_chars - memory_chars)
    components = {
        "system_instructions": instruction_chars,
        "project_snapshot": project_chars,
        "project_summary": project_summary_chars,
        "retrieved_memories": memory_chars,
        "context_envelope_and_policy": context_overhead_chars,
        "conversation_messages": message_chars,
        "prior_tool_call_payloads": prior_tool_call_chars,
        "bound_tool_schemas": tool_chars,
    }
    prompt_chars = sum(components.values())
    estimated_tokens = (prompt_chars + 3) // 4 + len(message_items) * 4 + len(tool_items) * 8
    try:
        threshold = max(512, min(100000, int(large_context_threshold)))
    except (TypeError, ValueError):
        threshold = 4096
    tiers = context_metadata.get("memory_tier_counts", {})
    return {
        "schema_version": 1,
        "provider": str(provider),
        "model": str(model),
        "prompt_chars": prompt_chars,
        "estimated_input_tokens": estimated_tokens,
        "estimate_method": "ceil(text_chars/4) + 4 tokens/message + 8 tokens/tool; provider tokenizer unavailable",
        "components": {
            name: {"chars": chars, "estimated_tokens": (chars + 3) // 4}
            for name, chars in components.items()
        },
        "message_count": len(message_items),
        "prior_tool_call_count": tool_call_count,
        "tool_schema_count": len(tool_items),
        "multimodal_blocks_not_estimated": non_text_blocks,
        "estimate_includes_all_payloads": non_text_blocks == 0,
        "context_package_chars": context_chars,
        "context_package_truncated": bool(context_metadata.get("truncated", False)),
        "project_source_chars": int(context_metadata.get("project_source_chars", 0)),
        "project_snapshot_omitted": bool(context_metadata.get("project_snapshot_omitted", False)),
        "memory_entry_count": int(context_metadata.get("memory_entry_count", 0)),
        "omitted_memory_entry_count": int(
            context_metadata.get("omitted_memory_entry_count", 0)),
        "memory_tier_chars": {
            tier: max(0, int(context_metadata.get("memory_tier_chars", {}).get(tier, 0)))
            for tier in ("stm", "ltm", "episodic")},
        "context_package_limit_chars": int(context_metadata.get("context_limit", 0)),
        "context_package_sources": list(context_metadata.get("sources", [])),
        "project_counts": {
            str(key): max(0, int(value)) for key, value in
            context_metadata.get("project_counts", {}).items()
            if isinstance(value, int)},
        "memory_tier_counts": {tier: int(tiers.get(tier, 0))
                                for tier in ("stm", "ltm", "episodic")},
        "memory_retrieval": [
            _safe_retrieval_metadata(item, include_bm25=False)
            for item in context_metadata.get("memory_retrieval", [])
            if isinstance(item, dict)],
        "memory_runtime": {
            tier: {
                "enabled": bool(context_metadata.get("memory_runtime", {}).get(tier, {}).get("enabled", False)),
                "runtime_entries": int(context_metadata.get("memory_runtime", {}).get(tier, {}).get("runtime_entries", 0)),
                "persistent_entries": int(context_metadata.get("memory_runtime", {}).get(tier, {}).get("persistent_entries", 0)),
                "loaded_into_process": bool(context_metadata.get("memory_runtime", {}).get(tier, {}).get("loaded_into_process", False)),
                "namespace_hash": str(context_metadata.get("memory_runtime", {}).get(tier, {}).get("namespace_hash", "")),
            } for tier in ("stm", "ltm", "episodic")},
        "conversation_in_context_package": False,
        "conversation_sent_as_messages": bool(message_items),
        "model_context_limit": model_context_limit if isinstance(model_context_limit, int) and model_context_limit > 0 else None,
        "model_context_limit_source": "authoritative_catalog" if isinstance(model_context_limit, int) and model_context_limit > 0 else "unavailable",
        "estimated_context_fraction": (
            estimated_tokens / model_context_limit
            if isinstance(model_context_limit, int) and model_context_limit > 0 else None),
        "large_context": estimated_tokens >= threshold,
        "large_context_threshold_tokens": threshold,
        "content_emitted": False,
        "secret_value_visible": False,
    }


def format_large_context_explanation(report: dict,
                                     context_metadata: dict,
                                     *, mode: str = "provider") -> str:
    """Explain actual context and memory flow without disclosing their content."""
    components = report["components"]
    component_text = "\n".join(
        f"{name.replace('_', ' ')}: ~{value['estimated_tokens']:,} tokens"
        for name, value in components.items())
    tiers = report["memory_tier_counts"]
    lifecycle = context_metadata.get("memory_runtime", {})
    tier_text = "\n".join(
        f"{tier.upper()}: {'enabled' if lifecycle.get(tier, {}).get('enabled') else 'disabled'}; "
        f"{lifecycle.get(tier, {}).get('runtime_entries', 0)} cached in this process; "
        f"{tiers.get(tier, 0)} retrieved for this request; "
        f"{lifecycle.get(tier, {}).get('persistent_entries', 0)} durable records"
        for tier in ("stm", "ltm", "episodic"))
    retrieval = context_metadata.get("memory_retrieval", [])
    ranking = "; ".join(
        f"#{item['rank']} {item['tier']} overlap={item['query_overlap_terms']}"
        for item in retrieval) or "none"
    limit = report["model_context_limit"]
    limit_text = (f"{limit:,} tokens ({report['model_context_limit_source']})"
                  if limit else "unavailable from current model metadata")
    project_detail = (
        f"Project snapshot omitted ({report['project_source_chars']:,} source chars); "
        f"a counts-only summary with object counts {report['project_counts']} was sent, "
        "so exact design work must first call project.state."
        if report["project_snapshot_omitted"] else
        f"Project snapshot supplied ({report['project_source_chars']:,} source chars).")
    lifecycle_detail = (
        "End-to-end assembly: Qt supplies the active project snapshot with this user "
        "turn and binds its durable session ID, LangGraph thread ID, and project ID before "
        "provider execution. The runtime loads only enabled namespaces (up to 64 records "
        "per durable namespace and 64 records in each of at most 32 cached STM tasks), "
        "removes expired entries during load and retrieval, then ranks matches by query-term overlap, "
        "recency, and tier order, retaining at most 8 by default. Retrieval actually "
        "filters by tier plus namespace; the stored scope label is descriptive metadata, "
        "not a retrieval filter. STM is transient and scoped to an explicit `/task start` "
        "to `/task end` interval; without one, each turn gets an isolated short-lived scope. "
        "At most 32 task scopes are active per process, each retaining up to 64 records. "
        "LTM is durable and "
        "thread-scoped; episodic memory is durable and shared in this OS user's local "
        "memory store across projects. The bounded conversation is sent separately as provider "
        "messages; it is not copied into the project/memory JSON envelope. The system "
        "prompt, that envelope, conversation messages, prior tool-call payloads, and bound "
        "tool schemas are then assembled for the selected model. Memory capture remains "
        "explicit through the memory manager, settings UI, and /memory commands; there is "
        "no automatic learning from ordinary chat. Normalized exact duplicates return "
        "the existing record; high lexical-overlap near-duplicates are rejected with "
        "the matching record ID. Durable namespaces retain at most 64 newest records, secret-bearing "
        "writes are rejected, and unsafe legacy records are excluded from runtime and UI "
        "listing. Turning a tier off immediately unloads it and stops retrieval/writes, "
        "but preserves durable records; scoped delete/reset is a separate confirmed action.")
    heading = (
        f"Local context preview{' (large)' if report['large_context'] else ''} "
        f"for {report['provider']}/{report['model']} "
        "(no provider request was sent): "
        if mode == "preview" else
        f"Large provider request assembled for {report['provider']}/{report['model']}: "
    )
    return (
        heading +
        f"~{report['estimated_input_tokens']:,} estimated input tokens using "
        f"{report['estimate_method']}. Model context limit: {limit_text}.\n\n"
        f"REQUEST COMPONENTS\n{component_text}\n\n"
        f"MEMORY STATE\n{tier_text}\n"
        f"Memory matches use fielded BM25 over titles, content, and tags, reciprocal-rank fusion, "
        f"then bounded maximal-marginal-relevance diversity selection; semantic embeddings are "
        f"not configured. Selected matches: {ranking}. "
        f"{report['memory_entry_count']} included; "
        f"{report['omitted_memory_entry_count']} omitted by package budget.\n\n"
        f"ASSEMBLY LIFECYCLE\n{lifecycle_detail}\n\n"
        f"PROJECT CONTEXT\n{project_detail} Object counts: {report['project_counts']}. "
        f"The envelope is bounded to {report['context_package_limit_chars']:,} chars; "
        f"sources: {report['context_package_sources']}. It contains "
        f"{report['context_package_chars']:,} chars; truncated="
        f"{str(report['context_package_truncated']).lower()}.\n\n"
        "PRIVACY\nThis explanation contains counts and lifecycle metadata only, not prompt, "
        "design, memory, or credential contents.")
