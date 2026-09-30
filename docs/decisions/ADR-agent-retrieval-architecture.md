# ADR: CCad Agent retrieval architecture and backend boundaries

Status: Accepted for the interface boundary; backend selection remains evidence-gated.

Date: 2026-09-29

## Context

CCad Agent combines five kinds of evidence that must not be confused: exact
identities, text matches, authoritative typed relationships, geometric
proximity, and optional semantic similarity. The typed C++ project model and
its serialized snapshot remain the design authority. Search indexes are
rebuildable derivatives, and no text or vector system may invent board
connectivity, schematic connectivity, or geometric facts.

The current implementation has useful real channels, but `ContextBroker` had
direct dependencies on `ProjectIndex` and `MemoryManager` result shapes. Raw
scores from BM25, cosine similarity, geometric distance, or another engine
cannot be compared as though they shared units. Backend changes must therefore
be isolated from context assembly and provider-facing package schemas.

## Decision

Define canonical `RetrievalRequest`, `RetrievalHit`, `RetrievalResult`, channel,
and readiness-status types. Domain adapters translate the existing project
index and tier-authorized memory manager into those types; `ContextBroker`
consumes the canonical results and converts project hits back into its existing
provider package shape at one compatibility boundary. This is an adapter
boundary, not a claim that the planner, shared fusion, revision-race protocol,
or all five channel implementations are complete.

The logical system remains:

```text
typed project/memory sources
       -> channel retrieval
       -> canonical revisioned hits and truthful status
       -> deterministic fusion and scope validation
       -> ranking/diversity and context budget
       -> ContextBroker
       -> provider-facing package
```

Exact identity is independent of embeddings. Graph/connectivity and spatial
answers come only from typed native data. Semantic retrieval is optional and
supplementary. A failed or disabled optional semantic channel must not disable
exact, lexical, graph, or spatial retrieval. Each result records its canonical
identity, source type and scope, source revision where the source provides
one, channel, channel rank, provenance, and a content handle. Status distinguishes
ready, partial, disabled, unavailable, stale, and failed; absence of results is
not silently reported as a successful search. Memory adapters must enforce the
request's project and thread identities before returning candidates.

`RetrievalRequest.fields` means a requested content projection; adapters retain
identity and retrieval-evidence fields needed to interpret each hit. The
request also bounds query size, filters, top-k, candidate count, and token
budget. Channel scores remain source-local evidence and must not be directly
combined. Fusion belongs in the planned central layer and should use channel
ranks (for example reciprocal-rank fusion), explicit exact/relationship/spatial
features, and recorded source provenance.

The initial backend decision is to keep the current in-process implementations
and not add a search service, FTS5, or ANN dependency until corpus-specific
benchmarks show a requirement the existing code fails. This is the smallest
reversible choice and does not prevent a later backend swap behind canonical
adapters.

## Options and build-versus-reuse comparison

