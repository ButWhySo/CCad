# Sprint 12 Design: Machine-Readable CLI Help

## Goal

Expose the native CLI command surface in deterministic JSON so LLM agents can discover available commands without parsing human usage text.

## Command

```powershell
ccad help --format json
```

## Output Contract

The command emits a stable JSON object:

```json
{
  "commands": [
    {
      "name": "init",
      "summary": "Create a CCad project file",
      "usage": "ccad init --name <name> --out <path> [--width-mm <n> --height-mm <n>]"
    }
  ]
}
```

## Rules

- Keep output deterministic.
- Do not add runtime filesystem scanning.
- Do not derive help from shell commands.
- Keep human usage text and JSON help in one dispatcher module for now.

## Definition Of Done

- Black-box CLI test proves `ccad help --format json` exits zero.
- JSON output includes `commands`.
- JSON output includes `pcb place-footprint` and its `--rotation-deg` option.
- Full native Qt build and CTest pass.
