"""No-network Cerebras/OpenAI-compatible adapter initialization contract."""

import os

from langchain_openai import ChatOpenAI


model = os.environ.get("CCAD_CEREBRAS_MODEL", "qwen-3-32b")
adapter = ChatOpenAI(
    model=model,
    base_url="https://api.cerebras.ai/v1",
    api_key="sk-local-contract-only",
    timeout=5,
)
assert adapter.model_name == model
assert str(adapter.openai_api_base).rstrip("/") == "https://api.cerebras.ai/v1"
print("PASS Cerebras adapter initialization; no request sent")
