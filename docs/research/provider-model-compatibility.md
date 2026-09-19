# Provider/model compatibility record

This file records the provider model IDs used by the CCad Agent Settings
presets. It is deliberately limited to models confirmed in first-party
documentation on 2026-09-18. A preset is only a convenience choice; the
provider remains the authority, and the UI must allow a custom model ID for
OpenAI-compatible endpoints.

## Cerebras

Cerebras' current public model catalog lists production `gpt-oss-120b` and
`qwen-3.8-27b`; availability and limits follow provider documentation. CCad
uses the OpenAI-compatible endpoint `https://api.cerebras.ai/v1`, the
`CEREBRAS_API_KEY` credential, and `CCAD_CEREBRAS_MODEL` for an explicit
override. The adapter performs no network request during configuration or
contract tests, preserving user quota.

## Google Gemini

The Gemini model page currently documents a larger catalog; CCad presents a
curated subset of stable text choices
`gemini-3.8-flash`, `gemini-3.7-flash`, `gemini-3.6-flash`, and
`gemini-3.5-flash`, plus preview `gemini-3.1-pro-preview`, and the documented
Gemini 2.5 text models `gemini-2.5-flash`, `gemini-2.5-flash-lite`, and
`gemini-2.5-pro`. CCad maps `GEMINI_API_KEY` to the Google adapter's
`GOOGLE_API_KEY` environment and passes the selected model ID unchanged.
Preview and legacy IDs must not be silently substituted for a selected stable
ID.

## Anthropic Claude

The current Anthropic model overview documents API IDs; CCad presents a
curated subset:
`claude-opus-5`, `claude-sonnet-5`, and
`claude-haiku-4-5-20251001`. CCad passes the selected ID to
`ChatAnthropic`; it does not concatenate provider labels into model IDs.

## OpenAI

The OpenAI model catalog is the source of truth for OpenAI IDs. CCad's
presets are limited to text-capable IDs used by the chat/tool adapter:
`gpt-5.1`, `gpt-5`, `gpt-5-mini`, `gpt-4.1`, and `gpt-4.1-mini`. Runtime
default is `gpt-5.1`. The UI does
not claim that image, audio, moderation, or deprecated models are compatible
with this text/tool path.

## Failure diagnosis and quota policy

Provider configuration is not a provider health check. CCad's no-network
contract tests only validate adapter construction, endpoint, key alias, and
model propagation. A real request is opt-in and must be initiated by the user.
The orchestrator must not retry authentication, quota, or rate-limit failures;
those errors are surfaced as provider failures without another request. Runtime
messages now classify failures as authentication, model-not-found,
quota/rate-limit, timeout, dependency, or provider-unavailable, without
including exception text that could contain a key or request payload.

Official references: [Cerebras model catalog](https://inference-docs.cerebras.ai/models/overview),
[Gemini models](https://ai.google.dev/gemini-api/docs/models),
[Anthropic models](https://platform.claude.com/docs/en/models/overview), and
[OpenAI models](https://developers.openai.com/api/docs/models/all).

## OpenRouter

OpenRouter is a first-class OpenAI-compatible provider option. CCad uses
`https://openrouter.ai/api/v1`, `OPENROUTER_API_KEY`, and defaults to
`openrouter/auto`; users may type any current OpenRouter model ID. The model
catalog is intentionally not hardcoded because OpenRouter exposes a dynamic
`GET /api/v1/models` catalog with changing providers, pricing, context limits,
and capabilities. CCad does not fetch that catalog during startup or tests.

Official reference: [OpenRouter model API](https://openrouter.ai/docs/api/api-reference/models/get-models).

CCad exposes catalogs through explicit `agent.list_models` with
`provider: "openrouter"` or `provider: "cerebras"`. Both are explicit,
credentialed refreshes; neither runs at startup. Cerebras keeps a documented
snapshot for the initial dropdown, then fetches `https://api.cerebras.ai/v1/models`
when the user presses Refresh models. Method discovery reports this provider
specific network capability. Response retains model ID, display name, context
length, and architecture metadata; keys, prices, prompts, and raw response
payload are not returned. Missing keys and HTTP/URL/timeout/JSON failures are
redacted categories, not raw exception text.
