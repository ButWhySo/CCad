import os
import sys
import json
import time
import struct
import pyautogui

def read_ui_map():
    pipe_name = r'\\.\pipe\ccad_agent_pipe'
    try:
        with open(pipe_name, 'r+b') as pipe:
            req = {"jsonrpc": "2.0", "method": "ui.map", "id": 1}
            req_str = json.dumps(req) + "\n"
            pipe.write(req_str.encode('utf-8'))
            pipe.flush()
            
            # Read response
            resp_line = pipe.readline()
            if not resp_line:
                return None
            resp = json.loads(resp_line.decode('utf-8') if isinstance(resp_line, bytes) else resp_line)
            return resp.get('result')
    except Exception as e:
        print(f"Error reading UI map: {e}")
        return None

def main():
    print("LIVE AGENT DRIVER STARTED")
    print("Send commands in the format: 'CLICK <x> <y>' or 'TYPE <text>' or 'WAIT' or 'EXIT'")
    sys.stdout.flush()
    
    while True:
        # 1. Fetch and print UI Map
        ui_map = read_ui_map()
        if ui_map:
            # We will just print the nodes so the agent can parse it
            print("--- UI MAP ---")
            nodes = ui_map.get('nodes', [])
            for node in nodes:
                nid = node.get('id', '')
                tx = node.get('target_x', -1)
                ty = node.get('target_y', -1)
                rect = node.get('global_rect', {})
                print(f"Node: {nid} | Target: ({tx}, {ty}) | Rect: {rect}")
            print("--- END UI MAP ---")
        else:
            print("Failed to fetch UI Map.")
            
        print("WAITING FOR COMMAND...")
        sys.stdout.flush()
        
        # 2. Wait for stdin command
        cmd_line = sys.stdin.readline().strip()
        if not cmd_line:
            continue
            
        print(f"Executing: {cmd_line}")
        
        parts = cmd_line.split(" ", 2)
        cmd = parts[0].upper()
        
        if cmd == "CLICK":
            if len(parts) >= 3:
                x = int(parts[1])
                y = int(parts[2])
                pyautogui.moveTo(x, y, duration=0.0)
                pyautogui.click()
                print(f"Clicked at ({x}, {y})")
        elif cmd == "TYPE":
            if len(parts) >= 2:
                text = cmd_line[5:] # Everything after "TYPE "
                pyautogui.write(text, interval=0.0)
                print(f"Typed: {text}")
        elif cmd == "WAIT":
            time.sleep(0.1)
            print("Waited 0.1 second.")
        elif cmd == "EXIT":
            print("Exiting driver.")
            break
        else:
            print(f"Unknown command: {cmd}")
            
        sys.stdout.flush()

if __name__ == "__main__":
    main()
