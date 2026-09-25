"""Measure cold, cached, and incremental retrieval on typed PCB geometry."""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from project_index import ProjectIndex


def snapshot_for(count: int) -> dict:
    tracks = []
    for index in range(count):
        x_mm = float(index % 1000)
        y_mm = float(index // 1000)
        tracks.append({
            "id": f"T{index:06d}", "net_id": f"N{index % 32:02d}",
            "layer_id": "F.Cu", "width_nm": 200_000,
            "start": {"x_mm": x_mm, "y_mm": y_mm},
            "end": {"x_mm": x_mm + 1.0, "y_mm": y_mm},
        })
    return {"typed_state": {"available": True, "project": {
        "id": "project-index-benchmark", "board": {
            "layers": [{"id": "F.Cu", "name": "Front copper"}],
            "tracks": tracks, "footprints": [], "pads": [], "vias": [],
            "zones": [], "track_arcs": [], "graphics": [], "texts": [],
        }, "components": [], "nets": [],
    }}}


def timed(callable_):
    started = time.perf_counter()
    result = callable_()
    return result, (time.perf_counter() - started) * 1000.0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--objects", type=int, default=10_000,
                        help="number of typed board tracks to index (1..50000)")
    args = parser.parse_args()
    if not 1 <= args.objects <= 50_000:
        parser.error("--objects must be between 1 and 50000")

    snapshot = snapshot_for(args.objects)
    index = ProjectIndex(max_entities=32, max_chars=16_000)
    first, cold_ms = timed(lambda: index.retrieve(snapshot, "T000000", limit=32))
    cached, cached_ms = timed(lambda: index.retrieve(snapshot, "T000000", limit=32))
    snapshot["typed_state"]["project"]["board"]["tracks"][0]["end"]["x_mm"] += 1.0
    incremental, incremental_ms = timed(
        lambda: index.retrieve(snapshot, "T000000", limit=32))
    print(json.dumps({
        "benchmark": "typed_project_index_c3",
        "input_track_count": args.objects,
        "indexed_entity_count": first["stats"]["total_entities"],
        "cold_build_ms": round(cold_ms, 3),
        "cached_query_ms": round(cached_ms, 3),
        "incremental_single_entity_update_ms": round(incremental_ms, 3),
        "cold_state": first["stats"]["index_state"],
        "cached_state": cached["stats"]["index_state"],
        "incremental_state": incremental["stats"]["index_state"],
        "incremental_updated_count": incremental["stats"]["updated_count"],
        "query_result_count": len(incremental["entities"]),
        "revision_changed": first["revision"] != incremental["revision"],
    }, sort_keys=True))
    if (first["stats"]["index_state"] != "built" or
            cached["stats"]["index_state"] != "cached" or
            incremental["stats"]["index_state"] != "incremental" or
            incremental["stats"]["updated_count"] != 1 or
            first["revision"] == incremental["revision"]):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
