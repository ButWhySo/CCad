# Sprint 140: Agent MCP Support

## Goal
Expose standard Model Context Protocol (MCP) methods (`initialize`, `tools/list`, `tools/call`) over the existing JSON-RPC `agent serve` loop, so LLMs can transparently drive `ccad` natively as an MCP server.

## Branch
`sprint-140-agent-mcp`

## Tasks
- Extend `agentCommand` JSON-RPC dispatcher to recognize `initialize` and `tools/list`.
- Return proper MCP server initialization payload.
- Return the `ccad_execute` tool signature.
- Adapt the existing `execute` loop to handle `tools/call` parsing.
- Return output wrapped in MCP's `content` array with `isError` set based on exit code.
- Add CTest coverage for all new MCP methods.

## Verification
- Added explicit test functions in `tests/test_agent_serve.cpp`.
- 100% CTest gate passed.

## Status
Closed.
