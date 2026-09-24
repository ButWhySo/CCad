# Provider/model compatibility record

This file records provider adapters, model-discovery sources, and model IDs
used by CCad Agent Settings. First-party references were checked on
2026-09-24. A preset is only a convenience choice; the
provider remains the authority, and the UI must allow a custom model ID for
OpenAI-compatible endpoints.

## Cerebras

Cerebras' current public model catalog lists `gpt-oss-120b` and
`qwen-3.8-27b`; both currently report function-calling/tool support. The
public catalog reports maximum contexts of 131,072 and 65,536 tokens
respectively; account-specific limits still come from Cerebras. CCad
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
ID. Gemini's list endpoint reports generation methods but does not by itself
prove that every listed model supports function calling.

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
contracts validate adapter configuration, endpoint, key alias, model
propagation, and safe error translation; they do not prove live inference or
tool execution. A real request is opt-in and must be initiated by the user.
The orchestrator must not retry authentication, quota, or rate-limit failures;
those errors are surfaced as provider failures without another request. Runtime
messages now classify failures as authentication, model-not-found,
payment, quota, rate-limit, timeout, dependency, or provider-unavailable,
without including exception text that could contain a key or request payload.
Google's generic gRPC `RESOURCE_EXHAUSTED` can mean either quota exhaustion or
a rate limit. When structured details do not distinguish the two, CCad reports
`quota_or_rate_limit`, includes a bounded retry-after value if present, and
sends no automatic retry. Explicit quota and rate-limit codes retain their
separate categories.

Official references: [Cerebras model catalog](https://inference-docs.cerebras.ai/models/overview),
[Gemini models](https://ai.google.dev/gemini-api/docs/models),
[Anthropic models](https://platform.claude.com/docs/en/models/overview), and
[OpenAI models](https://developers.openai.com/api/docs/models/all).

## OpenRouter

OpenRouter is a first-class OpenAI-compatible provider option. CCad uses
`https://openrouter.ai/api/v1`, `OPENROUTER_API_KEY`, and defaults to
`openrouter/free`; `openrouter/auto` is not the runtime default. Users may type
any current OpenRouter model ID. The model catalog is intentionally not
hardcoded because OpenRouter exposes a dynamic `GET /api/v1/models` catalog
with changing providers, pricing, context limits, and supported parameters.
CCad does not fetch that catalog during startup or offline tests. The free
router chooses from eligible free models and filters for request features such
as tool use, but manually selected models are not guaranteed to support tools.

Official references: [OpenRouter model API](https://openrouter.ai/docs/api/api-reference/models/get-models), [tool calling](https://openrouter.ai/docs/guides/features/tool-calling), and [free router](https://openrouter.ai/openrouter/free/apps).

CCad exposes catalogs through explicit `agent.list_models` with
`provider: "openrouter"` or `provider: "cerebras"`. Both refreshes are explicit
and neither runs at startup. OpenRouter catalog refresh uses its authenticated
model API. Cerebras uses the public, unauthenticated
`https://api.cerebras.ai/public/v1/models` endpoint. Its model response includes
function-calling/tool capability flags, but runtime does not yet block a
selected model whose metadata says tool use is unsupported. Catalog responses
retain safe model metadata only; keys, prices, prompts, and raw payloads are
not returned.

## Ollama and custom OpenAI-compatible endpoints

Ollama uses the local OpenAI-compatible chat endpoint
`http://127.0.0.1:11434/v1` and explicit local model inventory at `/api/tags`.
CCad does not start Ollama, pull models, or alter the local inventory. Ollama
documents tool calling through this compatibility endpoint for supported
installed models; an installed name alone does not prove tool support.

The generic OpenAI-compatible and local-model adapters use configured base
URLs and model IDs through `ChatOpenAI`. They are transport adapters, not a
claim that every server implements every OpenAI feature. CCad calls
`bind_tools`; server/model capability remains unknown until a live tool request
proves otherwise. Configuration checks do not report model tool-call success.

## Provider tool-call support boundary

Official OpenAI, Anthropic, and Gemini docs describe their native tool/function
calling APIs. Cerebras publishes per-model capability flags and documents
multi-turn tool calls. OpenRouter documents model capability filtering and a
free router that chooses a destination compatible with requested features.
Ollama documents tool calls through its OpenAI-compatible API, subject to the
installed model. Thus offline tests prove CCad's adapter and request contract,
not every account/model/server combination. Only a successful, bounded,
user-authorized tool-call request proves that exact integration end to end.

Additional first-party references: [OpenAI function calling](https://developers.openai.com/api/docs/guides/function-calling), [Anthropic tool use](https://platform.claude.com/docs/en/agents-and-tools/tool-use/overview), [Gemini function calling](https://ai.google.dev/gemini-api/docs/function-calling), [Cerebras tool use](https://inference-docs.cerebras.ai/capabilities/tool-use), [Cerebras public model catalog](https://inference-docs.cerebras.ai/api-reference/models/public-models), [Ollama tool support](https://ollama.com/blog/tool-support), and [Ollama OpenAI compatibility](https://github.com/ollama/ollama/blob/main/docs/api/openai-compatibility.mdx).
