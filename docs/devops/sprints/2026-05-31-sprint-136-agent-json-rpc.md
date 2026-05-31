# Sprint 136: Agent JSON-RPC Protocol

**Goal:** Establish the foundation for Phase 6 (Agent protocol) by implementing a basic JSON-RPC interface over standard input/output.

**Branch:** `sprint-136-agent-json-rpc`

**Tasks:**
1. Add `src/ccad_cli/agent_commands.hpp/.cpp` to handle `ccad agent serve`.
2. Implement a JSON-RPC 2.0 loop reading from `std::cin` and writing to `std::cout`.
3. Support a basic `ping` method.
4. Support an `execute` method that wraps existing CLI commands (e.g., `["pcb", "get-object", "--file", "..."]`).
5. Add CLI tests for the `agent serve` loop.

**Why:**
To allow future agentic IDEs, MCP clients, or other processes to interact with CCad durably over a structured JSON-RPC stream, without paying process startup costs for every command.

**References Checked:**
- JSON-RPC 2.0 Specification (https://www.jsonrpc.org/specification)
- Existing CLI command dispatch in `app.cpp`

**Verification:**
- `ccad agent serve` reads JSON-RPC requests, dispatches, and responds.
- CTest passes.
