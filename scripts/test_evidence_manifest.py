from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

CHECKER = Path(__file__).with_name("check_evidence_manifest.py")


class EvidenceManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        subprocess.run(["git", "init", "-q"], cwd=self.root, check=True)
        (self.root / ".gitattributes").write_text("artifacts/evidence/** -text\n", encoding="utf-8")
        subprocess.run(["git", "config", "user.email", "test@example.invalid"], cwd=self.root, check=True)
        subprocess.run(["git", "config", "user.name", "Evidence Contract"], cwd=self.root, check=True)
        self.artifact = self.root / "artifacts/evidence/sprint/test.log"
        self.artifact.parent.mkdir(parents=True)
        self.artifact.write_text("real test output\n", encoding="utf-8")
        self.manifest_path = Path("artifacts/evidence/sprint.json")
        self.manifest_path.parent.mkdir(parents=True, exist_ok=True)
        manifest = {
            "status": "pass",
            "artifacts": [{"path": "artifacts/evidence/sprint/test.log",
                           "sha256": hashlib.sha256(self.artifact.read_bytes()).hexdigest()}],
        }
        raw = json.dumps(manifest, separators=(",", ":")).encode()
        (self.root / self.manifest_path).write_bytes(raw)
        self.manifest_hash = hashlib.sha256(raw).hexdigest()
        subprocess.run(["git", "add", "."], cwd=self.root, check=True)
        message = ("Why: test\n\nVerification: " + self.manifest_path.as_posix() +
                   " (sha256 " + self.manifest_hash + ")")
        subprocess.run(["git", "commit", "-qm", message], cwd=self.root, check=True)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def invoke(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run([sys.executable, str(CHECKER), "--repo", str(self.root),
                               "--commit", "HEAD", "--required"],
                              text=True, capture_output=True, check=False)

    def commit_manifest(self, manifest: dict) -> str:
        raw = json.dumps(manifest, separators=(",", ":")).encode()
        (self.root / self.manifest_path).write_bytes(raw)
        digest = hashlib.sha256(raw).hexdigest()
        subprocess.run(["git", "add", str(self.manifest_path)], cwd=self.root, check=True)
        message = ("Workspace-only evidence\n\nVerification: " + self.manifest_path.as_posix() +
                   " (sha256 " + digest + ")")
        subprocess.run(["git", "commit", "-qm", message], cwd=self.root, check=True)
        return digest

    def invoke_local_hook(self, digest: str) -> subprocess.CompletedProcess[str]:
        message = self.root / "COMMIT_EDITMSG"
        message.write_text(
            "Verification: " + self.manifest_path.as_posix() + " (sha256 " + digest + ")\n",
            encoding="utf-8")
        return subprocess.run(
            [sys.executable, str(CHECKER), "--repo", str(self.root),
             "--commit-message", str(message), "--required"],
            text=True, capture_output=True, check=False)

    def test_commit_hook_reads_the_index_blob(self) -> None:
        message = self.root / "COMMIT_EDITMSG"
        message.write_text(
            "Verification: " + self.manifest_path.as_posix() + " (sha256 " +
            self.manifest_hash + ")\n", encoding="utf-8")
        (self.root / self.artifact.relative_to(self.root)).write_text(
            "unstaged edit must not affect indexed evidence\n", encoding="utf-8")
        result = subprocess.run(
            [sys.executable, str(CHECKER), "--repo", str(self.root),
             "--commit-message", str(message), "--required"],
            text=True, capture_output=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_accepts_committed_manifest_and_artifact_hashes(self) -> None:
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_rejects_tampered_manifest(self) -> None:
        (self.root / self.manifest_path).write_text(
            '{"status":"pass","artifacts":[]}', encoding="utf-8")
        subprocess.run(["git", "add", str(self.manifest_path)], cwd=self.root, check=True)
        subprocess.run(["git", "commit", "-qm", "Tampered manifest"], cwd=self.root, check=True)
        failed = self.invoke()
        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("no Verification", failed.stderr)

    def test_workspace_only_evidence_is_verified_by_local_hook_not_staged(self) -> None:
        workspace_artifact = self.root / "artifacts/evidence/sprint/canvas.png"
        workspace_artifact.write_bytes(b"locally inspected screenshot")
        manifest = {
            "status": "pass",
            "artifacts": [],
            "workspace_only_artifacts": [{
                "path": "artifacts/evidence/sprint/canvas.png",
                "sha256": hashlib.sha256(workspace_artifact.read_bytes()).hexdigest(),
            }],
        }
        digest = self.commit_manifest(manifest)
        local_check = self.invoke_local_hook(digest)
        self.assertEqual(local_check.returncode, 0, local_check.stderr)
        self.assertIn("workspace-only", local_check.stdout)

        workspace_artifact.unlink()
        commit_check = self.invoke()
        self.assertEqual(commit_check.returncode, 0, commit_check.stderr)
        self.assertIn("not independently byte-verified", commit_check.stdout)

    def test_local_hook_rejects_changed_workspace_only_evidence(self) -> None:
        workspace_artifact = self.root / "artifacts/evidence/sprint/canvas.png"
        workspace_artifact.write_bytes(b"locally inspected screenshot")
        manifest = {
            "status": "pass",
            "artifacts": [],
            "workspace_only_artifacts": [{
                "path": "artifacts/evidence/sprint/canvas.png",
                "sha256": hashlib.sha256(workspace_artifact.read_bytes()).hexdigest(),
            }],
        }
        digest = self.commit_manifest(manifest)
        workspace_artifact.write_bytes(b"changed after visual inspection")
        local_check = self.invoke_local_hook(digest)
        self.assertNotEqual(local_check.returncode, 0)
        self.assertIn("workspace-only artifact hash mismatch", local_check.stderr)

    def test_rejects_malformed_workspace_only_artifact_metadata(self) -> None:
        digest = self.commit_manifest({
            "status": "pass",
            "artifacts": [],
            "workspace_only_artifacts": [{"path": 17, "sha256": "0" * 64}],
        })
        local_check = self.invoke_local_hook(digest)
        self.assertNotEqual(local_check.returncode, 0)
        self.assertIn("artifact path must be a non-empty string", local_check.stderr)


if __name__ == "__main__":
    unittest.main()
