# Sprint 1043 — Retrieval benchmark execution

## Scope

Execute the versioned calibration and held-out corpus through CCad's real
`ProjectIndex`, `MemoryManagerRetriever`, and `ConversationStore` retrieval.
No hosted model is contacted. Output distinguishes measured offline channels
from unavailable or unmeasured model-backed behavior.

## Changes

`scripts/benchmark_agent_retrieval.py` reports per-task Recall@k, Precision@k,
MRR, nDCG, hard-negative outcomes, retrieved-context size and channel/revision
metadata, plus index build/update and query latency, process peak RSS, temporary
store size, and benchmark execution time. Reports include dataset checksum,
software revision, and split. `scripts/test_agent_retrieval_benchmark.py` tests
metric math and execution against the actual retrievers. CMake/CTest and the
`agent-python` CI job run the behavior suite.

## Evidence and limitations

The fixture corpus is controlled and small, so results characterize the
current retrievers on these cases only. Calibration produced one hard-negative
false positive; held-out had one lexical false negative, and its one hard
negative was rejected. Cases permitting semantic search ran with semantic
retrieval disabled in this offline profile; lexical hits are reported as
lexical, not semantic. Product startup latency, separately instrumented
embedding latency, backend failure recovery, and model follow-up tool calls
remain unmeasured. Backend comparisons and R3 closure remain prohibited until
those gaps are measured on representative project data.

No GUI behavior changes; visual interaction validation is not applicable.
Qt/MinGW Release build passed; full CTest passed 126/126; changed-script
Pyright reported zero diagnostics. The official non-visual verifier passed
preflight, build, and CTest. Manifest:
`artifacts/evidence/sprint-1043-retrieval-runner-final.json`, SHA-256
`C2BB3B9652F8AA8DC4B2B9A0B98A0D9F4FDCB4DA67D3EFD19FF8230C61643CC7`.
Calibration/held-out JSON reports and captured logs are workspace-only and are
hash-bound by that manifest.

## Method references

- NIST TREC evaluation measures: <https://trec.nist.gov/pubs/trec11/appendices/MEASURES.pdf>
- Python high-resolution timing: <https://docs.python.org/3/library/time.html>
- Python process resource measurements: <https://docs.python.org/3/library/resource.html>
