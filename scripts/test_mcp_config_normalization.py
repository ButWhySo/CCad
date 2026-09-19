"""No-network persistence contract for MCP settings entries."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "ccad_agent"))
from config import AgentConfigManager


entries = AgentConfigManager._normalize_mcp_servers([
    {"name": " valid ", "command": " ccad-mcp ", "args": "--stdio --safe",
     "port": "-4", "enabled": False},
    {"name": "missing-command"},
    "not-an-object",
    {"name": "bad-port", "command": "tool", "port": "nope"},
])
assert entries == [
    {"name": "valid", "command": "ccad-mcp", "args": ["--stdio", "--safe"],
     "port": 0, "enabled": False},
    {"name": "bad-port", "command": "tool", "args": [], "port": 0,
     "enabled": True},
]
assert AgentConfigManager._normalize_mcp_servers({}) == []
print("PASS MCP settings normalization; no network or server launch")
