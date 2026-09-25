"""Durable raw Agent conversations and derived searchable turn records."""

from __future__ import annotations

import json
import os
import re
import sqlite3
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable

from langchain_core.messages import AIMessage, HumanMessage, ToolMessage
from lexical_retrieval import rank_documents


_WORDS = re.compile(r"[a-z0-9_]{2,}", re.IGNORECASE)
_SECRET_KEY = re.compile(r"api.?key|authorization|credential|password|secret|access.?token|headers", re.I)
_SECRET_VALUE = re.compile(
    r"(?:api[_-]?key|secret|password|access[_-]?token)\s*[:=]\s*\S+"
    r"|\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b"
    r"|\bgh[pousr]_[A-Za-z0-9_]{20,}\b|\bAIza[A-Za-z0-9_-]{20,}\b"
    r"|\bBearer\s+[A-Za-z0-9._~+/=-]{12,}", re.IGNORECASE)
_CONSTRAINT = re.compile(
    r"[^.!?\n]{0,180}\b(?:must|must not|do not|don't|never|avoid|keep|preserve|only|exactly)\b[^.!?\n]{0,180}[.!?]?",
    re.IGNORECASE)


class ConversationStoreError(RuntimeError):
    """Safe persistence category; never includes a path or stored content."""

    def __init__(self, category: str):
        self.category = category
        super().__init__(category)


def conversation_path() -> Path:
    configured = os.environ.get("CCAD_AGENT_CONVERSATION_DB", "").strip()
    if configured:
        return Path(configured)
    app_data = os.environ.get("APPDATA")
    root = Path(app_data) / "CCad" if app_data else Path.home() / ".ccad"
    return root / "agent_conversations.sqlite3"


def _now() -> str:
    return datetime.now(timezone.utc).isoformat()


