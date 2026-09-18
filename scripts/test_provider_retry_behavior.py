"""Exercise retry policy with fake clients only; never imports provider SDKs."""

import ast
import os
from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
tree = ast.parse(source)
node = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "invoke_provider_with_retry")
module = ast.Module(body=[node], type_ignores=[])
namespace = {"os": os, "emit": lambda event: events.append(event)}
events = []
exec(compile(module, "orchestrator.py", "exec"), namespace)
retry = namespace["invoke_provider_with_retry"]

class TemporaryFailure(Exception):
    pass

class QuotaFailure(Exception):
    status_code = 429

class FakeClient:
    def __init__(self, failures):
        self.failures = list(failures)
        self.calls = 0

    def invoke(self, messages, config=None):
        self.calls += 1
        if self.failures:
            raise self.failures.pop(0)
        return "ok"

os.environ["CCAD_PROVIDER_RETRIES"] = "2"
transient = FakeClient([TemporaryFailure(), TemporaryFailure()])
assert retry(transient, ["prompt"]) == "ok"
assert transient.calls == 3
assert len(events) == 2

quota = FakeClient([QuotaFailure(), TemporaryFailure()])
try:
    retry(quota, ["prompt"])
except QuotaFailure:
    pass
else:
    raise AssertionError("quota failure must propagate")
assert quota.calls == 1
print("PASS fake-client retry policy: transient bounded, quota immediate; no network")
