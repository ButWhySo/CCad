# Sprint 1034 — Langfuse v4 readiness

## Repository changes and evidence

The repository now sends direct OTLP/HTTP traces to the Langfuse v4 endpoint
with `x-langfuse-ingestion-version: 4`, Basic Auth, and a host-only validated
base URL. Trace readback uses the installed v4 SDK's Observations API v2,
queries by exact trace ID with a bounded UTC start-time range, follows cursors,
and counts only rows belonging to the requested trace. Normal development-turn
readback is bounded to two seconds; the explicit connection test allows up to
twenty seconds for backend indexing. A successful collector response alone is
not reported as a verified trace. The metadata-only exporter remains the final
privacy gate.

The installed environment resolves `langfuse 4.7.1` and `pydantic 2.13.4`; the
repository declares `langfuse>=4.7.1,<4.8.0` in
`src/ccad_agent/requirements.txt` and has no Python dependency lockfile. Source
inventory found no other deprecated Langfuse trace-read API, evaluator or
dataset/experiment API, or separate export consumer in the repository.

Offline runtime/readback, trace hierarchy, and privacy contracts pass; the
repository-wide Pyright check reports zero diagnostics; Python source compiles;
the Qt/MinGW Release build is up to date and successful; full CTest passes
120/120. Manifest: `artifacts/evidence/sprint-1034-langfuse-v4-compatibility.json`,
SHA-256 `F3905B76E182FE1AC91B803B86E7B4315BDF4E49DFCD301B002360B1AB434752`.
These results prove local code compatibility only, not Cloud project migration
or delivery. No project credentials were requested or used, and no Langfuse
Cloud canary was sent. Browser automation was unavailable in this session, so
the canonical online documentation was checked through web search instead.

## Seven-row readiness report

| Readiness area | Status | Evidence and next step |
|---|---|---|
| Project access | blocked | No authenticated Langfuse project connection is available here. A project owner must configure the target project and CCad vault access; [Langfuse Cloud](https://cloud.langfuse.com). |
| SDK/instrumentation | changed | Python SDK 4.7.1 is within the v4 realtime range; direct OTLP now sends the v4 header, and local endpoint/privacy/hierarchy contracts pass. Confirm ingestion against the target project after access. |
| Trace evaluators | blocked | No evaluator API consumer exists in repository code, but project-side evaluator rules cannot be inspected without access; [Langfuse Evaluations](https://cloud.langfuse.com). |
| Dataset evaluators | blocked | No dataset/experiment API consumer exists in repository code; project datasets and experiment-linked evaluators remain uninspected; [Langfuse Datasets](https://cloud.langfuse.com). |
| Direct APIs | changed | Deprecated trace-detail reads are replaced by time-bounded, paginated Observations API v2 reads. Re-scan any project-owned external integration after access. |
| Exports | blocked | Repository OTLP export code is v4-compatible, but project export destinations, downstream consumers, and cutover state are not accessible; [Langfuse project settings](https://cloud.langfuse.com). |
| Verification/rollback | blocked | Offline contracts and 120/120 CTests pass, but no uniquely tagged Cloud canary, trace-tree inspection, or project rollback trial occurred. Run those steps only after project access and retain the old project export setting until verified. |

Official migration references: [Langfuse v4 OTLP migration](https://langfuse.com/integrations/native/opentelemetry/migration-to-v4), [Observations API v2](https://langfuse.com/docs/api-and-data-platform/features/observations-api), and [masking guidance](https://langfuse.com/docs/observability/features/masking). The v4 API guidance requires bounded time windows for observations queries and the ingestion-version header for realtime OTLP data.
