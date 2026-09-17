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
    parser.add_argument("--chat-input-check", action="store_true",
                        help="type into live mapped agent chat editor without sending")
    parser.add_argument("--chat-send-check", action="store_true",
                        help="send mapped agent chat text through offline mock provider")
    parser.add_argument("--provider", default="",
                        help="provider override inherited by GUI backend, e.g. mock")
    parser.add_argument("--mock-tool-approval-check", action="store_true",
                        help="request one offline non-dry-run via and approve it")
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
    if args.provider:
        env["CCAD_PROVIDER"] = args.provider
    if args.mock_tool_approval_check:
        env["CCAD_MOCK_MUTATION"] = "1"
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
    if args.mock_tool_approval_check:
        with open(rf"\\.\pipe\{server}", "r+b", buffering=0) as pipe:
            send(pipe, {"method": "ui.type_text", "id": "control:agent_chat_input",
                        "text": "Routing Expert: place one via using approved route."})
            send(pipe, {"method": "ui.click", "id": "action:agent_submit_chat"})
            time.sleep(2.0)
            pending = send(pipe, {"method": "ui.map_compact", "id": "mutation-pending",
                                  "limit": 100})
            approved = send(pipe, {"method": "ui.click", "id": "action:agent_approve_next"})
            time.sleep(1.0)
            after = send(pipe, {"method": "ui.map_compact", "id": "mutation-after",
                                "limit": 100})
            counts = send(pipe, {"method": "project.object_counts", "id": "mutation-counts"})
        pending_result = pending.get("result", {})
        approved_result = approved.get("result", {})
        after_result = after.get("result", {})
        counts_result = counts.get("result", {})
        pending_nodes = pending_result.get("nodes", []) if isinstance(pending_result, dict) else []
        after_nodes = after_result.get("nodes", []) if isinstance(after_result, dict) else []
        pending_card = next((node for node in pending_nodes
                             if node.get("id") == "panel:agent_approval_preview"), {})
        after_status = next((node.get("label", "") for node in after_nodes
                             if node.get("id") == "label:agent_approval_status"), "")
        via_count = counts_result.get("via_count", counts_result.get("vias", 0)) \
            if isinstance(counts_result, dict) else 0
        print("LIVE APPROVED MUTATION " + json.dumps({
            "pending_visible": pending_card.get("visible", False),
            "approve_performed": approved_result.get("performed", False),
            "status": after_status,
            "via_count": via_count,
        }, separators=(",", ":")), flush=True)
        if (not pending_card.get("visible") or not approved_result.get("performed")
                or "accepted" not in after_status.lower() or via_count < 1):
            raise RuntimeError("approved mock mutation did not complete through native approval")
    if args.chat_input_check or args.chat_send_check:
        with open(rf"\\.\pipe\{server}", "r+b", buffering=0) as pipe:
            typed = send(pipe, {"method": "ui.type_text",
                                "id": "control:agent_chat_input",
                                "text": "Summarize this board before changing it."})
            fresh_map = send(pipe, {"method": "ui.map_compact", "id": "chat-input-state",
                                    "limit": 100})
            if args.chat_send_check:
                sent = send(pipe, {"method": "ui.click", "id": "action:agent_submit_chat"})
                time.sleep(2.0)
                response_map = send(pipe, {"method": "ui.map_compact", "id": "chat-response-state",
                                           "limit": 100})
        typed_result = typed.get("result", {})
        print("LIVE CHAT INPUT " + json.dumps({
            "performed": typed_result.get("performed"),
            "reason": typed_result.get("reason"),
            "focused": typed_result.get("focused"),
        }, separators=(",", ":")), flush=True)
        if not typed_result.get("performed") or typed_result.get("reason") != "text_set":
            raise RuntimeError("live mapped agent chat input did not accept text")
        if not fresh_map.get("result"):
            raise RuntimeError("live GUI map unavailable after chat input")
        if args.chat_send_check:
            sent_result = sent.get("result", {})
            print("LIVE CHAT SEND " + json.dumps({
                "performed": sent_result.get("performed"),
                "reason": sent_result.get("reason"),
            }, separators=(",", ":")), flush=True)
            if not sent_result.get("performed"):
                raise RuntimeError("live mapped agent Send action did not perform")
            response_result = response_map.get("result", {})
            response_nodes = response_result.get("nodes", []) if isinstance(response_result, dict) else []
            response_labels = [node.get("label", "") for node in response_nodes
                               if node.get("id") == "label:agent_result"]
            print("LIVE CHAT RESPONSE " + json.dumps({
                "received": any("Chat response received" in label for label in response_labels),
                "labels": response_labels,
            }, separators=(",", ":")), flush=True)
            if not any("Chat response received" in label for label in response_labels):
                raise RuntimeError("mock provider response was not reflected in GUI state")
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
        # The bridge may stage only; simulate the explicitly human GUI decision
        # through the same mapped action, then read authoritative approval state.
        with open(rf"\\.\pipe\{server}", "r+b", buffering=0) as pipe:
            pending_state = send(pipe, {"method": "ui.map_compact", "id": "approval-pending-state",
                                        "limit": 100})
            decision = send(pipe, {"method": "ui.click",
                                   "id": "action:agent_decline_next"})
            state = send(pipe, {"method": "ui.map_compact", "id": "approval-state",
                                "limit": 100})
        decision_result = decision.get("result", {})
        state_result = state.get("result", {})
        pending_result = pending_state.get("result", {})
        if "nodes" not in state_result and isinstance(state_result.get("result"), dict):
            state_result = state_result["result"]
        if "nodes" not in pending_result and isinstance(pending_result.get("result"), dict):
            pending_result = pending_result["result"]
        pending_cards = [node for node in pending_result.get("nodes", [])
                         if node.get("id") == "panel:agent_approval_preview"]
        pending_visible = bool(pending_cards and pending_cards[0].get("visible"))
        final_cards = [node for node in state_result.get("nodes", [])
                       if node.get("id") == "panel:agent_approval_preview"]
        final_visible = bool(final_cards and final_cards[0].get("visible"))
        status_labels = [node.get("label", "") for node in state_result.get("nodes", [])
                         if node.get("id") == "label:agent_approval_status"]
        status_label = status_labels[0] if status_labels else ""
        print(f"LIVE APPROVAL CARD pending_visible={pending_visible} final_visible={final_visible}", flush=True)
        print(f"LIVE HUMAN DECISION declined performed={decision_result.get('performed')}", flush=True)
        print("LIVE APPROVAL STATE " + json.dumps({"status": status_label}, separators=(",", ":")), flush=True)
        if (not pending_visible or final_visible or not decision_result.get("performed")
                or "declined" not in status_label.lower()):
            raise RuntimeError("live human approval decision was not reflected in authoritative state")
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
