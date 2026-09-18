"""No-network CI contract: every checkout is clean and submodule-free."""

from pathlib import Path

workflow = (Path(__file__).resolve().parents[1] / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")
assert workflow.count("uses: actions/checkout@v5") == 4
assert workflow.count("submodules: false") == 4
assert workflow.count("clean: true") == 4
print("PASS CI checkout contract: clean, explicit no-submodule checkout in all jobs")
