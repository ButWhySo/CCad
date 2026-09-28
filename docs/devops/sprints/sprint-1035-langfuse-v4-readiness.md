# Sprint 1035 — Langfuse v4 SDK and instrumentation readiness

## Repository work

CCad's declared Langfuse Python SDK floor moved from 4.7.1 to the stable v4.15.6
patch line (`>=4.15.6,<4.16.0`). The resolved Agent environment is Langfuse
4.15.6, Pydantic 2.13.4, and OpenTelemetry OTLP/HTTP exporter 1.42.1. The direct
exporter continues using `/api/public/otel/v1/traces`, Basic Auth, and
`x-langfuse-ingestion-version: 4`; readback continues using time-bounded,
cursor-paginated Observations API v2. The metadata-only exporter remains the
final export privacy boundary.

The local wire test now invokes the real Langfuse LangChain `CallbackHandler`
while a CCad Agent-turn root and session context are active. A local HTTP
receiver decodes the protobuf and verifies the callback is a child of that
root, shares trace/session/name/turn metadata, and is sent through the
configured v4 endpoint without credential bytes. Focused local ingestion,
trace hierarchy, reconfiguration/readback, and privacy contracts pass on
4.15.6; Pyright reports zero diagnostics for the touched runtime/test paths.
These are repository and local-wire checks only. They do not contact a provider
or the user's Langfuse project.

The official Qt/MinGW Release build passed and the complete CTest suite passed
122/122. The verification manifest is
`artifacts/evidence/sprint-1035-langfuse-v4-current-patch.json` (SHA-256
`D599D59D6EE0FE90D04911341030761FA445662A1B6808FDBF83E8164F7E6F1F`). GUI
visual validation was not applicable to this code-only slice; the proposed
PCB/schematic visual-diff behavior remains unimplemented and is explicitly
open in the production TODO.

Official Langfuse documentation states that Python SDK 4.7.0+ and direct OTLP
export with the v4 ingestion header provide the realtime-compatible v4 path;
the November 16, 2026 Cloud cutoff concerns legacy ingestion and deprecated
API/features. Current v4 trace/session metadata propagation and observations
are therefore retained and tested, rather than attempting a redundant data
model conversion.

Desktop browser automation was unavailable in this environment (the computer
surface reported no browsers). Current official Langfuse documentation and the
SDK release page were checked through the web research tool. No project API
keys were requested, loaded, or transmitted.

## Seven-row readiness report

| Readiness area | Status | Evidence and next step |
|---|---|---|
| Project access | blocked | No authenticated target project is connected. Project owner must configure approved project access before any project reads/writes; [Langfuse Cloud](https://cloud.langfuse.com). |
| SDK/instrumentation | changed | Resolved Python SDK 4.15.6; local receiver verified realtime OTLP v4 path/header/auth, safe root, actual LangChain callback child, trace/session propagation, and no credential bytes. A non-production Cloud canary is still required to verify service receipt. |
| Trace evaluators | blocked | Repository contains no evaluator API consumer, but active project rules and legacy trace evaluators were not inspected; verify through the authenticated [Evaluations UI](https://cloud.langfuse.com). |
| Dataset evaluators | blocked | Repository contains no dataset/experiment API consumer, but target datasets and project-side experiment evaluators remain uninspected; verify through [Langfuse Datasets](https://cloud.langfuse.com). |
| Direct APIs | changed | Repository audit found exact-trace readback on the v4 Observations API v2 and no deprecated trace-detail/list or raw legacy ingestion calls. Recheck project-owned external integrations after access. |
| Exports | blocked | Repository OTLP output is v4-compatible; project integrations, destinations, downstream consumers, and their enriched-observation compatibility cannot be inspected here; [project settings](https://cloud.langfuse.com). |
| Verification/rollback | blocked | SDK compatibility, local protobuf delivery, callback hierarchy, runtime/readback, and privacy are locally verified. No uniquely tagged Cloud canary, project trace inspection, export cutover, or project rollback test was performed. |

This report distinguishes code readiness from project state. Project-specific
checks remain blocked until access exists; no Cloud migration or live receipt
is claimed.

## Official references

- [Langfuse v4 migration and timeline](https://langfuse.com/docs/v4)
- [Python SDK v3-to-v4 upgrade guide](https://langfuse.com/docs/observability/sdk/upgrade-path/python-v3-to-v4)
- [OpenTelemetry migration to v4](https://langfuse.com/integrations/native/opentelemetry/migration-to-v4)
- [Langfuse Python SDK releases](https://github.com/langfuse/langfuse-python/releases)
