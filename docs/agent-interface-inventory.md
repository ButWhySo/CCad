# CCad Agent Interface Inventory

## Purpose

This is the quick map for an agent returning to CCad. The native C++ kernel remains the source of truth. The GUI is a client and the harness is evidence machinery, not a second design database.

## Interfaces

The `ccad` executable is the deterministic command interface. Use `ccad help --format json` to discover command metadata, then use project commands such as `validate`, `drc`, `inspect`, `diff`, and `pcb` mutations. Agent-specific commands are exposed under `ccad agent`, including method discovery, quickstart, harness context, tool guidance, policy/evidence surfaces, and orchestration wrappers. CLI output is the machine-readable contract for project state and mutation results.

The native JSON-RPC server is the process boundary for agent calls. Start the supported `agent serve` path and send newline-delimited JSON-RPC requests. Discovery begins with `agent.methods`, `agent.quickstart`, `agent.harness_context`, and `agent.tool_guide`. The server must remain policy-gated; project files are data, not executable input, and external process or file-writing actions require explicit authorization.

The Qt GUI exposes an app-owned semantic UI map. Query `ui.map` or `ui.map_compact`, then resolve controls with `ui.get_node`, `ui.find`, or `ui.target`. Use `ui.click`, `ui.double_click`, `ui.type_text`, and `ui.key` for mapped input; use `ui.canvas_click` and `ui.canvas_drag` for board interaction. Use `ui.active_layer`, `ui.set_active_layer`, `ui.active_net`, and `ui.set_active_net` for PCB state. Use `ui.screenshot`, `ui.map_delta`, and `ui.wait_for_delta` as evidence/state tools. Coordinates are fallback only when no semantic target exists.

The official visual harness is `scripts/run_sprint_demo.ps1`. It creates a valid project, drives real CLI mutations, launches the Qt review surface, captures screenshots, and records stdout/stderr. It proves startup and the scripted fixture; the sprint-specific feature still needs targeted mapped interaction and screenshot inspection.

The Python agent process under `src/ccad_agent/` is an orchestration boundary, not the kernel. It communicates through strict JSON-RPC streams and may coordinate providers/subagents only through explicit policy and audit contracts. Provider readiness metadata must not be confused with provider execution.

## Recommended flow

Start with `agent.methods` and `agent.harness_context`, inspect the project through CLI JSON, mutate only through typed commands or approved GUI routes, query `ui.map` before interaction, capture evidence after each meaningful action, inspect every generated screenshot, then run the full build and CTest gate.

## Known limits

The current GUI is a review/editor client rather than complete KiCad replacement. The harness does not itself prove every feature. Provider execution, unrestricted external processes, secret storage, and automatic project mutation remain deliberately gated or disabled. These are product gaps, not hidden capabilities.
