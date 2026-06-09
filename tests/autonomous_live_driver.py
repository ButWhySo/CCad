import sys
import json
import time
import pyautogui

def read_ui_map():
    pipe_name = r'\\.\pipe\ccad_agent_pipe'
    try:
        with open(pipe_name, 'r+b') as pipe:
            req = {"jsonrpc": "2.0", "method": "ui.map", "id": 1}
            req_str = json.dumps(req) + "\n"
            pipe.write(req_str.encode('utf-8'))
            pipe.flush()
            
            resp_line = pipe.readline()
            if not resp_line:
                return None
            resp = json.loads(resp_line.decode('utf-8') if isinstance(resp_line, bytes) else resp_line)
            return resp.get('result')
    except Exception as e:
        print(f"Error reading UI map: {e}")
        return None

def find_element(ui_map, name):
    if not ui_map or 'nodes' not in ui_map:
        return None
    for node in ui_map['nodes']:
        if node.get('id') == name:
            rect = node.get('global_rect', {})
            x = rect.get('x', -1)
            y = rect.get('y', -1)
            w = rect.get('width', 0)
            h = rect.get('height', 0)
            if x != -1 and y != -1:
                return (x + w // 2, y + h // 2)
    return None

def click_node(name):
    print(f"Searching for {name}...")
    for _ in range(50): # Wait up to 5 seconds for element to appear
        ui_map = read_ui_map()
        coords = find_element(ui_map, name)
        if coords:
            print(f"Found {name} at {coords}. Clicking.")
            pyautogui.moveTo(coords[0], coords[1], duration=0.0)
            pyautogui.click()
            time.sleep(0.1)
            return True
        time.sleep(0.1)
    print(f"Failed to find {name}")
    return False

def main():
    print("AUTONOMOUS LIVE DRIVER STARTED")
    
    # 1. Open Agent Tab
    if not click_node("tab:agent"): return
    
    # 2. Click Settings button
    if not click_node("action:settingsBtn"): return
    
    # 3. Wait for Configuration tab explicitly (or just wait for input)
    # The dialog takes a moment to appear
    print("Typing into project name...")
    if not click_node("control:projectNameInput"): return
    
    # Select all and type
    pyautogui.hotkey('ctrl', 'a')
    pyautogui.write("Autonomous Hyper-Speed Mode", interval=0.0)
    time.sleep(0.1)
    
    # 4. Save
    if not click_node("action:primaryButton"): return
    
    print("AUTONOMOUS DRIVER COMPLETE")

if __name__ == "__main__":
    main()
