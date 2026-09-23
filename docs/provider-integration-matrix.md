# CCad provider integration matrix

This matrix records the provider-facing contract checked in the current Tier 1 slice. The model catalog path is explicit refresh only; startup uses the persisted provider/model and does not silently spend network quota. Catalog responses are normalized into CCad model records, and failures are reduced to safe categories without returning response bodies or credentials.

| Provider | Catalog contract | Runtime adapter | Credential | Safe failures |
| --- | --- | --- | --- | --- |
| OpenAI | `GET https://api.openai.com/v1/models`, bearer authentication, `data[]` | LangChain `ChatOpenAI` | `OPENAI_API_KEY` | authentication, permission, payment, rate limit, timeout, connection, invalid response |
| Anthropic | `GET https://api.anthropic.com/v1/models`, `x-api-key`, `anthropic-version`, paginated `data[]` | LangChain `ChatAnthropic` | `ANTHROPIC_API_KEY` | same normalized categories |
| Google Gemini | `GET https://generativelanguage.googleapis.com/v1beta/models`, `x-goog-api-key`, paginated `models[]`, filter `generateContent` | LangChain `ChatGoogleGenerativeAI` | `GEMINI_API_KEY` or `GOOGLE_API_KEY` | same normalized categories |
| OpenRouter | `GET https://openrouter.ai/api/v1/models`, bearer authentication, `data[]` | OpenAI-compatible `ChatOpenAI` with OpenRouter base URL | `OPENROUTER_API_KEY` | same normalized categories |
| Cerebras | public `GET https://api.cerebras.ai/public/v1/models`, `data[]` | OpenAI-compatible `ChatOpenAI` with Cerebras base URL | `CEREBRAS_API_KEY` | same normalized categories |
| Ollama | local `GET {base}/api/tags`, `models[]` | OpenAI-compatible `ChatOpenAI` with local `/v1` base URL | optional local key | same categories, source marked local |

The authoritative references are [OpenAI Models](https://platform.openai.com/docs/api-reference/models), [Anthropic model lifecycle](https://docs.anthropic.com/en/docs/about-claude/model-deprecations), [Gemini Models API](https://ai.google.dev/api/models), [OpenRouter Models API](https://openrouter.ai/docs/api/api-reference/models/get-models), [Cerebras List Models](https://inference-docs.cerebras.ai/api-reference/models/list-models), and [Ollama API](https://docs.ollama.com/api).

The no-network evidence is `scripts/test_provider_integration_contract.py`, the parser/error contracts, and the bounded provider catalog tests. A live request remains opt-in because it spends provider quota and requires the user's current credentials.
