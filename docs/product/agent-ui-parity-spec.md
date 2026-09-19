# CCad Agent UI/UX parity contract

This document records the supplied reference screens as a product contract. It is a target for the native Qt application, not a claim that every surface already exists.

## Information architecture

The Agent experience has a collapsible chat pane. The expanded state shows conversation history, pinned and recent sessions, a project-aware composer, attachment and marketplace actions, model selection, context usage, voice input, and Send. The collapsed state preserves the PCB or schematic canvas and exposes a compact reopen affordance without destroying the active session.

Settings is a separate modeless window with General, Configuration, Personalisation, MCP, Providers & Models, and Plugins sections. General controls chat behaviour (inline or detached), context inclusion, marketplace visibility, voice visibility, and confirmation-before-change. Configuration shows a resolved configuration preview. Personalisation controls agent personality, custom instructions, memory, and workflows. Providers & Models shows provider selection, model selection, model details, a model catalogue action, and protected API-key entry. MCP manages server name, command, arguments, port, and enabled state.

## Agent interaction flow

The normal design flow is: user states a concrete PCB or schematic goal; agent loads project context and reports the evidence; agent runs DRC/ERC or other relevant inspections; agent explains findings; agent prepares a proposed change set; user opens a visual change review; user may inspect PCB diff, schematic diff, or a change list; user approves, revises, or rejects; approval applies one transaction; verification runs again; chat reports the result and offers undo. A proposal must not appear as an unexplained permanent approval card: approval UI is visible only while a pending proposal exists.

## Proposal review

PCB review shows before/after imagery, an object-level change list, selected-object details, old-route overlay, annotations, and approve/revise/reject actions. Revision captures structured constraints such as avoid area, preserve route, use fewer vias, preserve placement, keep widths, and selected-only scope, plus free-form instructions and optional annotations. The revised request remains pending until a new proposal is generated or the user rejects it.

## Visual language

Use a clean neutral desktop theme, strong blue primary actions, restrained status colours, readable cards, consistent spacing, and visible focus/hover states. Native Qt owns layout and security; the agent supplies typed data and actions, never arbitrary HTML. Screens must remain useful at reduced pane width and must not show telemetry chips as primary user content.

## Implementation order

The next bounded slices are collapsible chat pane state, provider/model page polish, settings persistence coverage, proposal review data model and card, revision form, then PCB/schematic diff surfaces. Every slice requires behavior tests, official visual harness execution, screenshot ingestion, feature-specific UI-map and physical input interaction, stderr inspection, documentation, and a full gate before merge.

## Provider compatibility contract

Provider entries are executable only when their adapter, endpoint, credential environment, model identifier, and tool-call protocol are known. OpenRouter uses `OPENROUTER_API_KEY`, base URL `https://openrouter.ai/api/v1`, author/model slugs, and the standard OpenAI chat-completions tool format. Its live catalogue is exposed through `GET /api/v1/models`; the UI must treat that catalogue as refreshable data, not a permanently complete hard-coded list. Models are eligible for CCad agent use only when their metadata supports text output and `tools`; image or audio-only entries are not agent defaults.

Cerebras uses `CEREBRAS_API_KEY`, model override `CCAD_CEREBRAS_MODEL`, base URL `https://api.cerebras.ai/v1`, and the OpenAI-compatible chat interface. The adapter must preserve CCad's bound tool schemas and report a clear unavailable/dependency error when a selected model cannot accept tool calls. Model names shown in the UI must be documented against the provider's current model/API documentation; names alone do not prove compatibility.

The current native Cerebras presets are the first-party public entries
`gpt-oss-120b` and `qwen-3.8-27b`, as documented at
`https://inference-docs.cerebras.ai/models/overview`. This is not an
exhaustive catalogue; live model listing remains explicit and must filter
entries by current text-output and tool-calling metadata, removing stale IDs
rather than silently presenting them as usable.
