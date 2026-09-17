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


def call_gui(server, method, arguments):
    if method not in READ_ONLY_METHODS:
        raise PermissionError("GUI MCP bridge permits read-only methods only")
    request = dict(arguments)
    request["method"] = method
    pipe_path = rf"\\.\pipe\{server}"
    with open(pipe_path, "r+b", buffering=0) as pipe:
        pipe.write((json.dumps(request, separators=(",", ":")) + "\n").encode())
        pipe.flush()
        response = json.loads(pipe.readline().decode("utf-8"))
    return response


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
            elif method == "ping":
                result = {}
            elif method == "tools/list":
                result = {"tools": [{"name": "ccad_gui_query",
                    "description": "Read live CCad GUI-map state; read-only allowlist",
                    "annotations": {"readOnlyHint": True, "destructiveHint": False,
                                    "openWorldHint": False},
                    "inputSchema": {"type": "object", "properties": {
                        "method": {"type": "string", "enum": sorted(READ_ONLY_METHODS)},
                        "arguments": {"type": "object"}}, "required": ["method"]}}]}
            elif method == "tools/call":
                params = request.get("params", {})
                if params.get("name") != "ccad_gui_query":
                    raise ValueError("unknown MCP tool")
                args = params.get("arguments", {})
                result = {"content": [{"type": "text", "text": json.dumps(
                    call_gui(options.server, args.get("method", ""), args.get("arguments", {}))) }],
                          "isError": False}
            else:
                raise ValueError("unsupported MCP method")
            print(json.dumps({"jsonrpc": "2.0", "id": request_id, "result": result}), flush=True)
        except PermissionError as exc:
            print(json.dumps(error(request.get("id"), -32001, str(exc))), flush=True)
        except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
            print(json.dumps(error(request.get("id"), -32602, str(exc))), flush=True)


if __name__ == "__main__":
    main()
