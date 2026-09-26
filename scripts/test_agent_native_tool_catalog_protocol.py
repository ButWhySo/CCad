"""Verify the real native-catalog-to-LangChain registration boundary."""

import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ORCHESTRATOR = ROOT / "src" / "ccad_agent" / "orchestrator.py"
sys.path.insert(0, str(ORCHESTRATOR.parent))
CATALOG = [{
    "method": "project.context",
    "description": "Return a compact project and board summary.",
    "read_only": True,
    "inputSchema": {"type": "object", "properties": {}, "required": []},
}, {
    "method": "ui.route_track",
    "description": "Route one track between two board points.",
    "read_only": False,
    "inputSchema": {
        "type": "object",
        "properties": {
            "start_x_mm": {"type": "number", "description": "Start X in mm."},
            "end_x_mm": {"type": "number", "description": "End X in mm."},
            "dry_run": {"type": "boolean", "description": "Preview only.", "default": False},
        },
        "required": ["start_x_mm", "end_x_mm"],
    },
}, {
    "method": "cli.pcb.add-track",
    "description": "CLI command descriptor only; the GUI broker has no CLI executor yet.",
    "read_only": False,
    "callable": False,
    "inputSchema": {
        "type": "object",
        "properties": {"argv": {"type": "array", "items": {"type": "string"}}},
        "required": ["argv"],
    },
}]


spec = importlib.util.spec_from_file_location("ccad_agent_catalog", ORCHESTRATOR)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)
tools = module.build_native_tools(module.validate_native_tool_catalog(CATALOG))
assert [tool.name for tool in tools] == ["ccad_project_context", "ccad_ui_route_track"]
assert tools[1].args_schema.model_json_schema()["required"] == ["start_x_mm", "end_x_mm"]
assert len(module.validate_native_tool_catalog(CATALOG)) == 2
try:
    module.validate_native_tool_catalog([CATALOG[-1]])
except ValueError as error:
    assert "callable" in str(error)
else:
    raise AssertionError("a catalog containing only non-callable descriptors must not install")

env = os.environ.copy()
env["CCAD_AGENT_DEFER_PROVIDER_INIT"] = "1"
env["PYTHONNOUSERSITE"] = "1"
run = subprocess.run(
    [sys.executable, str(ORCHESTRATOR)],
    input=(json.dumps({"method": "agent.set_tool_catalog", "params": {"catalog": CATALOG}}) + "\n"
           + json.dumps({"method": "agent.set_tool_catalog", "params": {"catalog": []}}) + "\n"),
    text=True, capture_output=True, env=env, timeout=20, check=False,
)
assert run.returncode == 0, run.stderr
responses = [json.loads(line) for line in run.stdout.splitlines() if line.startswith("{")]
states = [item["params"] for item in responses if item.get("method") == "tool_catalog_state"]
assert states[0] == {"accepted": True, "method_count": 2, "tool_count": 6,
                     "native_tool_count": 2, "local_tool_count": 4,
                     "secret_value_visible": False}
assert states[1]["accepted"] is False and states[1]["tool_count"] == 0
print("PASS native catalog builds provider-safe LangChain tools and IPC rejects invalid input; no provider call")
