"""No-network contract: benign Gemini import warnings never claim request state."""

from pathlib import Path


source = (
    Path(__file__).resolve().parents[1]
    / "src"
    / "ccad_gui"
    / "agent_panel.cpp"
).read_text(encoding="utf-8")

warning_branch = source[source.index("void AgentPanel::handlePythonError()"):source.index("void AgentPanel::submitChat()")]
assert 'error.contains("langchain_google_genai")' in warning_branch
assert 'error.contains("google.generativeai package has ended")' in warning_branch
assert "Provider was not contacted" not in warning_branch
assert "Gemini adapter dependency notice" not in warning_branch
assert "addActivityEvent" not in warning_branch[warning_branch.index('if ((error.contains("FutureWarning")'):warning_branch.index("// Surface actionable startup/provider diagnostics")]
print("PASS benign Gemini import warnings do not fabricate provider execution state; no network")
