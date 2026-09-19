"""Prove session provider keys do not cross-contaminate adapters; no network."""

import os
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
import orchestrator as ccad  # noqa: E402

ccad.set_session_provider_env("OPENAI_API_KEY", "old-openai")
ccad.set_session_provider_env("CEREBRAS_API_KEY", "old-cerebras")
ccad.clear_session_provider_env()
assert "OPENAI_API_KEY" not in os.environ
assert "CEREBRAS_API_KEY" not in os.environ

ccad.set_session_provider_env("CEREBRAS_API_KEY", "session-only")
assert os.environ["CEREBRAS_API_KEY"] == "session-only"
assert "OPENAI_API_KEY" not in os.environ
ccad.clear_session_provider_env()
assert "CEREBRAS_API_KEY" not in os.environ
print("PASS provider key isolation; no network")
