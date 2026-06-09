import os
import sys
import time
import json
import subprocess
import pyautogui

def read_ui_map():
    pipe_path = r'\\.\pipe\ccad_agent_pipe'
    try:
        with open(pipe_path, 'r+b') as pipe:
            pipe.write(b'{"method":"ui.map"}\n')
            pipe.flush()
            response = pipe.readline().decode('utf-8')
            full_resp = json.loads(response)
            return full_resp.get('result', {})
    except Exception as e:
        print(f"Error reading pipe: {e}")
        return None

def click_element(ui_map, target_id):
    nodes = ui_map.get('nodes', [])
    for node in nodes:
        node_id = str(node.get('id', ''))
        if target_id in node_id:
            x = node.get('target_x')
            y = node.get('target_y')
            if x is not None and y is not None:
                print(f"Clicking {target_id} (matched {node_id}) at ({x}, {y})")
                pyautogui.moveTo(x, y, duration=0.2)
                pyautogui.click()
                return True
            else:
                print(f"Node {node_id} found but missing x/y: {node}")
    print(f"Element {target_id} not found!")
    return False

def type_text(ui_map, target_id, text):
    if click_element(ui_map, target_id):
        time.sleep(0.1)
        # First clear it maybe? Or just type
        pyautogui.write(text, interval=0.02)
        return True
    return False

def click_list_item(ui_map, list_id, item_index):
    nodes = ui_map.get('nodes', [])
    for node in nodes:
        node_id = str(node.get('id', ''))
        if list_id in node_id:
            x = node.get('target_x')
            global_rect = node.get('global_rect', {})
            list_y = global_rect.get('y')
            if x is not None and list_y is not None:
                # Approximate 35 pixels per item height in Qt
                target_y = list_y + 17 + (item_index * 35)
                print(f"Clicking list {list_id} item {item_index} at ({x}, {target_y})")
                pyautogui.moveTo(x, target_y, duration=0.2)
                pyautogui.click()
                return True
    print(f"List {list_id} not found!")
    return False

def main():
    print("Starting CCad GUI with UiMapServer...")
    demo_file = r"f:\CCad\artifacts\demos\sprint-demo.ccad.json"
    gui_exe = r"f:\CCad\build-qt\ccad_gui.exe"
    
    if os.path.exists("ui_ready.txt"):
        os.remove("ui_ready.txt")
        
    proc = subprocess.Popen([
        gui_exe,
        "--serve-ui-map",
        demo_file,
        "ccad_agent_pipe",
        "ui_ready.txt"
    ])
    
    # Wait for ready file
    ready = False
    for i in range(50):
        if os.path.exists("ui_ready.txt"):
            ready = True
            break
        time.sleep(0.2)
        
    if not ready:
        print("UI Map Server failed to start.")
        proc.kill()
        sys.exit(1)
        
    print("Waiting 2 seconds for Qt layouts to settle...")
    time.sleep(2) 
    
    print("Fetching UI Map...")
    ui_map = read_ui_map()
    if not ui_map:
        proc.kill()
        sys.exit(1)
        
    # Click settings button to open dialog
    print("Opening Settings Dialog...")
    click_element(ui_map, "action:settingsBtn")
    time.sleep(1) # Wait for dialog to open
    
    # Refresh UI map for dialog elements
    ui_map = read_ui_map()
    if not ui_map:
        proc.kill()
        sys.exit(1)
        
    # Go to Configuration Tab (Index 1)
    print("Switching to Configuration tab...")
    click_list_item(ui_map, "control:categoryList", 1)
    time.sleep(0.5)
    ui_map = read_ui_map()
    
    config_elements = [
        "control:providerCombo", "control:modelInput", "control:sandboxCb", 
        "control:approvalCb", "control:projectNameInput", "control:projectPathInput", 
        "control:trustLevelCombo", "control:stmCb", "control:ltmCb", 
        "control:episodicCb", "control:hooksCombo"
    ]
    for el in config_elements:
        click_element(ui_map, el)
        time.sleep(0.1)
        
    # Go to Personalisation Tab (Index 2)
    print("Switching to Personalisation tab...")
    click_list_item(ui_map, "control:categoryList", 2)
    time.sleep(0.5)
    ui_map = read_ui_map()
    
    person_elements = [
        "control:followUpInput", "control:contextWindowCb", "control:chatModeCombo",
        "control:agentPersonalityCombo", "control:customInstructionsText"
    ]
    for el in person_elements:
        click_element(ui_map, el)
        time.sleep(0.1)
        
    type_text(ui_map, "control:followUpInput", "Physical robot exhaustively testing all inputs!")
    time.sleep(0.5)
    
    # Go to MCP Tab (Index 3)
    print("Switching to MCP tab...")
    click_list_item(ui_map, "control:categoryList", 3)
    time.sleep(0.5)
    ui_map = read_ui_map()

    # Go to API & Providers Tab (Index 4)
    print("Switching to API & Providers tab...")
    click_list_item(ui_map, "control:categoryList", 4)
    time.sleep(0.5)
    ui_map = read_ui_map()
    
    api_elements = [
        "action:testExportBtn"
    ]
    for el in api_elements:
        click_element(ui_map, el)
        time.sleep(0.1)

    # Go to Plugins Tab (Index 5)
    print("Switching to Plugins tab...")
    click_list_item(ui_map, "control:categoryList", 5)
    time.sleep(0.5)
    ui_map = read_ui_map()

    # Go to Workflows Tab (Index 6)
    print("Switching to Workflows tab...")
    click_list_item(ui_map, "control:categoryList", 6)
    time.sleep(0.5)
    ui_map = read_ui_map()
    
    # Click save
    click_element(ui_map, "action:primaryButton")
    time.sleep(1)
    
    # Close app gracefully
    proc.kill()
    print("Physical UI Robot Test Complete!")

if __name__ == "__main__":
    main()
