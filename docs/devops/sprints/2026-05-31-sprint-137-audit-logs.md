# Sprint 137: Agent Audit Logs

## Goal
Implement append-only audit logging for all CLI mutations, capturing the command, the before-and-after project states, and the diff.

## Branch
`sprint-137-audit-logs`

## Tasks
- Add `setAuditCommand` to capture `argc`/`argv` command string in `ccad_cli::run`.
- Update `writeProjectFile` to compare the saved `before` project with the `after` project.
- Append a deterministic JSONL line with the `Transaction` to `<project_path>.audit.jsonl` upon mutation.
- Avoid updating 25 individual CLI command implementations by centralizing the hook.

## References Checked
- File structure: Append-only `.jsonl` files are standard for streaming audit logs.
- CCad Architecture: `Transaction` object was already implemented but only used in tests/GUI. Now reused for file-backed logging.

## Verification
- Verified by running tests. All 26/26 tests passed.
- Output validation on `demo.ccad.json` producing `.audit.jsonl` with correct `txn-xxx`, `command`, `summary`, and `diff`.

## Status
Closed.
