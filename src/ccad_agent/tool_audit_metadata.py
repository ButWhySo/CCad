"""Safe approval provenance transport, separate from model-visible tool content."""

from __future__ import annotations

import json
import uuid
from typing import Any


_CHECKPOINT_OUTPUT_MARKER = "_ccad_tool_output_version"
_AUDIT_ARTIFACT_KEY = "ccad_audit"
_APPROVAL_DECISIONS = frozenset({"approved", "rejected", "cancelled"})


def safe_approval_metadata(value: Any) -> dict[str, Any]:
    """Return only valid opaque IDs and an explicit approval decision."""
    if not isinstance(value, dict) or type(value.get("schema_version")) is not int \
            or value["schema_version"] != 1:
        return {}
    identifiers: dict[str, str] = {}
    for key in ("proposal_id", "approval_id"):
        raw = value.get(key)
        if not isinstance(raw, str) or len(raw) != 36:
            return {}
        try:
            identifier = str(uuid.UUID(raw))
        except (ValueError, AttributeError, TypeError):
            return {}
        identifiers[key] = identifier
    decision = value.get("approval_decision")
    if not isinstance(decision, str) or decision not in _APPROVAL_DECISIONS:
        return {}
    return {"schema_version": 1, **identifiers,
            "approval_decision": decision}


def _json_content(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def tool_output_from_response(response: Any, expected_call_id: str
                              ) -> tuple[str, dict[str, Any]]:
    """Preserve native result as content; put approval provenance in ToolMessage artifact."""
    call_id = str(expected_call_id)
    if (not isinstance(response, dict) or response.get("method") != "tool_result"
            or str(response.get("id", "")) != call_id):
        return _json_content({"error": "invalid_tool_result", "call_id": call_id}), {}
    if response.get("error") is not None:
        content = {"error": response["error"], "call_id": call_id}
    else:
        content = response.get("result", {"error": "empty_broker_result"})
    audit = safe_approval_metadata(response.get("audit"))
    artifact = {_AUDIT_ARTIFACT_KEY: audit} if audit else {}
    return _json_content(content), artifact


def checkpoint_resume_value(result: Any, error: Any, call_id: str,
                            audit: Any) -> dict[str, Any]:
    """Carry broker result and private audit artifact through LangGraph interrupt state."""
    return {
        _CHECKPOINT_OUTPUT_MARKER: 1,
        "call_id": str(call_id),
        "result": result,
        "error": error,
        "audit": safe_approval_metadata(audit),
    }


def checkpointed_tool_output(value: Any) -> tuple[str, dict[str, Any]] | None:
    """Unwrap this process's checkpoint resume envelope into LangChain's content/artifact pair."""
    if (not isinstance(value, dict)
            or type(value.get(_CHECKPOINT_OUTPUT_MARKER)) is not int
            or value[_CHECKPOINT_OUTPUT_MARKER] != 1):
        return None
    response: dict[str, Any] = {
        "method": "tool_result", "id": value.get("call_id", ""),
        "audit": value.get("audit"),
    }
    if value.get("error") is not None:
        response["error"] = value["error"]
    else:
        response["result"] = value.get("result", {"error": "empty_broker_result"})
    return tool_output_from_response(response, str(value.get("call_id", "")))
