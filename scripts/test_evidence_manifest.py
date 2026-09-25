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


if __name__ == "__main__":
    unittest.main()
