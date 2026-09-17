"""Drive CCad's persistent GUI through its live UI-map command socket."""

import argparse
import json
import os
import subprocess
import time


def send(pipe, request):
    pipe.write((json.dumps(request, separators=(",", ":")) + "\n").encode("utf-8"))
    pipe.flush()
    return json.loads(pipe.readline().decode("utf-8"))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--delay", type=float, default=0.25)
    parser.add_argument("--project", default=r"artifacts\demos\live-agent-blank.ccad.json")
    parser.add_argument("--hold-seconds", type=float, default=10.0)
    parser.add_argument("--reuse-project", action="store_true")
    parser.add_argument("--mcp-bridge-check", action="store_true",
                        help="query live GUI through MCP bridge and stage approval card")
    args = parser.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    gui = os.path.join(root, "build-qt", "ccad_gui.exe")
    ccad = os.path.join(root, "build-qt", "ccad.exe")
    project = os.path.abspath(os.path.join(root, args.project))
    ready = os.path.join(root, "artifacts", "live-agent-route.ready.txt")
    server = f"ccad_live_agent_route_demo_{os.getpid()}"
    if os.path.exists(ready):
        os.remove(ready)

    env = os.environ.copy()
    env["PATH"] = r"C:\Qt\6.11.1\mingw_64\bin;" + env.get("PATH", "")
    if not args.reuse_project:
        os.makedirs(os.path.dirname(project), exist_ok=True)
        subprocess.run(
            [ccad, "init", "--name", "live-agent-blank", "--width-mm", "90",
             "--height-mm", "52", "--out", project],
            check=True,
            env=env,
        )
        subprocess.run([ccad, "pcb", "add-standard-layers", "--file", project],
                       check=True, env=env)
    process = subprocess.Popen([gui, "--serve-ui-map", project, server, ready], env=env)
    for _ in range(100):
        if os.path.exists(ready):
            break
        if process.poll() is not None:
            raise RuntimeError(f"CCad GUI exited early with code {process.returncode}")
        time.sleep(0.1)
    else:
        process.kill()
        raise RuntimeError("CCad GUI socket did not become ready")

    time.sleep(5.0)
    if args.mcp_bridge_check:
        bridge = os.path.join(root, "scripts", "ccad_mcp_gui_bridge.py")
        payload = (json.dumps({"jsonrpc": "2.0", "method": "tools/call",
                               "params": {"name": "ccad_gui_query", "arguments":
                                           {"method": "ui.map_compact", "arguments": {}}},
                               "id": 1}) + "\n" +
                   json.dumps({"jsonrpc": "2.0", "method": "tools/call",
                               "params": {"name": "ccad_gui_request_approval", "arguments":
                                           {"request": "Approve live MCP route test"}},
                               "id": 2}) + "\n")
        result = subprocess.run(["python", bridge, "--server", server], input=payload,
                                text=True, capture_output=True, env=env, check=True)
        print("MCP BRIDGE " + result.stdout.replace("\n", " "), flush=True)
    requests = [{"method": "ui.trigger_safe", "id": "action:fit"}]
    for row in range(6):
        y = 7 + row * 7
        for column in range(7):
            x1 = 5 + column * 11
            x2 = x1 + 8
            y2 = y + (3 if (row + column) % 2 == 0 else -3)
            requests.append({"method": "ui.route_track", "start_x_mm": x1,
                             "start_y_mm": y, "end_x_mm": x2, "end_y_mm": y2})
    for x, y in ((10, 10), (24, 17), (38, 24), (52, 31), (66, 38), (80, 45)):
        requests.append({"method": "ui.place_via", "x_mm": x, "y_mm": y})
    for y in (4, 48):
        requests.append({"method": "ui.draw_graphic", "start_x_mm": 4,
                         "start_y_mm": y, "end_x_mm": 86, "end_y_mm": y})

    pipe_path = rf"\\.\pipe\{server}"
    with open(pipe_path, "r+b", buffering=0) as pipe:
        track_number = 0
        total_tracks = sum(request["method"] == "ui.route_track" for request in requests)
        for request in requests:
            response = send(pipe, request)
            if request["method"] == "ui.route_track":
                track_number += 1
                result = response.get("result", {})
                print(
                    f"LIVE GUI TRACK {track_number}/{total_tracks} performed={result.get('performed')}",
                    flush=True,
                )
                time.sleep(args.delay)
            else:
                print(f"LIVE GUI {request['method']}", flush=True)
                time.sleep(max(0.1, args.delay))

    print(f"Runtime burst complete. Holding GUI for {args.hold_seconds:g} seconds.", flush=True)
    time.sleep(max(0.0, args.hold_seconds))
    process.terminate()
    try:
        process.wait(timeout=3.0)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=3.0)


if __name__ == "__main__":
    main()
