# Sprint 989 — bounded BM25 memory and history retrieval

## Goal

Implement the first lexical retrieval layer for enabled memory and compact, same-project conversation history. This is M4-A, not completion of the hybrid/semantic M4 group.

## Implementation

The shared `lexical_retrieval.py` module uses deterministic BM25 scoring over bounded documents. Enabled STM, conversation/project LTM, and user episodic entries participate after runtime scope and expiry checks. Conversation TurnRecords use the same scorer. Historical search filters recap rows by exact native project ID before ranking, considers up to 500 recent recaps, expands up to 16 ranked recaps, and then rechecks the project ID on selected source TurnRecords. The active thread is excluded from this cross-thread path; query matches below two terms for multi-term history searches are discarded. The orchestrator limits combined current-thread and same-project historical turns to eight, and the serialized provider context retains thread, turn, and source message IDs.

## Evidence

The eight focused contract scripts all pass: `test_lexical_retrieval.py`, `test_memory_manager.py`, `test_conversation_store.py`, `test_context_budget.py`, `test_context_broker.py`, `test_conversation_runtime.py`, `test_agent_memory_search_tool.py`, and `test_agent_context_package.py`. Changed-module Pyright reports 0 errors/warnings/information. The Qt MinGW Release build succeeded and full CTest passed 114/114. The first CTest attempt exposed a dynamic-import path missing from `test_context_budget.py`; the test loader was corrected, its nine tests passed, and the full suite was rerun clean. No GUI source changed, so no additional screenshots were produced. `git diff --check` is clean. The tracked-repository secret-pattern review found only historical artifact-name matches, intentional synthetic test fixtures, and solder-mask option-name matches; the staged added-line scan found zero matches.

## Remaining M4 work

Embedding backend/readiness/versioning, no-silent-cost policy, hybrid fusion, diversity reranking, importance/recency/usage factors, preference/correction evidence, project-memory summaries, and provider-specific token accounting remain open.
