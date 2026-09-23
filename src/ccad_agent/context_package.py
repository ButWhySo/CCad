"""Build bounded, secret-safe provider context from native CCad state."""

import hashlib
import json
import re
from typing import Any, Iterable


_SECRET = re.compile(
    r"(?:api[_-]?key|secret|password|token)\s*[:=]\s*\S+"
    r"|\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b"
    r"|\bAIza[A-Za-z0-9_-]{20,}", re.IGNORECASE)
_PREFIX = "[CCAD_CONTEXT_V2]\n"


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
        content = _safe_text(entry.get("content"), per_entry_limit)
        if not content:
            continue
        result.append({
            "id": _safe_text(entry.get("id"), 100),
            "title": _safe_text(entry.get("title"), 200),
            "tier": _safe_text(entry.get("tier", "ltm"), 32),
            "scope": _safe_text(entry.get("scope", "project"), 100),
            "tags": [_safe_text(tag, 60) for tag in entry.get("tags", [])[:20]
                     if _safe_text(tag, 60)],
            "content": content,
        })
    return result


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
                          memory_runtime: dict | None = None) -> dict:
    """Return actual provider content plus non-content metadata for one turn."""
    limit = min(131072, max(1024, int(char_limit)))
    project, native_revision = _project_payload(raw_context)
    memories = _memory_payload(memory_entries)
    history_items = list(history)
    project_counts = _project_counts(project)
    project_source_chars = len(json.dumps(
        project, ensure_ascii=False, sort_keys=True, separators=(",", ":"))) if project else 0
    envelope = {
        "schema_version": 2,
        "context_kind": "ccad_provider_context",
        "project": project,
        "memory": memories,
        "conversation": {"recent_message_count": len(history_items)},
        "constraints": {
            "read_only_by_default": True,
            "approval_required_for_mutation": True,
            "secret_values_excluded": True,
        },
    }
    encode = lambda: _PREFIX + json.dumps(
        envelope, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    encoded = encode()
    truncated = False
    omitted_memory_count = 0
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
        while len(encoded := encode()) > limit and envelope["memory"]:
            envelope["memory"].pop()
            omitted_memory_count += 1
        if omitted_memory_count:
            envelope["constraints"]["omitted_memory_entry_count"] = omitted_memory_count
            encoded = encode()
        if len(encoded) > limit:
            envelope["memory"] = []
            omitted_memory_count = len(memories)
            envelope["constraints"]["omitted_memory_entry_count"] = omitted_memory_count
            encoded = encode()
        if len(encoded) > limit:
            raise ValueError("context limit is too small for the safe summary envelope")
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
    if included_memories:
        sources.append("retrieved_memory")
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
            included_retrieval.append({key: item[key] for key in
                                       ("rank", "tier", "query_overlap_terms", "namespace_hash")
                                       if key in item})
    return {
        "content": encoded,
        "metadata": {
            "schema_version": 2,
            "project_revision": revision,
            "content_size": len(encoded),
            "package_digest": package_digest,
            "estimated_token_count": (len(encoded) + 3) // 4,
            "context_limit": limit,
            "truncated": truncated,
            "sources": sources,
            "memory_entry_count": len(included_memories),
            "omitted_memory_entry_count": omitted_memory_count,
            "memory_tier_counts": memory_tier_counts,
            "memory_tier_chars": memory_tier_chars,
            "memory_retrieval": included_retrieval,
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
            {key: item[key] for key in
             ("rank", "tier", "query_overlap_terms", "namespace_hash")
             if key in item}
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
                                     context_metadata: dict) -> str:
    """Explain actual context and memory flow without disclosing their content."""
    components = report["components"]
    component_text = "; ".join(
        f"{name.replace('_', ' ')} ~{value['estimated_tokens']} tokens"
        for name, value in components.items())
    tiers = report["memory_tier_counts"]
    lifecycle = context_metadata.get("memory_runtime", {})
    tier_text = "; ".join(
        f"{tier}: {'on' if lifecycle.get(tier, {}).get('enabled') else 'off'}, "
        f"{lifecycle.get(tier, {}).get('runtime_entries', 0)} loaded, "
        f"{tiers.get(tier, 0)} retrieved"
        for tier in ("stm", "ltm", "episodic"))
    retrieval = context_metadata.get("memory_retrieval", [])
    ranking = ", ".join(
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
    return (
        f"Large provider request assembled for {report['provider']}/{report['model']}: "
        f"~{report['estimated_input_tokens']:,} input tokens ({report['estimate_method']}). "
        f"Breakdown: {component_text}. Memory lifecycle: {tier_text}. "
        f"Retrieved-memory ranking: {ranking}. {lifecycle_detail} {project_detail} "
        f"Included memories: {report['memory_entry_count']}; omitted by package budget: "
        f"{report['omitted_memory_entry_count']}. Project object counts: "
        f"{report['project_counts']}. The package is bounded to "
        f"{report['context_package_limit_chars']:,} chars; project/memory source channels: "
        f"{report['context_package_sources']}. Project context package: "
        f"{report['context_package_chars']:,} chars, "
        f"truncated={str(report['context_package_truncated']).lower()}; "
        f"selected-model context limit: {limit_text}. This diagnostic contains counts "
        "only, not prompt, design, or memory content.")
