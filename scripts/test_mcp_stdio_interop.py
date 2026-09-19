"""Provider-free smoke test for the native CCad MCP stdio boundary."""

from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "build-qt" / "ccad.exe"


def main() -> int:
    if not EXE.exists():
        raise SystemExit(f"missing built MCP executable: {EXE}")
    requests = [
        {"jsonrpc": "2.0", "method": "initialize", "params": {"protocolVersion": "2025-06-18"}, "id": 1},
        {"jsonrpc": "2.0", "method": "notifications/initialized"},
        {"jsonrpc": "2.0", "method": "tools/list", "id": 2},
        {"jsonrpc": "2.0", "method": "resources/list", "id": 3},
        {"jsonrpc": "2.0", "method": "resources/read", "params": {"uri": "ccad://harness-context"}, "id": 4},
    ]
    payload = "\n".join(json.dumps(item) for item in requests) + "\n"
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join(
        [r"C:\Qt\6.11.1\mingw_64\bin", r"C:\Qt\Tools\mingw1310_64\bin", env.get("PATH", "")]
    )
    completed = subprocess.run(
        [str(EXE), "agent", "serve", "--allow-read"],
        cwd=ROOT,
        input=payload,
        text=True,
        capture_output=True,
        env=env,
        timeout=10,
        check=False,
    )
    if completed.returncode != 0:
        raise SystemExit(f"MCP subprocess failed: {completed.returncode}\n{completed.stderr}")
    lines = [json.loads(line) for line in completed.stdout.splitlines() if line.strip()]
    if len(lines) != 4:
        raise SystemExit(f"expected four responses (notification silent), got {len(lines)}: {completed.stdout!r}")
    if lines[0]["result"]["protocolVersion"] != "2025-06-18":
        raise SystemExit("initialize did not negotiate 2025-06-18")
    tool_names = {tool["name"] for tool in lines[1]["result"]["tools"]}
    required = {"ccad_execute", "ccad_harness_context", "ccad_workspace_state", "ccad_agent_methods"}
    if not required.issubset(tool_names):
        raise SystemExit(f"missing MCP tools: {sorted(required - tool_names)}")
    resource_uris = {resource["uri"] for resource in lines[2]["result"]["resources"]}
    if "ccad://harness-context" not in resource_uris or "ccad://workspace-state" not in resource_uris:
        raise SystemExit("required CCad resources were not advertised")
    if lines[3]["result"]["contents"][0]["uri"] != "ccad://harness-context":
        raise SystemExit("resource read returned the wrong URI")
    print("PASS native MCP stdio external-client interoperability; no network")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
