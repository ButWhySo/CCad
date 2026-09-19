"""No-network Cerebras/OpenAI-compatible adapter initialization contract."""

import os

from langchain_openai import ChatOpenAI


model = os.environ.get("CCAD_CEREBRAS_MODEL", "gpt-oss-120b")
adapter = ChatOpenAI(
    model=model,
    base_url="https://api.cerebras.ai/v1",
    api_key="sk-local-contract-only",
    reasoning_effort="none" if model == "qwen-3.8-27b" else "medium",
    default_headers={"X-Cerebras-3rd-Party-Integration": "langgraph"},
    timeout=5,
)
assert adapter.model_name == model
assert str(adapter.openai_api_base).rstrip("/") == "https://api.cerebras.ai/v1"
assert adapter.default_headers["X-Cerebras-3rd-Party-Integration"] == "langgraph"
assert adapter.reasoning_effort in {"none", "medium"}
print("PASS Cerebras adapter initialization; no request sent")
