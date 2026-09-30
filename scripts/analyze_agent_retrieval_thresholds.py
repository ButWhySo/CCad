"""Calibrate retrieval score thresholds on calibration data and audit held-out cost."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
from typing import Any, cast


EVALUATION_VERSION = "1.0.0"
PROJECT_TASKS = {
    "semantic_paraphrase", "component_function", "design_intent", "project_entity",
}


class RetrievalThresholdScopeError(ValueError):
    """Raised when reports cannot be compared within one immutable scope."""


def _model_digest(report: dict[str, Any]) -> str:
    runtime = report.get("runtime")
    identity = runtime.get("semantic_model_identity") if isinstance(runtime, dict) else None
    digest = identity.get("digest") if isinstance(identity, dict) else None
    if not isinstance(digest, str) or not digest.strip():
        raise RetrievalThresholdScopeError("semantic_model_digest_missing")
    return digest


def _validate_reports(reports: tuple[dict[str, Any], ...], surface: str) -> dict[str, Any]:
    if len(reports) != 4:
        raise RetrievalThresholdScopeError("benchmark_report_count_invalid")
    expected_splits = ("calibration", "held_out", "calibration", "held_out")
    scope: tuple[str, str, str, str, str] | None = None
    for index, (report, expected_split) in enumerate(zip(reports, expected_splits)):
        if not isinstance(report, dict) or report.get("schema_version") != 1:
            raise RetrievalThresholdScopeError("benchmark_report_schema_invalid")
        if report.get("split") != expected_split:
            raise RetrievalThresholdScopeError("benchmark_split_mismatch")
        digest = _model_digest(report)
        identity = (digest, report.get("dataset_id"), report.get("dataset_version"),
                    report.get("dataset_sha256"), report.get("benchmark_version"))
        if not all(isinstance(value, str) and value for value in identity):
            raise RetrievalThresholdScopeError("benchmark_scope_incomplete")
        identity = cast(tuple[str, str, str, str, str], identity)
        if scope is None:
            scope = identity
        elif identity != scope:
            raise RetrievalThresholdScopeError("scope_mismatch")
        mode = (report.get("project_retrieval_mode") if surface == "project" else
                report.get("memory_retrieval_mode"))
        expected_mode = "semantic" if index in {0, 1} else "lexical"
        if mode != expected_mode:
            raise RetrievalThresholdScopeError("retrieval_mode_mismatch")
        cases = report.get("cases")
        if not isinstance(cases, list) or any(not isinstance(row, dict) for row in cases):
            raise RetrievalThresholdScopeError("benchmark_cases_invalid")
    if scope is None:
        raise RetrievalThresholdScopeError("benchmark_report_count_invalid")
    return {
        "model_digest": scope[0], "dataset_id": scope[1], "dataset_version": scope[2],
        "dataset_sha256": scope[3], "benchmark_version": scope[4],
    }


def _case_map(report: dict[str, Any], task: str, surface: str) -> dict[str, dict[str, Any]]:
    result = {}
    for row in report["cases"]:
        if not row.get("semantic_search_allowed"):
            continue
        if row.get("threshold_task", row.get("task")) != task:
            continue
        if surface == "memory" and row.get("task") != "memory_retrieval":
            continue
        if surface == "project" and row.get("task") == "memory_retrieval":
            continue
        case_id = row.get("case_id")
        if not isinstance(case_id, str) or not case_id or case_id in result:
            raise RetrievalThresholdScopeError("case_identity_invalid_or_duplicate")
        result[case_id] = row
    return result


def _ids(value: Any) -> set[str]:
    if not isinstance(value, list) or any(not isinstance(item, str) for item in value):
        raise RetrievalThresholdScopeError("case_result_ids_invalid")
    return set(value)


def _semantic_scores(row: dict[str, Any]) -> dict[str, float]:
    candidates = row.get("semantic_candidates", [])
    if not isinstance(candidates, list):
        raise RetrievalThresholdScopeError("semantic_candidates_invalid")
    output: dict[str, float] = {}
    for candidate in candidates:
        if not isinstance(candidate, dict):
            raise RetrievalThresholdScopeError("semantic_candidate_invalid")
        entity_id, score = candidate.get("id"), candidate.get("score")
        if (not isinstance(entity_id, str) or not entity_id or
                isinstance(score, bool) or not isinstance(score, (int, float)) or
                not math.isfinite(float(score)) or not -1.0 <= float(score) <= 1.0):
            raise RetrievalThresholdScopeError("semantic_candidate_invalid")
        output[entity_id] = max(output.get(entity_id, -math.inf), float(score))
    return output


def _metrics(semantic_rows: dict[str, dict[str, Any]],
             lexical_rows: dict[str, dict[str, Any]], threshold: float | None) -> dict[str, Any]:
    if semantic_rows.keys() != lexical_rows.keys():
        raise RetrievalThresholdScopeError("case_set_mismatch")
    positives = negatives = 0
    relevant_total = baseline_relevant = selected_relevant = 0
    baseline_fp = selected_fp = baseline_fp_candidates = selected_fp_candidates = 0
    for case_id, semantic_row in semantic_rows.items():
        lexical_row = lexical_rows[case_id]
        expected = _ids(semantic_row.get("expected_ids"))
        if expected != _ids(lexical_row.get("expected_ids")):
            raise RetrievalThresholdScopeError("case_labels_mismatch")
        lexical_ids = _ids(lexical_row.get("result_ids"))
        scores = _semantic_scores(semantic_row)
        semantic_ids = ({item for item, score in scores.items()
                         if threshold is not None and score >= threshold})
        combined = lexical_ids | semantic_ids
        if expected:
            positives += 1
            relevant_total += len(expected)
            baseline_relevant += len(expected & lexical_ids)
            selected_relevant += len(expected & combined)
        else:
            negatives += 1
            baseline_false = lexical_ids
            selected_false = combined
            baseline_fp += bool(baseline_false)
            selected_fp += bool(selected_false)
            baseline_fp_candidates += len(baseline_false)
            selected_fp_candidates += len(selected_false)
    if not positives or not negatives or not relevant_total:
        return {"status": "insufficient_examples", "positive_cases": positives,
                "hard_negative_cases": negatives}
    baseline_recall = baseline_relevant / relevant_total
    selected_recall = selected_relevant / relevant_total
    return {
        "status": "measured", "positive_cases": positives,
        "hard_negative_cases": negatives, "relevant_items": relevant_total,
        "lexical_recall": round(baseline_recall, 6),
        "recall": round(selected_recall, 6),
        "incremental_recall": round(selected_recall - baseline_recall, 6),
        "lexical_hard_negative_false_positive_cases": baseline_fp,
        "hard_negative_false_positive_cases": selected_fp,
        "lexical_hard_negative_false_positive_rate": round(baseline_fp / negatives, 6),
        "hard_negative_false_positive_rate": round(selected_fp / negatives, 6),
        "incremental_hard_negative_false_positive_rate": round(
            (selected_fp - baseline_fp) / negatives, 6),
        "lexical_false_positive_candidates": baseline_fp_candidates,
        "false_positive_candidates": selected_fp_candidates,
    }


def analyze_scoped_thresholds(semantic_calibration: dict[str, Any],
                              semantic_held_out: dict[str, Any],
                              lexical_calibration: dict[str, Any],
                              lexical_held_out: dict[str, Any], *,
                              retrieval_surface: str = "project") -> dict[str, Any]:
    """Choose score cutoffs only on calibration rows and report held-out effects."""
    if retrieval_surface not in {"project", "memory"}:
        raise RetrievalThresholdScopeError("retrieval_surface_invalid")
    reports = (semantic_calibration, semantic_held_out,
               lexical_calibration, lexical_held_out)
    scope = _validate_reports(reports, retrieval_surface)
    if retrieval_surface == "project":
        tasks = sorted({row.get("threshold_task", row.get("task"))
                        for report in reports[:2] for row in report["cases"]
                        if row.get("semantic_search_allowed") and
                        row.get("task") != "memory_retrieval" and
                        row.get("threshold_task", row.get("task")) in PROJECT_TASKS})
    else:
        tasks = ["memory_retrieval"]
    profiles = []
    for task in tasks:
        sem_cal = _case_map(semantic_calibration, task, retrieval_surface)
        sem_test = _case_map(semantic_held_out, task, retrieval_surface)
        lex_cal = _case_map(lexical_calibration, task, retrieval_surface)
        lex_test = _case_map(lexical_held_out, task, retrieval_surface)
        calibration = _metrics(sem_cal, lex_cal, None)
        held_baseline = _metrics(sem_test, lex_test, None)
        scores = sorted({score for row in sem_cal.values()
                         for score in _semantic_scores(row).values()})
        held_scores = [score for row in sem_test.values()
                       for score in _semantic_scores(row).values()]
        calibration_all_candidates = _metrics(sem_cal, lex_cal,
                                              min(scores) if scores else None)
        held_all_candidates = _metrics(sem_test, lex_test,
                                       min(held_scores) if held_scores else None)
        profile = {
            **scope, "retrieval_surface": retrieval_surface, "task": task,
            "evaluation_version": EVALUATION_VERSION,
        }
        profile["profile_id"] = hashlib.sha256(json.dumps(
            profile, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()
        if calibration["status"] != "measured" or held_baseline["status"] != "measured":
            profiles.append({**profile, "status": "insufficient_examples",
                             "selected_threshold": None, "calibration": calibration,
                             "held_out": held_baseline})
            continue
        candidate_thresholds = (sorted(set(scores + [math.nextafter(max(scores), math.inf)]))
                                if scores else [])
        candidates = []
        for threshold in candidate_thresholds:
            metrics = _metrics(sem_cal, lex_cal, threshold)
            if metrics["hard_negative_false_positive_cases"] <= \
                    metrics["lexical_hard_negative_false_positive_cases"]:
                candidates.append((metrics["recall"], threshold, metrics))
        if not candidates:
            selected_threshold, chosen = None, calibration
        else:
            _, selected_threshold, chosen = max(candidates, key=lambda item: (item[0], item[1]))
        if chosen["incremental_recall"] <= 0:
            selected_threshold = None
            chosen = calibration
            held = held_baseline
            held_status = "no_calibration_incremental_gain"
            profiles.append({
                **profile, "status": held_status, "selected_threshold": None,
                "threshold_kind": "post_retriever_candidate_score_cutoff",
                "candidate_generation": "inherited_from_current_retriever",
                "threshold_search_limited_to_observed_candidates": True,
                "calibration": chosen, "held_out": {**held, "status": held_status},
                "held_out_lexical_baseline": held_baseline,
                "calibration_semantic_all_candidates": calibration_all_candidates,
                "held_out_semantic_all_candidates": held_all_candidates,
            })
            continue
        held = _metrics(sem_test, lex_test, selected_threshold)
        if selected_threshold is None:
            held_status = "no_safe_threshold"
        elif held["incremental_hard_negative_false_positive_rate"] > 0:
            held_status = "false_positive_cost_increased"
        elif held["incremental_recall"] > 0:
            held_status = "held_out_gain_without_false_positive_increase"
        else:
            held_status = "no_held_out_incremental_gain"
        profiles.append({
            **profile, "status": held_status,
            "selected_threshold": selected_threshold,
            "threshold_kind": "post_retriever_candidate_score_cutoff",
            "candidate_generation": "inherited_from_current_retriever",
            "threshold_search_limited_to_observed_candidates": True,
            "calibration": chosen, "held_out": {**held, "status": held_status},
            "held_out_lexical_baseline": held_baseline,
            "calibration_semantic_all_candidates": calibration_all_candidates,
            "held_out_semantic_all_candidates": held_all_candidates,
        })
    return {
        "schema_version": 1, "evaluation_version": EVALUATION_VERSION,
        **scope, "retrieval_surface": retrieval_surface, "profiles": profiles,
        "threshold_kind": "post_retriever_candidate_score_cutoff",
        "candidate_generation": "inherited_from_current_retriever",
        "production_threshold_changed": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("calibration-semantic", "held-out-semantic",
                 "calibration-lexical", "held-out-lexical"):
        parser.add_argument(f"--{name}", required=True, type=Path)
    parser.add_argument("--surface", choices=("project", "memory"), required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    reports = tuple(json.loads(getattr(args, name.replace("-", "_")).read_text(
        encoding="utf-8")) for name in ("calibration_semantic", "held_out_semantic",
                                         "calibration_lexical", "held_out_lexical"))
    result = analyze_scoped_thresholds(*reports, retrieval_surface=args.surface)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "written", "output": str(args.output),
                      "profile_count": len(result["profiles"]),
                      "statuses": [row["status"] for row in result["profiles"]]},
                     separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
