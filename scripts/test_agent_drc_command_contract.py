"""No-network contract: /drc dispatches the authoritative read-only method."""

from pathlib import Path


source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(
    encoding="utf-8"
)
start = source.index('elif cmd_base == "/drc":')
end = source.index('elif cmd_base == "/place":', start)
drc_branch = source[start:end]
assert '"tool": "project.drc"' in drc_branch
assert '"tool": "action.drc"' not in drc_branch
assert 'dispatch_client_tool("project.drc", {})' in drc_branch
assert '"DRC complete' in drc_branch
assert '"DRC did not complete' in drc_branch
print("PASS /drc awaits and reports the authoritative read-only project.drc result; no network")
