"""Read-only check for the isolated GUI conversation History/New Chat flow."""

from __future__ import annotations

import json
import re
import sqlite3
import sys
from pathlib import Path


DEFAULT_PROMPT = "Conversation history resume validation."
MARKDOWN_FIXTURE_PROMPT = "Show the persisted Markdown rendering example."
SECRET = re.compile(
    r"(?:api[_-]?key|secret|password|access[_-]?token)\s*[:=]\s*\S+"
    r"|\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b"
    r"|\bgh[pousr]_[A-Za-z0-9_]{20,}\b|\bAIza[A-Za-z0-9_-]{20,}\b"
    r"|\bBearer\s+[A-Za-z0-9._~+/=-]{12,}", re.IGNORECASE)


def main() -> int:
    if len(sys.argv) not in (3, 4):
        raise SystemExit(
            "usage: verify_conversation_history_state.py DATABASE THREAD_ID [EXPECTED_PROMPT]"
        )
    database = Path(sys.argv[1]).resolve()
    thread_id = sys.argv[2]
    expected_prompt = sys.argv[3] if len(sys.argv) == 4 else DEFAULT_PROMPT
    if thread_id == "sprint1031-markdown-fixture" and len(sys.argv) == 3:
        expected_prompt = MARKDOWN_FIXTURE_PROMPT
    if not database.is_file() or not thread_id:
        raise SystemExit("conversation database or active thread is missing")
    connection = sqlite3.connect(database.as_uri() + "?mode=ro", uri=True)
    try:
        thread = connection.execute(
            "SELECT title,message_count FROM threads WHERE thread_id=?", (thread_id,)
        ).fetchone()
        rows = connection.execute(
            "SELECT role,payload_json FROM messages WHERE thread_id=? ORDER BY sequence",
            (thread_id,),
        ).fetchall()
        roles = [role for role, _ in rows]
        payloads = [json.loads(payload) for _, payload in rows]
        text = "\n".join(str(item.get("content", "")) for item in payloads)
        valid = (
            thread is not None
            and thread[0] == expected_prompt
            and int(thread[1]) == 2
            and roles == ["user", "assistant"]
            and expected_prompt in text
            and not SECRET.search(text)
        )
        print(json.dumps({
            "thread_found": thread is not None,
            "messages": len(rows),
            "roles": roles,
            "title_matches_first_prompt": bool(thread and thread[0] == expected_prompt),
            "secret_pattern_found": bool(SECRET.search(text)),
        }, separators=(",", ":")))
        return 0 if valid else 1
    finally:
        connection.close()


if __name__ == "__main__":
    raise SystemExit(main())
