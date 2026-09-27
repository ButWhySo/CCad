"""Seed a real isolated conversation-store record for chat-rendering UI proof."""

from __future__ import annotations

from conversation_store import ConversationStore
from langchain_core.messages import AIMessage, HumanMessage


THREAD_ID = "sprint1031-markdown-fixture"
PROMPT = "Show the persisted Markdown rendering example."
RESPONSE = """# Routing review

**Three checks** are relevant before applying this change.

| Check | Result |
| --- | --- |
| Clearance | Pass |
| Net continuity | Review |

- Inspect U3
- [Open the report](https://example.com/report)

```cpp
const bool ready = true;
```
"""


def main() -> int:
    store = ConversationStore()
    store.ensure_thread(THREAD_ID, session_id=THREAD_ID, show_in_history=True)
    inserted = store.append_messages(
        THREAD_ID,
        [HumanMessage(content=PROMPT), AIMessage(content=RESPONSE)],
        session_id=THREAD_ID,
    )
    if inserted != 2:
        raise RuntimeError("markdown_fixture_transcript_not_created")
    print("persisted markdown transcript fixture ready")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
