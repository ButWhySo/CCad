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
        self.config[key] = value
        self.save()

    def get(self, key: str, default: Any = None) -> Any:
        return self.config.get(key, default)
