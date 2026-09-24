"""Read-only assertion for the isolated Sprint 976 GUI conversation run."""

import json
import sqlite3
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: verify_conversation_ui_state.py DATABASE THREAD_ID")
    database_path = Path(sys.argv[1]).resolve()
    thread_id = sys.argv[2]
    if not database_path.is_file():
        raise SystemExit("conversation database is missing")
    connection = sqlite3.connect(database_path.as_uri() + "?mode=ro", uri=True)
    try:
        messages = connection.execute(
            "SELECT role, COUNT(*) FROM messages WHERE thread_id=? GROUP BY role",
            (thread_id,)).fetchall()
        counts = {role: count for role, count in messages}
        turn_records = connection.execute(
            "SELECT COUNT(*) FROM turn_records WHERE thread_id=?", (thread_id,)
        ).fetchone()[0]
        projection_row = connection.execute(
            "SELECT messages_json FROM projections WHERE thread_id=?", (thread_id,)
        ).fetchone()
        result = {
            "messages": sum(counts.values()),
            "users": counts.get("user", 0),
            "assistants": counts.get("assistant", 0),
            "turn_records": turn_records,
            "projection": projection_row[0] if projection_row else None,
        }
        if (result["messages"] != 2 or result["users"] != 1 or
                result["assistants"] != 1 or turn_records != 1 or
                result["projection"] != "[]"):
            print(json.dumps(result))
            return 1
        print(json.dumps(result))
        return 0
    finally:
        connection.close()


if __name__ == "__main__":
    raise SystemExit(main())
