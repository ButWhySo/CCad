"""Tier-aware local memory slash-command operations."""

from __future__ import annotations

import shlex


def _metadata(arguments, *, allowed):
    tokens = shlex.split(arguments)
    values = {}
    while tokens and any(tokens[0].startswith(name + ":") for name in allowed):
        key, value = tokens.pop(0).split(":", 1)
        values[key] = value.strip()
    return values, " ".join(tokens).strip()


def execute_memory_command(manager, arguments):
    """Return one safe JSON-RPC event; persistent deletes require explicit scope."""
    command, _, remainder = arguments.strip().partition(" ")
    command = command.lower()
    if command == "list":
        options, trailing = _metadata(remainder, allowed=("tier", "scope"))
        if trailing:
            raise ValueError("unexpected text after memory list filters")
        tier = options.get("tier") or None
        entries = manager.list(tier=tier, scope=options.get("scope"))
        return "memory_state", {"entries": entries, "tier": tier or "all",
                                 "scope": options.get("scope", ""),
                                 "secret_value_visible": False}
    if command == "add":
        options, content = _metadata(remainder, allowed=("tier", "scope", "kind", "title", "importance"))
        try:
            importance = int(options["importance"]) if "importance" in options else None
        except ValueError as error:
            raise ValueError("memory importance must be an integer from 1 to 5") from error
        entry = manager.add(content, tier=options.get("tier", "ltm"),
                             scope=options.get("scope"),
                             title=options.get("title", ""),
                             kind=options.get("kind"), importance=importance)
        return "memory_added", {"id": entry["id"], "tier": entry["tier"],
                                 "scope": entry["scope"], "kind": entry["kind"],
                                 "importance": entry["importance"],
                                 "secret_value_visible": False}
    if command == "update":
        entry_id, _, remainder = remainder.strip().partition(" ")
        options, content = _metadata(remainder, allowed=("scope", "kind", "title", "importance"))
        try:
            importance = int(options["importance"]) if "importance" in options else None
        except ValueError as error:
            raise ValueError("memory importance must be an integer from 1 to 5") from error
        entry = manager.update(entry_id, content,
                               title=options.get("title") if "title" in options else None,
                               scope=options.get("scope") if "scope" in options else None,
                               kind=options.get("kind") if "kind" in options else None,
                               importance=importance)
        return "memory_updated", {"id": entry_id, "updated": entry is not None,
                                  "kind": entry.get("kind", "fact") if entry else "",
                                  "importance": entry.get("importance", 3) if entry else None,
                                  "secret_value_visible": False}
    if command == "delete":
        entry_id = remainder.strip()
        if not entry_id:
            raise ValueError("memory ID required")
        return "memory_deleted", {"removed": manager.delete(entry_id),
                                  "secret_value_visible": False}
    if command == "clear":
        options, trailing = _metadata(remainder, allowed=("tier", "scope"))
        if trailing == "all":
            removed = manager.reset(options.get("tier"))
        elif "scope" in options and not trailing:
            removed = manager.clear_scope(options["scope"], tier=options.get("tier"))
        else:
            raise ValueError("use `/memory clear all` or `/memory clear scope:<name> [tier:<tier>]`")
        return "memory_cleared", {"removed": removed, "secret_value_visible": False}
    raise ValueError("use `/memory list|add|update|delete|clear` with tier/scope filters")
