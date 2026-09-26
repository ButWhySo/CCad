"""No-network CI contract: every checkout is clean and submodule-free."""

from pathlib import Path

workflow = (Path(__file__).resolve().parents[1] / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")
assert workflow.count("uses: actions/checkout@v5") == 5
assert workflow.count("submodules: false") == 4
assert workflow.count("clean: true") == 5
evidence_job = workflow.split("  evidence-manifest:\n", 1)[1]
assert "fetch-depth: 0" in evidence_job
assert "ref: ${{ github.event.pull_request.head.sha || github.sha }}" in evidence_job
print("PASS CI checkout contract: clean app jobs and full-history evidence checkout")
