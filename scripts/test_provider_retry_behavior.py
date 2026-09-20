"""No-network contract for quota-preserving provider retry defaults."""

import ast
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
tree = ast.parse(source)
node = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "invoke_provider_with_retry")
node_source = ast.get_source_segment(source, node) or ""

assert 'os.environ.get("CCAD_PROVIDER_RETRIES", "0")' in node_source
assert '"payment_required"' in node_source
assert '"rate_limited"' in node_source
assert "if attempt >= retries or quota_or_rate_limited(error):" in node_source
print("PASS provider retries default to zero; billing and rate errors do not retry; no network")
