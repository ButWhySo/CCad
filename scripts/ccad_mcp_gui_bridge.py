"""Read-only MCP stdio bridge to a running CCad GUI-map named pipe."""
import argparse
import json
import sys

READ_ONLY_METHODS = frozenset({
    "agent.methods", "agent.harness_context", "agent.workspace_state",
    "agent.quickstart", "agent.tool_guide", "ui.map", "ui.map_compact",
    "ui.role_summary", "ui.index_stats", "ui.get_node", "ui.nodes_by_role",
    "ui.find", "ui.hit_test", "ui.current_tool", "ui.epoch", "ui.active_layer",
    "ui.active_net", "ui.get_selection", "project.context", "project.object_counts",
    "project.review", "project.erc", "project.drc", "project.diagnostics",
})


def error(request_id, code, message):
    return {"jsonrpc": "2.0", "id": request_id,
            "error": {"code": code, "message": message}}


def call_gui(server, method, arguments, *, allow_approval_ui=False):
    if method not in READ_ONLY_METHODS and not (allow_approval_ui and method in {"ui.type_text", "ui.click"}):
        raise PermissionError("GUI MCP bridge permits read-only methods only")
    request = dict(arguments)
    request["method"] = method
    pipe_path = rf"\\.\pipe\{server}"
    with open(pipe_path, "r+b", buffering=0) as pipe:
        pipe.write((json.dumps(request, separators=(",", ":")) + "\n").encode())
        pipe.flush()
        response = json.loads(pipe.readline().decode("utf-8"))
    return response


def request_native_approval(server, request_text):
    """Stage approval in native panel; never execute or accept mutation."""
    staged = call_gui(server, "ui.type_text", {
        "id": "control:agent_approval_request", "text": request_text}, allow_approval_ui=True)
    opened = call_gui(server, "ui.click", {"id": "action:agent_request_approval"}, allow_approval_ui=True)
    staged_ok = staged.get("result", {}).get("performed", False)
    opened_ok = opened.get("result", {}).get("performed", False)
    return {"approval_required": bool(staged_ok and opened_ok), "staged": staged,
            "opened": opened,
            "human_action": ("Use Agent panel Accept, Decline, or Cancel."
                              if staged_ok and opened_ok else
                              "Native approval target unavailable; inspect GUI-map.")}


def approval_status(server):
    """Return only the native approval lane state needed by an external host."""
    snapshot = call_gui(server, "ui.map_compact", {"limit": 100})
    nodes = snapshot.get("result", {}).get("nodes", [])
    selected = {node.get("id"): node for node in nodes
                if node.get("id") in {"panel:agent_approval_preview",
                                       "label:agent_approval_status"}}
    card = selected.get("panel:agent_approval_preview", {})
    label = selected.get("label:agent_approval_status", {})
    return {"visible": card.get("visible", False),
            "status": label.get("text", ""), "nodes": selected}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", default="ccad_live_cmd")
    options = parser.parse_args()
    for line in sys.stdin:
        request = {}
        try:
            request = json.loads(line)
            request_id = request.get("id")
            method = request.get("method", "")
            if method == "initialize":
                result = {"protocolVersion": "2024-11-05", "capabilities": {"tools": {}},
                          "serverInfo": {"name": "ccad-gui-bridge", "version": "1.0.0"}}
            elif method.startswith("notifications/"):
                # MCP notifications have no response. Keep every lifecycle
                # notification off stdout to preserve stdio framing.
                continue
            elif method == "ping":
                result = {}
            elif method == "tools/list":
                result = {"tools": [
                    {"name": "ccad_gui_query",
                     "description": "Read live CCad GUI-map state; read-only allowlist",
                     "annotations": {"readOnlyHint": True, "destructiveHint": False,
                                      "openWorldHint": False},
                     "inputSchema": {"type": "object", "properties": {
                         "method": {"type": "string", "enum": sorted(READ_ONLY_METHODS)},
                         "arguments": {"type": "object"}}, "required": ["method"]}},
                    {"name": "ccad_gui_request_approval",
                     "description": "Stage native approval card; never execute mutation",
                     "annotations": {"readOnlyHint": False, "destructiveHint": False,
                                      "openWorldHint": False},
                     "inputSchema": {"type": "object", "properties": {
                         "request": {"type": "string"}}, "required": ["request"]}},
                    {"name": "ccad_gui_approval_status",
                     "description": "Read native approval-card visibility and status",
                     "annotations": {"readOnlyHint": True, "destructiveHint": False,
                                      "openWorldHint": False},
                     "inputSchema": {"type": "object", "properties": {}}}
                ]}
            elif method == "tools/call" and request.get("params", {}).get("name") == "ccad_gui_approval_status":
                result = {"content": [{"type": "text", "text": json.dumps(
                    approval_status(options.server))}], "isError": False}
            elif method == "tools/call" and request.get("params", {}).get("name") == "ccad_gui_query":
                params = request.get("params", {})
                if params.get("name") != "ccad_gui_query":
                    raise ValueError("unknown MCP tool")
                args = params.get("arguments", {})
                result = {"content": [{"type": "text", "text": json.dumps(
                    call_gui(options.server, args.get("method", ""), args.get("arguments", {}))) }],
                          "isError": False}
            elif method == "tools/call" and request.get("params", {}).get("name") == "ccad_gui_request_approval":
                request_text = request.get("params", {}).get("arguments", {}).get("request", "").strip()
                if not request_text:
                    raise ValueError("request must be non-empty")
                result = {"content": [{"type": "text", "text": json.dumps(
                    request_native_approval(options.server, request_text))}], "isError": False}
            else:
                raise LookupError("unsupported MCP method")
            print(json.dumps({"jsonrpc": "2.0", "id": request_id, "result": result}), flush=True)
        except PermissionError as exc:
            print(json.dumps(error(request.get("id"), -32001, str(exc))), flush=True)
        except LookupError as exc:
            print(json.dumps(error(request.get("id"), -32601, str(exc))), flush=True)
        except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
            print(json.dumps(error(request.get("id"), -32602, str(exc))), flush=True)


if __name__ == "__main__":
    main()
