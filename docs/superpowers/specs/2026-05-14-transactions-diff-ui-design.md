# Transactions, Diff, And Review UI Design

## Purpose

CCad needs replayable, reviewable changes before it can safely expose editing commands to LLM agents. This sprint adds the first project diff and transaction journal model, plus CLI commands for inspection and diffing. It also improves the Qt review GUI so humans can comfortably review agent progress.

## Scope

In scope:

- Project diff model for components, nets, and constraints.
- Transaction journal entry model built from a before/after project pair.
- `ccad inspect` command for review JSON.
- `ccad diff` command for before/after project JSON.
- Modern Qt review GUI styling and diagnostic presentation.

Out of scope:

- Applying transactions to mutate project files.
- Reverting transactions.
- Editing from the GUI.
- Full schematic or PCB canvas.

## Architecture

Diff and transaction logic lives in `ccad_core`. CLI and GUI only render the core output. This keeps machine and human views aligned.

Modules:

- `src/ccad_core/diff.hpp/.cpp`: computes added, removed, and changed object entries.
- `src/ccad_core/transaction.hpp/.cpp`: creates transaction journal entries from before/after projects.
- `src/ccad_core/json.hpp/.cpp`: shared JSON string escaping and small output helpers.
- `src/ccad_cli/main.cpp`: adds `inspect` and `diff`.
- `src/ccad_gui/main.cpp`: improves review layout and style.

## CLI Contracts

`ccad inspect <project>` prints:

```json
{
  "project": {"id": "...", "name": "..."},
  "counts": {"components": 0, "nets": 0, "constraints": 0},
  "status": "...",
  "diagnostics": []
}
```

`ccad diff <before> <after>` prints:

```json
{
  "summary": {"added": 1, "removed": 0, "changed": 0},
  "entries": [
    {"change": "added", "object_type": "component", "object_id": "U1", "message": "Component added"}
  ]
}
```

## UI Direction

The GUI remains native Qt Widgets, but should look like a modern review surface:

- restrained dark-accent header
- readable summary cards/chips
- clear open/reload actions
- severity-colored diagnostics
- useful empty state
- compact but not cramped spacing

The GUI is still read-only until transactions can be safely applied.

## Testing

Core tests cover diff and transaction behavior. CLI tests cover `inspect` and `diff`. GUI polish is compile-verified and manually inspected through the local Qt build.

