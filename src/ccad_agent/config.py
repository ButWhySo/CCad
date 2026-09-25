import json
import os
import tempfile
from pathlib import Path
from typing import Dict, Any, List


class ConfigPersistenceError(RuntimeError):
    """Safe configuration write failure without leaking user paths or values."""

    category = "configuration_persistence_failed"

    def __init__(self):
        super().__init__(self.category)

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
                    config = json.load(f)
                if not isinstance(config, dict):
                    raise ValueError("configuration root must be an object")
                # Older unattended GUI tests appended their own literal input
                # to the persisted project fields.  Repair only those exact
                # known test values; never rewrite a real project name/path.
                repaired = self._repair_known_test_contamination(config)
                if repaired:
                    with open(self.config_path, "w", encoding="utf-8") as f:
                        json.dump(config, f, indent=4)
                return config
            except Exception as e:
                print(f"Error loading config: {e}")
        return self._default_config()

    @staticmethod
    def _repair_known_test_contamination(config: Dict[str, Any]) -> bool:
        repaired = False
        project_name = config.get("project_name")
        if isinstance(project_name, str) and project_name and set(project_name.split("test_proj")) == {""}:
            config["project_name"] = "sprint-demo"
            repaired = True
        project_path = config.get("project_path")
        if isinstance(project_path, str) and project_path and set(project_path.split("/tmp/test")) == {""}:
            config["project_path"] = ""
            repaired = True
        return repaired

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
                "episodic": False,
                "semantic": {
                    "enabled": False,
                    "backend": "ollama_local",
                    "base_url": "http://127.0.0.1:11434",
                    "model": "embeddinggemma"
                }
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
            "workflows": [],
            "observability": {
                "enabled": False,
                "backend": "langfuse",
                "base_url": "",
                "environment": "development",
                "service_name": "ccad-agent"
            }
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

    def update_checked(self, key: str, value: Any):
        """Persist a critical preference atomically before changing live config."""
        if key == "mcp_servers":
            value = self._normalize_mcp_servers(value)
        candidate = dict(self.config)
        candidate[key] = value
        self._write_checked(candidate)
        self.config = candidate
        return True

    def _write_checked(self, candidate):
        temporary_path = None
        try:
            self.config_path.parent.mkdir(parents=True, exist_ok=True)
            descriptor, temporary_name = tempfile.mkstemp(
                prefix=".ccad-agent-config-", dir=self.config_path.parent)
            temporary_path = Path(temporary_name)
            with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
                json.dump(candidate, handle, indent=4)
                handle.write("\n")
                handle.flush()
                os.fsync(handle.fileno())
            written = json.loads(temporary_path.read_text(encoding="utf-8"))
            if written != candidate:
                raise ConfigPersistenceError()
            os.replace(temporary_path, self.config_path)
            temporary_path = None
            readback = json.loads(self.config_path.read_text(encoding="utf-8"))
            if readback != candidate:
                raise ConfigPersistenceError()
        except ConfigPersistenceError:
            raise
        except (OSError, TypeError, ValueError) as error:
            raise ConfigPersistenceError() from error
        finally:
            if temporary_path is not None:
                try:
                    temporary_path.unlink(missing_ok=True)
                except OSError:
                    pass

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
