# Sprint 138: Agent Permission Gates

## Goal
Implement a permission gateway for the JSON-RPC agent loop. By default, the agent should not execute arbitrary commands unless explicitly granted `read` or `write` scope. 

## Branch
`sprint-138-permission-gates`

## Tasks
- Add `--allow-read` and `--allow-write` parameters to `ccad agent serve`.
- Classify `ccad` CLI commands into read/write groups.
- Enforce permissions when parsing the incoming JSON-RPC `execute` method.
- Update `test_agent_serve.cpp` to provide `--allow-read` for testing `help`.
- Build and verify CTest success.

## Verification
- Verified by running tests. All tests passed.

## Status
Closed.
