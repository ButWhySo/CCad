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
    if manifest.get("status") != "pass":
        return fail("manifest status is not pass")
    for artifact in manifest.get("artifacts", []):
        artifact_path = Path(artifact.get("path", "").replace("\\", "/"))
        if artifact_path.is_absolute() or ".." in artifact_path.parts:
            return fail("artifact path escapes repository")
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
        if hashlib.sha256(actual).hexdigest().lower() != artifact.get("sha256", "").lower():
            return fail(f"artifact hash mismatch: {artifact_path}")
    print(f"evidence gate: valid pass manifest {relative.as_posix()} ({len(manifest.get('artifacts', []))} artifacts)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
