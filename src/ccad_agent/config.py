import json
import os
from pathlib import Path
from typing import Dict, Any, List

def get_config_path() -> Path:
    app_data = os.getenv("APPDATA")
    if app_data:
        config_dir = Path(app_data) / "CCad"
    else:
        config_dir = Path.home() / ".ccad"
    config_dir.mkdir(parents=True, exist_ok=True)
    return config_dir / "agent_config.json"

class AgentConfigManager:
    def __init__(self):
        self.config_path = get_config_path()
        self.config = self._load()

    def _load(self) -> Dict[str, Any]:
        if self.config_path.exists():
            try:
                with open(self.config_path, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception as e:
                print(f"Error loading config: {e}")
        return self._default_config()

    def _default_config(self) -> Dict[str, Any]:
        return {
            "theme": "Dark",
            "grid": "0.5 mm",
            "autosave": True,
            "restore_session": True,
            "provider": "openai",
            "model": "gpt-5.1",
            "sandbox_mode": True,
            "approval_policy": True,
            "project_name": "sprint-demo",
            "project_path": "",
            "trust_level": "Trusted",
            "memory": {
                "stm": True,
                "ltm": False,
                "episodic": False
            },
            "hooks": [],
            "personalisation": {
                "follow_up": "ask",
                "show_context_usage": True,
                "chat_mode": "Inline",
                "agent_personality": "Default",
                "custom_instructions": ""
            },
            "mcp_servers": [],
            "plugins": [],
            "workflows": []
        }

    def save(self):
        try:
            with open(self.config_path, "w", encoding="utf-8") as f:
                json.dump(self.config, f, indent=4)
        except Exception as e:
            print(f"Error saving config: {e}")

    def update(self, key: str, value: Any):
        if key == "mcp_servers":
            value = self._normalize_mcp_servers(value)
        self.config[key] = value
        self.save()

    @staticmethod
    def _normalize_mcp_servers(value: Any) -> List[Dict[str, Any]]:
        """Keep persisted MCP entries typed and launchable, without starting them."""
        if not isinstance(value, list):
            return []
        normalized = []
        for entry in value:
            if not isinstance(entry, dict):
                continue
            name = str(entry.get("name", "")).strip()
            command = str(entry.get("command", "")).strip()
            if not name or not command:
                continue
            args = entry.get("args", [])
            if isinstance(args, str):
                args = args.split()
            if not isinstance(args, list):
                args = []
            args = [str(arg) for arg in args]
            try:
                port = max(0, int(entry.get("port", 0)))
            except (TypeError, ValueError):
                port = 0
            normalized.append({"name": name, "command": command,
                               "args": args, "port": port,
                               "enabled": bool(entry.get("enabled", True))})
        return normalized

    def get(self, key: str, default: Any = None) -> Any:
        return self.config.get(key, default)
