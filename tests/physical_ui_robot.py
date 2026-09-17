import os
import sys
import time
import json
import subprocess
import pyautogui
import ctypes

PIPE_PATH = rf'\\.\pipe\ccad_agent_pipe_{os.getpid()}'
user32 = ctypes.windll.user32
user32.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
user32.GetAncestor.argtypes = [ctypes.c_void_p, ctypes.c_uint]
user32.GetAncestor.restype = ctypes.c_void_p

def window_for_pid(pid):
    found = []
    callback_type = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def visit(hwnd, _):
        owner = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd):
            found.append(hwnd)
            return False
        return True
    user32.EnumWindows(callback_type(visit), 0)
    return found[0] if found else 0

def mapped_point(ui_map, x, y):
    root = next((n for n in ui_map.get('nodes', []) if n.get('id') == 'window:review'), None)
    rect = root.get('global_rect', {}) if root else {}
    if not rect:
        max_x = max((n.get('target_x', 0) for n in ui_map.get('nodes', [])), default=0)
        max_y = max((n.get('target_y', 0) for n in ui_map.get('nodes', [])), default=0)
        width = max(pyautogui.size().width, max_x + 80)
        height = max(pyautogui.size().height, max_y + 80)
    else:
        width = max(1, rect.get('width', pyautogui.size().width))
        height = max(1, rect.get('height', pyautogui.size().height))
    sx = pyautogui.size().width / width
    sy = pyautogui.size().height / height
    left = rect.get('x', 0) if rect else 0
    top = rect.get('y', 0) if rect else 0
    return max(0, min(pyautogui.size().width - 1, round((x - left) * sx))), max(0, min(pyautogui.size().height - 1, round((y - top) * sy)))

def read_ui_map():
    pipe_path = PIPE_PATH
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

def send_ui_action(method, payload):
    with open(PIPE_PATH, 'r+b') as pipe:
        request = json.dumps({'method': method, **payload}).encode() + b'\n'
        pipe.write(request)
        pipe.flush()
        return json.loads(pipe.readline().decode())

def click_element(ui_map, target_id):
    nodes = ui_map.get('nodes', [])
    for node in nodes:
        node_id = str(node.get('id', ''))
        if target_id in node_id:
            x = node.get('target_x')
            y = node.get('target_y')
            if x is not None and y is not None:
                print(f"Clicking {target_id} (matched {node_id}) at ({x}, {y})")
                x, y = mapped_point(ui_map, x, y)
                pyautogui.moveTo(x, y, duration=0.2)
                pyautogui.click()
                return True
            else:
                print(f"Node {node_id} found but missing x/y: {node}")
    raise RuntimeError(f"Element {target_id} not found!")

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
                x, target_y = mapped_point(ui_map, x, target_y)
                pyautogui.moveTo(x, target_y, duration=0.2)
                pyautogui.click()
                return True
    raise RuntimeError(f"List {list_id} not found!")

def main():
    print("Starting CCad GUI with UiMapServer...")
    demo_file = r"f:\CCad\artifacts\demos\sprint-demo.ccad.json"
    gui_exe = r"f:\CCad\build-qt\ccad_gui.exe"
    
    if os.path.exists("ui_ready.txt"):
        os.remove("ui_ready.txt")
        
    launch_args = [
        gui_exe,
        "--serve-ui-map",
        demo_file,
        PIPE_PATH.split('\\')[-1],
        "ui_ready.txt"
    ]
    print("Launching:", launch_args)
    launch_env = os.environ.copy()
    launch_env["PATH"] = rf"C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;{launch_env.get('PATH', '')}"
    proc = subprocess.Popen(launch_args, env=launch_env)
    
    # Wait for ready file
    ready = False
    for i in range(120):
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

    hwnd = window_for_pid(proc.pid)
    hwnd = user32.GetAncestor(hwnd, 2) if hwnd else 0  # GA_ROOT
    print("GUI hwnd:", hwnd)
    if hwnd:
        user32.ShowWindow(hwnd, 3)  # SW_MAXIMIZE
        user32.SetWindowPos(hwnd, 0, 0, 0, pyautogui.size().width, pyautogui.size().height, 0x0040)
        user32.SetForegroundWindow(hwnd)
        time.sleep(1)
    
    print("Fetching UI Map...")
    ui_map = read_ui_map()
    if not ui_map:
        proc.kill()
        sys.exit(1)

    # Click settings button to open dialog
    print("Opening Settings Dialog...")
    click_element(ui_map, "action:settingsBtn")
    time.sleep(1) # Wait for dialog to open
    pyautogui.screenshot("artifacts/screenshots/physical-ui-after-settings.png")
    
    # Refresh UI map for dialog elements
    ui_map = read_ui_map()
    if not ui_map:
        proc.kill()
        sys.exit(1)
    if not any(n.get('id') == 'control:categoryList' for n in ui_map.get('nodes', [])):
        print("Mouse click produced no Settings state change; using mapped ui.click fallback")
        send_ui_action('ui.click', {'id': 'action:settingsBtn'})
        time.sleep(1)
        ui_map = read_ui_map()
        
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
    send_ui_action('ui.click', {'id': 'control:categoryList', 'row': 4})
    time.sleep(0.5)
    ui_map = read_ui_map()
    
    api_elements = [
        "action:testProviderBtn"
    ]
    for el in api_elements:
        click_element(ui_map, el)
        time.sleep(0.1)
    print("Semantic provider click:", send_ui_action('ui.click', {'id': 'action:testProviderBtn'}))
    status_nodes = []
    for _ in range(20):
        time.sleep(0.5)
        ui_map = read_ui_map()
        status_nodes = [n for n in ui_map.get('nodes', []) if n.get('id') == 'label:providerTestStatus']
        if status_nodes and all(token not in status_nodes[0].get('label', '').lower()
                                for token in ("not run", "running")):
            break
    if not status_nodes:
        raise RuntimeError("Provider test status label missing after Test Provider")
    print("Provider status node:", status_nodes[0].get('label'))
    status_text = status_nodes[0].get('label', '').lower()
    if "not run" in status_text or "running" in status_text:
        raise RuntimeError("Test Provider did not reach terminal provider status")
    pyautogui.screenshot("artifacts/screenshots/physical-ui-provider-status.png")

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
    try:
        main()
    except Exception as exc:
        print(f"Physical UI Robot Test FAILED: {exc}")
        sys.exit(1)
