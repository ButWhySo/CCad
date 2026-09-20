# Provider integration audit — 2026-09-19

## Scope and method

This audit covers the providers exposed in Agent Settings: OpenAI, Anthropic,
Google Gemini, OpenRouter, Cerebras, generic OpenAI-compatible endpoints, and
local OpenAI-compatible servers. Each first-party documentation page was loaded
in a headless Playwright session using the locally installed Google Chrome
binary. The audit did not send an inference request, use a user key, or fetch a
private catalog.

## Verified contracts

OpenAI's [Models API reference](https://developers.openai.com/api/reference/resources/models)
documents the models resource. CCad refreshes it only when the user presses
Refresh models, using `GET https://api.openai.com/v1/models` with the session
`OPENAI_API_KEY` as a Bearer credential. The agent runtime uses
`langchain-openai` and `ChatOpenAI` for chat and tool calls.

Anthropic's [List Models API reference](https://docs.anthropic.com/en/api/models-list)
documents `GET https://api.anthropic.com/v1/models`, with `x-api-key` and an
`anthropic-version` header. CCad sends `2023-06-01`, exactly as its catalog
client declares. The [Models overview](https://docs.anthropic.com/en/docs/about-claude/models/overview)
was also loaded to avoid treating the startup Claude preset list as live truth.
The runtime uses `langchain-anthropic` and `ChatAnthropic`.

Google's [Gemini Models documentation](https://ai.google.dev/gemini-api/docs/models)
and [models API reference](https://ai.google.dev/api/models) document
`models.list` and the `v1beta` API family. CCad explicitly refreshes
`https://generativelanguage.googleapis.com/v1beta/models` with
`x-goog-api-key`, normalizes the returned `models/` prefix, and keeps only
models that advertise `generateContent`. The runtime uses
`langchain-google-genai` and `ChatGoogleGenerativeAI`, mapping the UI's
`GEMINI_API_KEY` to the adapter's `GOOGLE_API_KEY` only within the child
process.

OpenRouter's [List all models reference](https://openrouter.ai/docs/api/api-reference/models/list-all-models-and-their-properties)
documents `GET https://openrouter.ai/api/v1/models` with Bearer authentication.
CCad already uses this endpoint only from Refresh models. Chat and tools flow
through OpenRouter's OpenAI-compatible endpoint via `ChatOpenAI`.

Cerebras' [Model Catalog](https://inference-docs.cerebras.ai/models/overview)
and [Chat Completions reference](https://inference-docs.cerebras.ai/api-reference/chat-completions)
were loaded live. The page contained both `gpt-oss-120b` and `qwen-3.8-27b` at
audit time, so neither preset is classified as stale. CCad uses its documented
OpenAI-compatible service with `ChatOpenAI`, and performs authenticated
`GET https://api.cerebras.ai/v1/models` only from Refresh models. Qwen's
reasoning effort is passed explicitly and defaults to `none` to avoid
unnecessary reasoning-token usage.

Generic OpenAI-compatible and local servers do not receive a Refresh models
button. Their catalog endpoint, credential style, and model naming are not
known safely to CCad. They retain a manual model ID and endpoint configuration
instead of a guessed discovery request.

## Runtime boundary and proof

The bundled interpreter is `src/ccad_agent/venv/Scripts/python.exe`. Its pinned
requirements include `langchain-openai`, `langchain-anthropic`, and
`langchain-google-genai`; `pip check` reported no broken requirements and all
three adapters imported successfully during this audit. CCad launches that
interpreter with `PYTHONNOUSERSITE=1`, which prevents unrelated global Python
packages from changing the shipped integration.

`Validate Provider Setup` is intentionally not a connection test. It creates
the selected adapter transiently, restores the previously active adapter, and
emits one selection-specific `provider_test_result` with
`network_access: "not_probed"`. It neither sends a prompt nor consumes quota.
An independent local OpenAI-compatible HTTP stub exercised the live GUI to
LangGraph to approval-gate to tool-result round trip without contacting an
external provider. A future real connection probe must remain a separate,
explicitly confirmed, budget-capped action.

## Failure reporting and selected-provider identity

Provider request failures are classified without returning provider response
bodies, prompts, endpoint details, or credentials. The runtime separates
authentication rejection, permission denial, missing model IDs, exhausted
credit, rate limiting, timeouts, missing dependencies, and generic provider
availability. When an SDK safely exposes an HTTP status, only that numeric
status accompanies the category. Terminal categories do not automatically
retry, preventing accidental extra quota use.

The native panel's local provider-status snapshot is selection-scoped. When
Settings saves a Cerebras key, its hidden provider backing control moves to
Cerebras before any activity event is emitted. The transcript can no longer
claim that OpenAI was refreshed merely because it was the panel's uninitialised
default selection.
