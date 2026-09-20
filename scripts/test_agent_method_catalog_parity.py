"""No-network proof: runtime agent controls stay discoverable."""

import re
from pathlib import Path


source = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
runtime = set(re.findall(r'(?:if|elif) method == "(agent\.[^"]+)"', source))
# The two explicit provider probes deliberately share one transient-secret
# dispatch branch. Treat its tuple members as runtime methods too; otherwise
# this contract would falsely claim the catalog is ahead of executable code.
for group in re.findall(r'method in \(([^)]*)\)', source, flags=re.DOTALL):
    runtime.update(re.findall(r'"(agent\.[^"]+)"', group))
catalog = set(re.findall(r'"name": "(agent\.[^"]+)"', source))
intentional_legacy = {"agent.test_export"}
assert runtime - catalog == intentional_legacy, sorted(runtime - catalog)
assert catalog - runtime == set(), sorted(catalog - runtime)
print("PASS runtime agent methods remain discoverable; legacy telemetry explicitly excluded; no network")