| Option | Capability and license | Runtime, Windows, startup, memory, and disk | Indexing, packaging, failures, and privacy | Sync, tests, rollback, and maintenance |
| --- | --- | --- | --- | --- |
| Existing CCad in-process retrieval | Exact identities, custom deterministic BM25, typed graph/spatial indexes, and bounded exact-cosine semantic ranking already exist. CCad is AGPL-3.0; these modules add no external license. | Same Agent process; no separate daemon or network; works in the current Windows application. Indexes and vector caches are process-local and rebuilt from authoritative typed state; memory and disk are bounded by existing application data and stores, not yet characterized by this ADR. | No new packaging unit. Existing Python/runtime failures can be isolated by channel and reported via status. Local data remains local; optional embeddings are separately configured loopback. | Existing project/memory tests cover behavior, but do not yet compare retrieval quality across backends. Reverting adapter routing is a small code change; rebuilding indexes is already supported. Custom ranking correctness and maintenance remain CCad's responsibility. |
| SQLite FTS5 | Mature full-text search with fielded queries and BM25; SQLite is public domain. FTS5 is included in SQLite amalgamation builds, but actual runtime/build enablement must be probed rather than assumed. | Embedded, in-process, no service or startup handshake; Windows support follows the SQLite build used by the application. Search indexes occupy the database file; exact RAM/storage costs need CCad corpus measurements. | No daemon packaging. FTS query syntax and tokenizer errors require safe translation. External-content tables require the application to keep indexed content consistent and support rebuild/integrity checks. Search stays local. | Transactional update hooks and migration tests are required; current in-process code remains a fallback during rollout. Rollback can discard/rebuild derived indexes. Adds SQLite-specific schema/query and version maintenance. Candidate for a measured lexical comparison only. |
| Typesense | Dedicated full-text, typo-tolerant, faceted, vector and hybrid search service. Server is GPL-3.0; client libraries may use separate licenses, so distribution/legal review is required before adoption. | Separate native service with health/auth/network lifecycle. Official install guidance on Windows uses WSL rather than a native Windows install path. Server maintains an in-memory index and disk copy; RAM depends on indexed fields and data, with extra RAM for built-in semantic models. | Adds service install, launch, upgrade, credentials, readiness, failure recovery and data migration. Local deployment can stay local, but hosted use sends indexed content externally. | Requires synchronization/outbox or rebuild strategy, API integration and service-failure tests. Rollback involves server/index version and migration compatibility. Adds substantial operational and long-term maintenance burden. Not justified for the current single-user desktop project corpus. |
| Exact cosine scan | Reuse current bounded vector caches and deterministic cosine computation; no additional library or license. It solves semantic candidate ranking, not exact CAD identity or text indexing. | In-process and Windows-compatible with the current Python implementation. Compute is proportional to vectors times dimensions; vectors/cache are process-local, while model weights and any external embedding service have separate costs. No persistent vector index. | No extra package or startup process. Embedding errors must retain lexical/exact fallback. Current deployment can keep embeddings on loopback; no vector data is sent to a hosted search service by this choice. | Simple to validate against a brute-force oracle and rebuild. Rollback is straightforward. Must measure actual entity counts and latency before choosing an approximate index; prompt/model changes need cache invalidation and benchmark coverage. |
| USearch ANN | Reusable C++ vector-search engine with exact and approximate search options; upstream identifies Apache-2.0 and Windows support. It solves scale/performance for vectors, not lexical, graph, or spatial authority. | In-process library, but integration boundary is nontrivial because CCad's semantic retrievers currently run in Python. Native ABI/build packaging must match supported Windows toolchains. Memory/index size depends on vectors and graph parameters; persistent-index policy is not selected. | Adds native build/package/update and serialization compatibility. ANN is approximate and must return safe fallback on unavailable or stale indexes. Local execution preserves privacy if embeddings stay local. | Requires recall/latency tests against exact cosine and index rebuild/update/revision tests. Rollback returns to exact scan. Native ABI, upstream releases, serialization formats, and parameter maintenance add burden. Consider only if measured CCad workloads violate an explicit exact-scan latency or memory target. |

## Build-vs-reuse conclusion

No backend replacement is accepted by this ADR. The canonical interfaces are
implemented first so later experiments do not leak backend objects into
orchestration. R3 will establish representative, deterministic corpus and
latency baselines before any decision to reuse FTS5 or USearch. Typesense
requires a stronger product-level need because its desktop deployment, data
synchronization, memory, privacy, license review, and rollback costs are much
higher. Any backend that wins a benchmark must still pass identity, scope,
revision, failure, and provider-package equivalence contracts.

## Consequences and open work

The project index and memory manager now have adapters to the canonical
contract, and ContextBroker routes through them while retaining its established
context package shape. The current adapter exposes channel-local metadata and
source revision without exporting raw backend scores to the provider package.
The typed request includes bounds and project/thread/filter metadata; current
adapters truthfully reject mismatched scopes and project field projection is
supported.

This ADR does not claim central query planning, shared multi-channel fusion,
common revision stamps across all derived indexes, cancellation, historical
revision lookup, ANN support, FTS5 evaluation, or a held-out relevance
benchmark. Those remain owned by the active retrieval TODO. Revisit this ADR
only when measured data or product constraints change.

## Primary references

- [SQLite FTS5 documentation](https://www.sqlite.org/fts5.html)
- [SQLite About and public-domain status](https://sqlite.org/about.html)
- [Typesense install options, including Windows/WSL](https://typesense.org/docs/guide/install-typesense.html)
- [Typesense system requirements and memory model](https://typesense.org/docs/guide/system-requirements.html)
- [Typesense server repository and GPL-3.0 license](https://github.com/typesense/typesense)
- [USearch repository, supported platforms, and Apache-2.0 license](https://github.com/unum-cloud/usearch)
- [CCad repository license](../../LICENSE)
