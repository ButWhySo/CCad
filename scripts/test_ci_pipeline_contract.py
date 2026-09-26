"""No-network contracts for portable, diagnosable build and test jobs."""

import re
from pathlib import Path


workflow = (Path(__file__).resolve().parents[1] / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")


def job_block(name: str) -> str:
    match = re.search(rf"(?ms)^  {re.escape(name)}:\n(.*?)(?=^  [a-z][a-z0-9-]*:\n|\Z)", workflow)
    assert match, f"missing CI job: {name}"
    return match.group(1)


for job in ("core-linux", "gui-linux", "core-windows"):
    block = job_block(job)
    assert "actions/setup-python@v6" in block, f"{job} must use the supported Python runtime"
    assert "src/ccad_agent/requirements.txt" in block, f"{job} CTest needs agent runtime dependencies"
    assert "pip install -r" in block, f"{job} must install dependencies before configure/CTest"
    assert "upload-artifact@v4" in block, f"{job} must retain failure diagnostics"
    assert "build.log" in block, f"{job} must retain the full build output"
    assert "ctest" in block, f"{job} must run CTest"
    assert "--rerun-failed --output-on-failure" in block, f"{job} must expose failed CTest details"
    assert "ctest-failed-details.log" in block, f"{job} must retain failed-test diagnostics"

assert workflow.count("Linux build diagnostic") == 1
assert workflow.count("Linux GUI build diagnostic") == 1

python_job = job_block("agent-python")
assert "agent-python-tests.log" in python_job
assert "upload-artifact@v4" in python_job
for test_name in ("test_agent_context_contract.py", "test_project_index.py"):
    test_source = (Path(__file__).resolve().parent / test_name).read_text(encoding="utf-8")
    assert "artifacts/demos/sprint160-placement-crash-ci-final.ccad.json" not in test_source
    assert '"artifacts" / "demos"' not in test_source
print("PASS CI contract: native test dependencies and per-job diagnostics are configured; CD is not configured")
