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
privacy gate. A wire-level contract runs the installed Langfuse SDK/exporter
against an isolated local OTLP receiver, decodes the protobuf request, and
checks endpoint path, v4 header, Basic Auth, root/child parentage, propagated
session and turn metadata, content-free root digest/count, and absence of
credential bytes. This proves local HTTP/protobuf behavior only, not Cloud
delivery or indexing in the target project.

The installed environment resolves `langfuse 4.7.1` and `pydantic 2.13.4`; the
repository declares `langfuse>=4.7.1,<4.8.0` in
`src/ccad_agent/requirements.txt` and has no Python dependency lockfile. Source
inventory found no other deprecated Langfuse trace-read API, evaluator or
dataset/experiment API, or separate export consumer in the repository.

Offline runtime/readback, trace hierarchy, local-wire ingestion, and privacy
contracts pass; repository-wide Pyright reports zero diagnostics; the Qt/MinGW
Release build succeeds and full CTest passes 122/122. Workspace verification
manifest: `artifacts/evidence/sprint-1034-langfuse-v4-wire-contract.json`,
SHA-256 `317EF50D635676B35FFFF9D09E0106302395153A29BD9E3679A18EEF6750965A`.
The manifest is committed; its build/test log files remain workspace-only.
These results prove local code compatibility only, not Cloud project migration
or delivery. No project credentials were requested or used, and no Langfuse
Cloud canary was sent. Desktop browser automation was unavailable; canonical
Langfuse migration and SDK documentation was fetched directly from the official
documentation site.

## Seven-row readiness report

| Readiness area | Status | Evidence and next step |
|---|---|---|
| Project access | blocked | No authenticated Langfuse project connection is available here. A project owner must configure the target project and CCad vault access; [Langfuse Cloud](https://cloud.langfuse.com). |
| SDK/instrumentation | changed | Python SDK 4.7.1 is within the v4 realtime range; a real local OTLP receiver decoded the v4 protobuf and verified path/header/auth, safe root input/output, parentage, and propagated correlation metadata. Confirm ingestion against the target project after access. |
| Trace evaluators | blocked | No evaluator API consumer exists in repository code, but project-side evaluator rules cannot be inspected without access; [Langfuse Evaluations](https://cloud.langfuse.com). |
| Dataset evaluators | blocked | No dataset/experiment API consumer exists in repository code; project datasets and experiment-linked evaluators remain uninspected; [Langfuse Datasets](https://cloud.langfuse.com). |
| Direct APIs | changed | Deprecated trace-detail reads are replaced by time-bounded, paginated Observations API v2 reads. Re-scan any project-owned external integration after access. |
| Exports | blocked | Repository OTLP export code is v4-compatible, but project export destinations, downstream consumers, and cutover state are not accessible; [Langfuse project settings](https://cloud.langfuse.com). |
| Verification/rollback | blocked | Local OTLP wire contract, Qt Release build, and 122/122 CTests pass. No uniquely tagged Cloud canary, trace-tree inspection, or project rollback trial occurred. Run those steps only after project access and retain the old project export setting until verified. |

Official migration references: [Langfuse v4 OTLP migration](https://langfuse.com/integrations/native/opentelemetry/migration-to-v4), [Observations API v2](https://langfuse.com/docs/api-and-data-platform/features/observations-api), and [masking guidance](https://langfuse.com/docs/observability/features/masking). The v4 API guidance requires bounded time windows for observations queries and the ingestion-version header for realtime OTLP data.
