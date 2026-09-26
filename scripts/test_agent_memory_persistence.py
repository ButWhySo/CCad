"""Persistent preference and real memory JSON-RPC contracts; no external network."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import sqlite3

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from config import AgentConfigManager


with tempfile.TemporaryDirectory() as temp:
    previous_app_data = os.environ.get("APPDATA")
    os.environ["APPDATA"] = temp
    config = AgentConfigManager()
    config.config_path = Path(temp) / "settings" / "agent_config.json"
    config.update_checked("memory", {"stm": True, "ltm": True, "episodic": False})
    restored = AgentConfigManager()
    restored.config_path = config.config_path
    restored.config = restored._load()
    assert restored.get("memory") == config.get("memory")

    original = config.config_path.read_text(encoding="utf-8")
    blocked_parent = Path(temp) / "not-a-directory"
    blocked_parent.write_text("preserve this file", encoding="utf-8")
    config.config_path = blocked_parent / "child" / "agent_config.json"
    previous = dict(config.get("memory"))
    try:
        config.update_checked("memory", {"stm": False, "ltm": False})
    except RuntimeError as error:
        assert getattr(error, "category", "") == "configuration_persistence_failed"
    else:
        raise AssertionError("memory preference reported saved with an unwritable path")
    assert config.get("memory") == previous
    assert blocked_parent.read_text(encoding="utf-8") == "preserve this file"
    assert original
    if previous_app_data is None:
        os.environ.pop("APPDATA", None)
    else:
        os.environ["APPDATA"] = previous_app_data


with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    app_data = root / "roaming"
    memory_path = root / "memory.json"
    project_path = root / "untouched-project.ccad.json"
    checkpoint_path = root / "checkpoints.sqlite"
    project_bytes = b'{"design_revision":"keep-exactly"}\n'
    project_path.write_bytes(project_bytes)
    with sqlite3.connect(checkpoint_path) as database:
        database.execute("CREATE TABLE protected_checkpoint (value TEXT NOT NULL)")
        database.execute("INSERT INTO protected_checkpoint VALUES (?)", ("retain",))
    database.close()
    env = os.environ.copy()
    env["APPDATA"] = str(app_data)
    env["CCAD_AGENT_MEMORY_PATH"] = str(memory_path)
    env["CCAD_AGENT_CHECKPOINT_DB"] = str(checkpoint_path)
    env["CCAD_AGENT_DEFER_PROVIDER_INIT"] = "1"
    requests = [
        {"method": "agent.memory_set_enabled", "params": {
            "tier": "ltm", "enabled": True}},
        {"method": "agent.memory_add", "params": {
            "tier": "ltm", "scope": "conversation", "title": "test",
            "kind": "preference",
            "importance": 5,
            "content": "Preserve the user's verified 0.25 mm clearance rule."}},
        {"method": "agent.memory_set_enabled", "params": {
            "tier": "ltm", "enabled": False}},
        {"method": "agent.memory_reset", "params": {"confirmed": False}},
        {"method": "agent.memory_state", "params": {"tier": "ltm"}},
        {"method": "agent.memory_reset", "params": {
            "tier": "ltm", "confirmed": True}},
        {"method": "agent.memory_set_enabled", "params": {
            "tier": "episodic", "enabled": True}},
        {"method": "agent.memory_set_enabled", "params": {
            "tier": "episodic", "enabled": False}},
    ]
    process = subprocess.run(
        [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
        input="".join(json.dumps(request) + "\n" for request in requests),
        capture_output=True,
        text=True,
        env=env,
        timeout=45,
        check=False,
    )
    assert process.returncode == 0, process.stderr[-2000:]
    events = []
    for line in process.stdout.splitlines():
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(event, dict):
            events.append(event)

    def matching(method):
        return [event["params"] for event in events
                if event.get("method") == method]

    assert matching("memory_added")[0]["kind"] == "preference"
    assert matching("memory_added")[0]["importance"] == 5

    toggles = matching("memory_state")
    assert any(state.get("tier") == "ltm" and state.get("enabled") is True
               and state.get("persisted") is True for state in toggles)
    assert any(state.get("tier") == "ltm" and state.get("enabled") is False
               and state.get("runtime_entries") == 0
               and state.get("persistent_entries") == 1
               and state.get("persisted") is True for state in toggles)
    assert any(state.get("tier") == "episodic" and state.get("enabled") is True
               and state.get("persistent_entries") == 0
               and state.get("persisted") is True for state in toggles)
    resets = matching("memory_reset")
    assert any(state.get("removed") == 0 and "error" in state for state in resets)
    assert any(state.get("removed") == 1 and "error" not in state for state in resets)
    config_path = app_data / "CCad" / "agent_config.json"
    persisted = json.loads(config_path.read_text(encoding="utf-8"))
    assert persisted["memory"]["ltm"] is False
    assert persisted["memory"]["episodic"] is False
    assert memory_path.is_file()
    assert project_path.read_bytes() == project_bytes
    with sqlite3.connect(checkpoint_path) as database:
        assert database.execute("SELECT value FROM protected_checkpoint").fetchone() == ("retain",)
    database.close()

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    app_data = root / "roaming"
    memory_path = root / "memory.json"
    original_bytes = b'{damaged memory; preserve this file'
    memory_path.write_bytes(original_bytes)
    env = os.environ.copy()
    env["APPDATA"] = str(app_data)
    env["CCAD_AGENT_MEMORY_PATH"] = str(memory_path)
    env.pop("CCAD_AGENT_CHECKPOINT_DB", None)
    request = {"method": "agent.memory_set_enabled", "params": {
        "tier": "ltm", "enabled": True}}
    process = subprocess.run(
        [sys.executable, str(ROOT / "src" / "ccad_agent" / "orchestrator.py")],
        input=json.dumps(request) + "\n", capture_output=True, text=True,
        env=env, timeout=45, check=False)
    assert process.returncode == 0, process.stderr[-2000:]
    replies = [json.loads(line) for line in process.stdout.splitlines()
               if line.startswith("{")]
    error_state = next(event["params"] for event in replies
                       if event.get("method") == "memory_state")
    assert error_state["enabled"] is False
    assert error_state["persisted"] is True
    assert error_state["error"] == "memory_store_corrupt"
    assert memory_path.read_bytes() == original_bytes
    saved = json.loads((app_data / "CCad" / "agent_config.json").read_text())
    assert saved["memory"]["ltm"] is False

print("PASS durable namespace activation, checked preferences, truthful memory IPC, reset approval, and project isolation")