def _safe(value: Any, key: str = "") -> Any:
    if _SECRET_KEY.search(key):
        return "[REDACTED]"
    if isinstance(value, str):
        return _SECRET_VALUE.sub("[REDACTED]", value)
    if isinstance(value, dict):
        return {str(k): _safe(v, str(k)) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [_safe(item) for item in value]
    if value is None or isinstance(value, (bool, int, float)):
        return value
    return _SECRET_VALUE.sub("[REDACTED]", str(value))


def _role(message: Any) -> str:
    kind = str(getattr(message, "type", ""))
    return {"human": "user", "ai": "assistant", "tool": "tool"}.get(kind, "")


def _message_payload(message: Any, *, assign_id: bool = True) -> dict[str, Any] | None:
    role = _role(message)
    if not role:
        return None
    message_id = str(getattr(message, "id", "") or "")
    if not message_id and assign_id:
        message_id = uuid.uuid4().hex
        try:
            message.id = message_id
        except (AttributeError, TypeError, ValueError):
            pass
    payload: dict[str, Any] = {
        "id": message_id,
        "role": role,
        "content": _safe(getattr(message, "content", ""), "content"),
    }
    if role == "assistant":
        calls = getattr(message, "tool_calls", [])
        if isinstance(calls, list):
            payload["tool_calls"] = _safe(calls, "tool_calls")
    elif role == "tool":
        payload["name"] = _safe(getattr(message, "name", ""), "name")
        payload["tool_call_id"] = str(getattr(message, "tool_call_id", "") or "")
    return payload


def _restore(payload: dict[str, Any]):
    role = payload.get("role")
    common = {"content": payload.get("content", ""), "id": payload.get("id", "")}
    if role == "user":
        return HumanMessage(**common)
    if role == "assistant":
        return AIMessage(**common, tool_calls=payload.get("tool_calls", []))
    if role == "tool":
        return ToolMessage(**common, name=payload.get("name", ""),
                           tool_call_id=payload.get("tool_call_id", ""))
    raise ConversationStoreError("conversation_store_corrupt")


def _content_text(value: Any) -> str:
    if isinstance(value, str):
        return value
    if isinstance(value, list):
        return " ".join(str(item.get("text", "")) for item in value
                        if isinstance(item, dict) and isinstance(item.get("text"), str))
    return ""


def _message_tokens(message: Any) -> int:
    payload = _message_payload(message, assign_id=False) or {}
    text = _content_text(payload.get("content", ""))
    calls = payload.get("tool_calls", [])
    encoded_calls = json.dumps(calls, ensure_ascii=False, separators=(",", ":")) if calls else ""
    return max(1, (len(text) + len(encoded_calls) + 3) // 4 + 4)


def budgeted_history_window(messages: Iterable[Any], *, token_budget: int = 8192,
                            max_messages: int = 24) -> list[Any]:
    """Take newest complete user-turn groups without splitting tool interactions."""
    items = [item for item in messages if _role(item)]
    if not items:
        return []
    budget = max(1, int(token_budget))
    count_limit = max(1, int(max_messages))
    groups: list[list[Any]] = []
    for item in items:
        if _role(item) == "user" or not groups:
            groups.append([item])
        else:
            groups[-1].append(item)
    selected: list[Any] = []
    used = 0
    for index in range(len(groups) - 1, -1, -1):
        group = groups[index]
        group_tokens = sum(_message_tokens(message) for message in group)
        if not selected and (group_tokens > budget or len(group) > count_limit):
            final_message = group[-1]
            if (_role(final_message) == "assistant" and
                    not getattr(final_message, "tool_calls", None) and
                    _role(group[0]) == "user"):
                compacted = [group[0], final_message]
                compacted_tokens = sum(_message_tokens(item) for item in compacted)
                if compacted_tokens <= budget and len(compacted) <= count_limit:
                    group, group_tokens = compacted, compacted_tokens
                elif isinstance(getattr(final_message, "content", None), str):
                    user_tokens = _message_tokens(group[0])
                    remaining_chars = max(0, (budget - user_tokens - 8) * 4)
                    marker = "\n[Assistant response shortened in active history; full transcript is preserved.]"
                    if remaining_chars > len(marker):
                        clipped = final_message.model_copy(update={
                            "content": final_message.content[:remaining_chars - len(marker)] + marker})
                        group = [group[0], clipped]
                        group_tokens = sum(_message_tokens(item) for item in group)
            if group_tokens > budget or len(group) > count_limit:
                raise ValueError("latest_conversation_turn_exceeds_context_budget")
        if used + group_tokens > budget or len(selected) + len(group) > count_limit:
            break
        selected[0:0] = group
        used += group_tokens
    return selected


class ConversationStore:
    """SQLite source of truth for full message history and derived TurnRecords."""

    SCHEMA_VERSION = 2

    def __init__(self, path: str | Path | None = None):
        self.path = Path(path) if path is not None else conversation_path()

    def _connect(self):
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            connection = sqlite3.connect(self.path, timeout=5.0)
            connection.row_factory = sqlite3.Row
            connection.execute("PRAGMA foreign_keys=ON")
            connection.execute("PRAGMA busy_timeout=5000")
            connection.execute("PRAGMA journal_mode=WAL")
            self._initialise(connection)
            return connection
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_store_unavailable") from error
        except OSError as error:
            raise ConversationStoreError("conversation_store_unavailable") from error

    def _initialise(self, db: sqlite3.Connection) -> None:
        version = int(db.execute("PRAGMA user_version").fetchone()[0])
        if version > self.SCHEMA_VERSION:
            raise ConversationStoreError("conversation_store_version_unsupported")
        db.executescript("""
            CREATE TABLE IF NOT EXISTS threads(
                thread_id TEXT PRIMARY KEY, session_id TEXT NOT NULL DEFAULT '',
                project_id TEXT NOT NULL DEFAULT '', title TEXT NOT NULL DEFAULT '',
                created_at TEXT NOT NULL, updated_at TEXT NOT NULL,
                message_count INTEGER NOT NULL DEFAULT 0);
            CREATE TABLE IF NOT EXISTS messages(
                sequence INTEGER PRIMARY KEY AUTOINCREMENT,
                thread_id TEXT NOT NULL REFERENCES threads(thread_id) ON DELETE CASCADE,
                message_id TEXT NOT NULL, turn_id TEXT NOT NULL DEFAULT '',
                role TEXT NOT NULL, payload_json TEXT NOT NULL, created_at TEXT NOT NULL,
                UNIQUE(thread_id, message_id));
            CREATE INDEX IF NOT EXISTS messages_thread_sequence
                ON messages(thread_id, sequence);
            CREATE INDEX IF NOT EXISTS messages_thread_turn
                ON messages(thread_id, turn_id);
            CREATE TABLE IF NOT EXISTS turn_records(
                turn_id TEXT PRIMARY KEY,
                thread_id TEXT NOT NULL REFERENCES threads(thread_id) ON DELETE CASCADE,
                project_id TEXT NOT NULL DEFAULT '', created_at TEXT NOT NULL,
                search_text TEXT NOT NULL, record_json TEXT NOT NULL);
            CREATE INDEX IF NOT EXISTS turn_records_thread_time
                ON turn_records(thread_id, created_at DESC);
            CREATE TABLE IF NOT EXISTS turn_terms(
                thread_id TEXT NOT NULL, term TEXT NOT NULL,
                turn_id TEXT NOT NULL REFERENCES turn_records(turn_id) ON DELETE CASCADE,
                PRIMARY KEY(thread_id,term,turn_id));
            CREATE INDEX IF NOT EXISTS turn_terms_lookup ON turn_terms(thread_id,term);
            CREATE TABLE IF NOT EXISTS thread_recaps(
                thread_id TEXT PRIMARY KEY REFERENCES threads(thread_id) ON DELETE CASCADE,
                recap_json TEXT NOT NULL, updated_at TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS projections(
                thread_id TEXT PRIMARY KEY REFERENCES threads(thread_id) ON DELETE CASCADE,
                base_sequence INTEGER NOT NULL, messages_json TEXT NOT NULL,
                updated_at TEXT NOT NULL);
        """)
        db.execute(f"PRAGMA user_version={self.SCHEMA_VERSION}")

    def append_messages(self, thread_id: str, messages: Iterable[Any], *, turn_id="",
                        session_id="", project_id="") -> int:
        thread_id = str(thread_id).strip()
        if not thread_id:
            raise ValueError("thread_id_required")
        prepared = []
        for message in messages:
            payload = _message_payload(message)
            if payload is not None and payload["id"]:
                prepared.append((payload, str(turn_id or "")))
        if not prepared:
            return 0
        now = _now()
        db = self._connect()
        try:
            with db:
                db.execute("""INSERT INTO threads(thread_id,session_id,project_id,created_at,updated_at)
                    VALUES(?,?,?,?,?) ON CONFLICT(thread_id) DO UPDATE SET
                    session_id=CASE WHEN excluded.session_id='' THEN threads.session_id ELSE excluded.session_id END,
                    project_id=CASE WHEN excluded.project_id='' THEN threads.project_id ELSE excluded.project_id END,
                    updated_at=excluded.updated_at""",
                    (thread_id, str(session_id or ""), str(project_id or ""), now, now))
                inserted = 0
                for payload, item_turn in prepared:
                    cursor = db.execute("""INSERT OR IGNORE INTO messages
                        (thread_id,message_id,turn_id,role,payload_json,created_at)
                        VALUES(?,?,?,?,?,?)""", (thread_id, payload["id"], item_turn,
                            payload["role"], json.dumps(payload, ensure_ascii=False,
                            separators=(",", ":")), now))
                    inserted += cursor.rowcount
                    if inserted and payload["role"] == "user":
                        title = _content_text(payload.get("content", "")).strip().splitlines()[0][:100]
                        db.execute("UPDATE threads SET title=CASE WHEN title='' THEN ? ELSE title END WHERE thread_id=?",
                                   (title, thread_id))
                if inserted:
                    db.execute("UPDATE threads SET message_count=message_count+?,updated_at=? WHERE thread_id=?",
                               (inserted, now, thread_id))
                return inserted
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_store_write_failed") from error
        finally:
            db.close()

    def _load_rows(self, db: sqlite3.Connection, thread_id: str, *, limit=None,
                   after_sequence=0):
        query = "SELECT sequence,payload_json FROM messages WHERE thread_id=? AND sequence>? ORDER BY sequence"
        params: list[Any] = [thread_id, int(after_sequence)]
        if limit is not None:
            query = "SELECT sequence,payload_json FROM (SELECT sequence,payload_json FROM messages WHERE thread_id=? AND sequence>? ORDER BY sequence DESC LIMIT ?) ORDER BY sequence"
            params.append(max(1, min(4096, int(limit))))
        return db.execute(query, params).fetchall()

    def load_messages(self, thread_id: str, *, limit=None) -> list[Any]:
        db = self._connect()
        try:
            rows = self._load_rows(db, str(thread_id), limit=limit)
            try:
                return [_restore(json.loads(row["payload_json"])) for row in rows]
            except (json.JSONDecodeError, TypeError, ValueError) as error:
                raise ConversationStoreError("conversation_store_corrupt") from error
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_store_read_failed") from error
        finally:
            db.close()

    def load_model_messages(self, thread_id: str, *, limit=256) -> list[Any]:
        db = self._connect()
        try:
            projection = db.execute("SELECT base_sequence,messages_json FROM projections WHERE thread_id=?",
                                    (str(thread_id),)).fetchone()
            if projection is None:
                rows = self._load_rows(db, str(thread_id), limit=limit)
                payloads = [json.loads(row["payload_json"]) for row in rows]
            else:
                payloads = json.loads(projection["messages_json"])
                remaining = max(0, min(256, int(limit)) - len(payloads))
                tail = (self._load_rows(db, str(thread_id),
                                        limit=remaining,
                                        after_sequence=projection["base_sequence"])
                        if remaining else [])
                payloads.extend(json.loads(row["payload_json"]) for row in tail)
            return [_restore(item) for item in payloads]
        except (sqlite3.Error, json.JSONDecodeError, TypeError, ValueError) as error:
            if isinstance(error, ConversationStoreError):
                raise
            raise ConversationStoreError("conversation_store_read_failed") from error
        finally:
            db.close()

    def compact_projection(self, thread_id: str, messages: Iterable[Any], *, turn_id="") -> None:
        payloads = [_message_payload(item) for item in messages]
        payloads = [item for item in payloads if item is not None]
        if not payloads:
            raise ValueError("projection_requires_messages")
        now = _now()
        db = self._connect()
        try:
            with db:
                thread = db.execute("SELECT 1 FROM threads WHERE thread_id=?", (str(thread_id),)).fetchone()
                if thread is None:
                    raise ValueError("thread_not_found")
                base = db.execute("SELECT COALESCE(MAX(sequence),0) FROM messages WHERE thread_id=?",
                                  (str(thread_id),)).fetchone()[0]
                db.execute("""INSERT INTO projections(thread_id,base_sequence,messages_json,updated_at)
                    VALUES(?,?,?,?) ON CONFLICT(thread_id) DO UPDATE SET
                    base_sequence=excluded.base_sequence,messages_json=excluded.messages_json,
                    updated_at=excluded.updated_at""", (str(thread_id), int(base),
                    json.dumps(payloads, ensure_ascii=False, separators=(",", ":")), now))
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_projection_write_failed") from error
        finally:
            db.close()

    def clear_model_projection(self, thread_id: str) -> None:
        """Reset active model history while retaining the canonical transcript."""
        now = _now()
        db = self._connect()
        try:
            with db:
                db.execute("""INSERT OR IGNORE INTO threads(thread_id,created_at,updated_at)
                    VALUES(?,?,?)""", (str(thread_id), now, now))
                base = db.execute("SELECT COALESCE(MAX(sequence),0) FROM messages WHERE thread_id=?",
                                  (str(thread_id),)).fetchone()[0]
                db.execute("""INSERT INTO projections(thread_id,base_sequence,messages_json,updated_at)
                    VALUES(?,?,?,?) ON CONFLICT(thread_id) DO UPDATE SET
                    base_sequence=excluded.base_sequence,messages_json=excluded.messages_json,
                    updated_at=excluded.updated_at""",
                    (str(thread_id), int(base), "[]", now))
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_projection_write_failed") from error
        finally:
            db.close()

    def turn_id_for_message(self, thread_id: str, message_id: str) -> str:
        db = self._connect()
        try:
            row = db.execute("SELECT turn_id FROM messages WHERE thread_id=? AND message_id=?",
                             (str(thread_id), str(message_id))).fetchone()
            return str(row[0]) if row else ""
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_store_read_failed") from error
        finally:
            db.close()

    def record_turn(self, thread_id: str, turn_id: str, prompt: str,
                    messages: Iterable[Any], *, outcome: str,
                    project_id="", project_revision_before="",
                    project_revision_after="", proposal_id="", transaction_id="",
                    drc_summary=None) -> dict[str, Any]:
        thread_id, turn_id = str(thread_id), str(turn_id)
        if not thread_id or not turn_id:
            raise ValueError("thread_and_turn_ids_required")
        allowed = {"completed", "provider_error", "provider_unavailable",
                   "cancelled", "awaiting_approval", "failed"}
        if outcome not in allowed:
            raise ValueError("turn_outcome_invalid")
        items = list(messages)[-512:]
        tool_ids = []
        for message in items:
            calls = getattr(message, "tool_calls", [])
            if isinstance(calls, list):
                for call in calls:
                    if isinstance(call, dict) and call.get("name"):
                        name = str(_safe(call["name"], "tool_name"))
                        if name not in tool_ids:
                            tool_ids.append(name)
        prompt = str(_safe(str(prompt)[:4096], "prompt"))
        constraints = list(dict.fromkeys(
            match.group(0).strip() for match in _CONSTRAINT.finditer(prompt)))[:20]
        assistant_summary = ""
        for message in reversed(items):
            if _role(message) == "assistant" and not getattr(message, "tool_calls", []):
                assistant_summary = _content_text(_safe(getattr(message, "content", "")))[:600]
                if assistant_summary:
                    break
        db = self._connect()
        try:
            source_ids = [str(row[0]) for row in db.execute(
                "SELECT message_id FROM messages WHERE thread_id=? AND turn_id=? ORDER BY sequence",
                (thread_id, turn_id)).fetchall()]
            recent = [{"id": item, "role": role} for item, role in db.execute(
                "SELECT message_id,role FROM messages WHERE thread_id=? AND turn_id=? ORDER BY sequence",
                (thread_id, turn_id)).fetchall()]
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_store_read_failed") from error
        finally:
            db.close()
        if not source_ids:
            raise ValueError("turn_has_no_persisted_source_messages")
        findings = []
        for message in items[-128:]:
            if _role(message) != "tool":
                continue
            body = _content_text(_safe(getattr(message, "content", "")))[:65536]
            try:
                decoded = json.loads(body)
            except (TypeError, json.JSONDecodeError, RecursionError):
                continue
            if not isinstance(decoded, dict):
                continue
            facts = {key: decoded[key] for key in (
                "status", "error", "error_count", "warning_count", "diagnostic_count",
                "count", "revision", "object_id", "reference", "net", "layer")
                if key in decoded and isinstance(decoded[key], (str, int, float, bool))}
            if facts:
                findings.append({"tool": str(getattr(message, "name", ""))[:120],
                                 "facts": _safe(facts)})
                if len(findings) >= 20:
                    break
        record = {
            "schema_version": 1, "turn_id": turn_id, "thread_id": thread_id,
            "user_request_summary": prompt[:600], "explicit_constraints": constraints,
            "referenced_entities": [], "agent_work_summary": tool_ids,
            "tool_ids": tool_ids, "important_tools_used": tool_ids,
            "important_findings": findings, "important_decisions": [],
            "outcome": outcome, "assistant_summary": assistant_summary,
            "proposal_id": str(proposal_id or ""), "transaction_id": str(transaction_id or ""),
            "project_revision_before": str(project_revision_before or ""),
            "project_revision_after": str(project_revision_after or ""),
            "user_follow_up_summary": "", "unresolved_questions": [],
            "artifact_references": [],
            "drc_summary": _safe(drc_summary or {}), "source_message_ids": source_ids,
            "source_events": recent, "created_at": _now(),
        }
        entities: dict[str, list[str]] = {}
        entity_key = re.compile(r"(reference|ref|net|layer|component|footprint|symbol|pin|pad)$", re.I)
        visited_values = 0
        def collect_entities(value: Any, key: str = "", depth: int = 0) -> None:
            nonlocal visited_values
            visited_values += 1
            if visited_values > 4096 or depth > 8:
                return
            if isinstance(value, dict):
                for child_key, child in value.items():
                    if entity_key.search(str(child_key)) and isinstance(child, (str, int)):
                        kind = str(child_key).casefold()
                        entity = str(child)[:120]
                        entities.setdefault(kind, [])
                        if entity and entity not in entities[kind] and len(entities[kind]) < 32:
                            entities[kind].append(entity)
                    else:
                        collect_entities(child, str(child_key), depth + 1)
            elif isinstance(value, list):
                for child in value[:64]:
                    collect_entities(child, key, depth + 1)
        for message in items:
            collect_entities(getattr(message, "tool_calls", []))
        for value in re.findall(r"\b[A-Z]{1,3}\d{1,5}\b", prompt):
            entities.setdefault("user_reference", [])
            if value not in entities["user_reference"]:
                entities["user_reference"].append(value)
        for value in re.findall(r"\b(?:F|B)\.(?:Cu|Adhes|Paste|SilkS|SilkF|Mask|CrtYd|Fab)\b",
                                prompt, re.IGNORECASE):
            entities.setdefault("user_layer", [])
            if value not in entities["user_layer"]:
                entities["user_layer"].append(value)
        for value in re.findall(r"\b(?:GND|VCC|VDD|VBUS|Net-[A-Za-z0-9_-]+|net\s+[A-Za-z0-9_-]+)\b",
                                prompt, re.IGNORECASE):
            entities.setdefault("user_net", [])
            if value not in entities["user_net"]:
                entities["user_net"].append(value)
        record["referenced_entities"] = entities
        record["search_text"] = " ".join((prompt, " ".join(tool_ids), assistant_summary,
                                            " ".join(constraints),
                                            " ".join(value for values in entities.values()
                                                    for value in values)))
        record["semantic_search_text"] = record["search_text"]
        record["lexical_fields"] = {
            "request": prompt[:600], "tools": tool_ids,
            "entities": entities, "constraints": constraints,
            "outcome": outcome,
        }
        now = _now()
        db = self._connect()
        try:
            with db:
                db.execute("""INSERT INTO turn_records(turn_id,thread_id,project_id,created_at,search_text,record_json)
                    VALUES(?,?,?,?,?,?) ON CONFLICT(turn_id) DO UPDATE SET
                    project_id=excluded.project_id,created_at=excluded.created_at,
                    search_text=excluded.search_text,record_json=excluded.record_json""",
                    (turn_id, thread_id, str(project_id or ""), now,
                     record["search_text"], json.dumps(record, ensure_ascii=False,
                     separators=(",", ":"))))
                db.execute("DELETE FROM turn_terms WHERE turn_id=?", (turn_id,))
                terms = set(_WORDS.findall(record["search_text"].casefold()))
                db.executemany("INSERT OR IGNORE INTO turn_terms(thread_id,term,turn_id) VALUES(?,?,?)",
                               [(thread_id, term, turn_id) for term in terms])
                recent_rows = db.execute("""SELECT record_json FROM turn_records
                    WHERE thread_id=? ORDER BY created_at DESC,turn_id DESC LIMIT 6""",
                    (thread_id,)).fetchall()
                recap = []
                for row in reversed(recent_rows):
                    item = json.loads(row[0])
                    recap.append({key: item.get(key) for key in (
                        "turn_id", "user_request_summary", "assistant_summary", "outcome",
                        "explicit_constraints", "tool_ids", "referenced_entities",
                        "project_revision_before", "project_revision_after")})
                recap_json = json.dumps({"schema_version": 1, "thread_id": thread_id,
                    "source_turn_ids": [item["turn_id"] for item in recap],
                    "turns": recap}, ensure_ascii=False, separators=(",", ":"))
                while len(recap_json) > 8192 and recap:
                    recap.pop(0)
                    recap_json = json.dumps({"schema_version": 1, "thread_id": thread_id,
                        "source_turn_ids": [item["turn_id"] for item in recap],
                        "turns": recap}, ensure_ascii=False, separators=(",", ":"))
                db.execute("""INSERT INTO thread_recaps(thread_id,recap_json,updated_at)
                    VALUES(?,?,?) ON CONFLICT(thread_id) DO UPDATE SET
                    recap_json=excluded.recap_json,updated_at=excluded.updated_at""",
                    (thread_id, recap_json, now))
        except sqlite3.Error as error:
            raise ConversationStoreError("turn_record_write_failed") from error
        finally:
            db.close()
        return record

    def search_turn_records(self, thread_id: str, query: str, *, limit=5) -> list[dict[str, Any]]:
        words = sorted(set(_WORDS.findall(str(query).casefold())))[:32]
        if not words:
            return []
        db = self._connect()
        try:
            placeholders = ",".join("?" for _ in words)
            rows = db.execute(f"""SELECT r.record_json
                FROM turn_terms t JOIN turn_records r ON r.turn_id=t.turn_id
                WHERE t.thread_id=? AND t.term IN ({placeholders})
                GROUP BY r.turn_id ORDER BY r.created_at DESC LIMIT 256""",
                [str(thread_id), *words]).fetchall()
            records = [json.loads(row["record_json"]) for row in rows]
            ranked = rank_documents(query, [{"record": record,
                "text": record.get("search_text", "")} for record in records],
                min_matches=min(2, len(words)))
            result = []
            for item in ranked[:max(0, min(20, int(limit)))]:
                record = item["document"]["record"]
                record["retrieval_overlap_terms"] = len(item["matched_terms"])
                record["retrieval_bm25_score"] = round(item["score"], 6)
                record["retrieval_matched_terms"] = item["matched_terms"]
                result.append(record)
            return result
        except (sqlite3.Error, json.JSONDecodeError, TypeError, ValueError) as error:
            raise ConversationStoreError("turn_record_search_failed") from error
        finally:
            db.close()

    def thread_recap(self, thread_id: str) -> dict[str, Any]:
        db = self._connect()
        try:
            row = db.execute("SELECT recap_json FROM thread_recaps WHERE thread_id=?",
                             (str(thread_id),)).fetchone()
            return json.loads(row[0]) if row else {
                "schema_version": 1, "thread_id": str(thread_id),
                "source_turn_ids": [], "turns": []}
        except (sqlite3.Error, json.JSONDecodeError, TypeError) as error:
            raise ConversationStoreError("thread_recap_unavailable") from error
        finally:
            db.close()

    def search_project_history(self, project_id: str, query: str, *,
                               exclude_thread_id="", limit=4,
                               candidate_limit=500) -> list[dict[str, Any]]:
        """Retrieve source-linked prior turns after a hard project scope filter."""
        project_id = str(project_id or "").strip()
        words = sorted(set(_WORDS.findall(str(query).casefold())))[:32]
        if not project_id or not words:
            return []
        db = self._connect()
        try:
            recaps = db.execute("""SELECT r.thread_id,r.recap_json
                FROM thread_recaps r JOIN threads t ON t.thread_id=r.thread_id
                WHERE t.project_id=? AND r.thread_id<>?
                ORDER BY r.updated_at DESC,r.thread_id LIMIT ?""",
                (project_id, str(exclude_thread_id),
                 max(1, min(500, int(candidate_limit))))).fetchall()
            documents = []
            for row in recaps:
                recap = json.loads(row["recap_json"])
                turns = recap.get("turns", [])
                documents.append({"thread_id": str(row["thread_id"]),
                    "turn_ids": [str(turn.get("turn_id", "")) for turn in turns
                                 if isinstance(turn, dict) and turn.get("turn_id")],
                    "text": " ".join(" ".join(str(turn.get(key, ""))
                        for key in ("user_request_summary", "assistant_summary",
                                    "outcome", "tool_ids", "explicit_constraints",
                                    "referenced_entities")) for turn in turns
                        if isinstance(turn, dict))})
            ranked_threads = rank_documents(query, documents,
                                            min_matches=min(2, len(words)))
            selected_ids = []
            for thread in ranked_threads[:16]:
                for turn_id in thread["document"]["turn_ids"]:
                    if turn_id not in selected_ids:
                        selected_ids.append(turn_id)
            if not selected_ids:
                return []
            placeholders = ",".join("?" for _ in selected_ids)
            rows = db.execute(f"""SELECT record_json FROM turn_records
                WHERE project_id=? AND turn_id IN ({placeholders})""",
                [project_id, *selected_ids]).fetchall()
            records = [json.loads(row[0]) for row in rows]
            ranked_turns = rank_documents(query, [{"record": record,
                "text": record.get("search_text", "")} for record in records],
                min_matches=min(2, len(words)))
            results = []
            for item in ranked_turns[:max(0, min(12, int(limit)))]:
                record = item["document"]["record"]
                record["retrieval_scope"] = "active_project"
                record["retrieval_bm25_score"] = round(item["score"], 6)
                record["retrieval_matched_terms"] = item["matched_terms"]
                results.append(record)
            return results
        except (sqlite3.Error, json.JSONDecodeError, TypeError, ValueError) as error:
            raise ConversationStoreError("project_history_search_failed") from error
        finally:
            db.close()

    def list_threads(self, *, limit=100) -> list[dict[str, Any]]:
        db = self._connect()
        try:
            rows = db.execute("""SELECT thread_id,session_id,project_id,title,created_at,
                updated_at,message_count FROM threads ORDER BY updated_at DESC LIMIT ?""",
                (max(1, min(500, int(limit))),)).fetchall()
            return [dict(row) for row in rows]
        except sqlite3.Error as error:
            raise ConversationStoreError("conversation_store_read_failed") from error
        finally:
            db.close()
