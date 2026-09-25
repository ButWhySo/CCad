"""Validate a commit's checked-in, hash-bound CCad evidence manifest."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

REFERENCE = re.compile(r"^Verification:\s+(\S+)\s+\(sha256\s+([0-9a-f]{64})\)\s*$", re.M)


def fail(message: str) -> int:
    print(f"evidence gate: {message}", file=sys.stderr)
    return 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path.cwd())
    parser.add_argument("--commit-message", type=Path)
    parser.add_argument("--commit", default="HEAD")
    parser.add_argument("--required", action="store_true")
    args = parser.parse_args()
    root = args.repo.resolve()
    if args.commit_message:
        message = args.commit_message.read_text(encoding="utf-8", errors="replace")
        staged_mode = True
    else:
        result = subprocess.run(
            ["git", "show", "-s", "--format=%B", args.commit], cwd=root,
            text=True, capture_output=True, check=False)
        if result.returncode:
            return fail(f"cannot read commit message: {result.stderr.strip()}")
        message = result.stdout
        staged_mode = False
    match = REFERENCE.search(message)
    if not match:
        return fail("commit has no Verification: path (sha256 digest) line") if args.required else 0
    relative = Path(match.group(1).replace("\\", "/"))
    if relative.is_absolute() or ".." in relative.parts:
        return fail("manifest path must be repository-relative and cannot traverse directories")
    manifest_path = root / relative
    try:
        if staged_mode:
            tracked = subprocess.run(["git", "ls-files", "--error-unmatch", "--", relative.as_posix()],
                                     cwd=root, text=True, capture_output=True, check=False)
            if tracked.returncode:
                return fail("manifest must be staged in the commit")
            raw = subprocess.run(["git", "show", f":{relative.as_posix()}"],
                                 cwd=root, capture_output=True, check=True).stdout
        else:
            raw = subprocess.run(["git", "show", f"{args.commit}:{relative.as_posix()}"],
                                 cwd=root, capture_output=True, check=True).stdout
        if hashlib.sha256(raw).hexdigest().lower() != match.group(2).lower():
            return fail("manifest SHA-256 does not match commit message")
        manifest = json.loads(raw)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        return fail(f"cannot load referenced manifest: {exc}")
    if not isinstance(manifest, dict):
        return fail("manifest root must be an object")
    if manifest.get("status") != "pass":
        return fail("manifest status is not pass")
    def checked_artifact_path(artifact: dict) -> tuple[Path, str] | None:
        path_value = artifact.get("path")
        if not isinstance(path_value, str) or not path_value:
            fail("artifact path must be a non-empty string")
            return None
        artifact_path = Path(path_value.replace("\\", "/"))
        if artifact_path.is_absolute() or ".." in artifact_path.parts:
            fail("artifact path escapes repository")
            return None
        digest = artifact.get("sha256", "")
        if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", digest):
            fail(f"artifact has invalid SHA-256 metadata: {artifact_path.as_posix()}")
            return None
        return artifact_path, digest

    artifacts = manifest.get("artifacts", [])
    workspace_only = manifest.get("workspace_only_artifacts", [])
    if not isinstance(artifacts, list) or not isinstance(workspace_only, list):
        return fail("manifest artifact collections must be arrays")

    for artifact in artifacts:
        if not isinstance(artifact, dict):
            return fail("artifact entry must be an object")
        checked = checked_artifact_path(artifact)
        if checked is None:
            return 1
        artifact_path, expected_digest = checked
        try:
            if staged_mode:
                tracked = subprocess.run(["git", "ls-files", "--error-unmatch", "--", artifact_path.as_posix()],
                                         cwd=root, text=True, capture_output=True, check=False)
                if tracked.returncode:
                    return fail(f"artifact must be staged in the commit: {artifact_path}")
                actual = subprocess.run(["git", "show", f":{artifact_path.as_posix()}"],
                                        cwd=root, capture_output=True, check=True).stdout
            else:
                actual = subprocess.run(["git", "show", f"{args.commit}:{artifact_path.as_posix()}"],
                                        cwd=root, capture_output=True, check=True).stdout
        except (OSError, subprocess.CalledProcessError) as exc:
            return fail(f"artifact missing from commit: {artifact_path}: {exc}")
        if hashlib.sha256(actual).hexdigest().lower() != expected_digest.lower():
            return fail(f"artifact hash mismatch: {artifact_path}")

    for artifact in workspace_only:
        if not isinstance(artifact, dict):
            return fail("workspace-only artifact entry must be an object")
        checked = checked_artifact_path(artifact)
        if checked is None:
            return 1
        artifact_path, expected_digest = checked
        if staged_mode:
            local_path = (root / artifact_path).resolve()
            try:
                local_path.relative_to(root)
            except ValueError:
                return fail("workspace-only artifact path escapes repository")
            try:
                actual = local_path.read_bytes()
            except OSError as exc:
                return fail(f"workspace-only artifact is missing from working tree: {artifact_path}: {exc}")
            if hashlib.sha256(actual).hexdigest().lower() != expected_digest.lower():
                return fail(f"workspace-only artifact hash mismatch: {artifact_path}")

    workspace_note = (
        f"; {len(workspace_only)} workspace-only artifact(s), not independently byte-verified from commit"
        if workspace_only and not staged_mode else
        f"; {len(workspace_only)} workspace-only artifact(s) checked in working tree"
        if workspace_only else ""
    )
    print(f"evidence gate: valid pass manifest {relative.as_posix()} "
          f"({len(artifacts)} committed artifact(s){workspace_note})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
