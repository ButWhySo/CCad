"""No-network behavior proof for recursive persisted-config secret removal."""

import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "ccad_agent"))
import orchestrator as ccad  # noqa: E402


rejected = []
clean = ccad.sanitize_persisted_config(
    {"provider": {"api_key": "must-drop", "model": "gpt-oss"},
     "items": [{"token": "must-drop", "keep": True}]},
    ("api_key", "apikey", "secret", "token", "password", "credential"),
    rejected,
)
assert clean == {"provider": {"model": "gpt-oss"}, "items": [{"keep": True}]}, clean
assert rejected == ["provider.api_key", "items.token"], rejected
print("PASS recursive config secret sanitizer; no network")
