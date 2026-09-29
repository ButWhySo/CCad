"""Run versioned relevance and systems measurements through CCad's real retrievers.

The benchmark uses controlled, source-revisioned fixtures. It never calls a hosted
provider. Local semantic retrieval is opt-in and uses the configured Ollama model.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import statistics
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from conversation_store import ConversationStore  # noqa: E402
from langchain_core.messages import AIMessage, HumanMessage  # noqa: E402
from memory_manager import MemoryManager  # noqa: E402
from memory_store import MemoryStore  # noqa: E402
from project_index import ProjectIndex  # noqa: E402
from retrieval_adapters import MemoryManagerRetriever, ProjectIndexRetriever  # noqa: E402
from retrieval_contracts import RetrievalChannel, RetrievalRequest  # noqa: E402

DATASET_PATH = ROOT / "scripts" / "fixtures" / "agent_retrieval_dataset_v1.json"
K_VALUES = (1, 3, 5)
TASK_CHANNELS = {
    "exact_cad": (RetrievalChannel.EXACT, RetrievalChannel.LEXICAL),
    "lexical_engineering": (RetrievalChannel.LEXICAL, RetrievalChannel.EXACT),
    "semantic_paraphrase": (RetrievalChannel.SEMANTIC, RetrievalChannel.LEXICAL),
    "hard_negative": (RetrievalChannel.EXACT, RetrievalChannel.LEXICAL),
    "graph_relationship": (RetrievalChannel.GRAPH, RetrievalChannel.EXACT,
                           RetrievalChannel.LEXICAL),
    "spatial_geometry": (RetrievalChannel.SPATIAL, RetrievalChannel.GRAPH,
                         RetrievalChannel.EXACT),
    "memory": (RetrievalChannel.LEXICAL, RetrievalChannel.SEMANTIC),
}


def _snapshot(fixture_name: str) -> dict[str, Any]:
    """Return deterministic native-shaped board fixtures, not synthetic hits."""
    if fixture_name == "power_small":
        positions = {
            "U1": (0.0, 0.0), "C1": (0.45, 0.20), "C2": (1.55, 0.0),
            "C3": (13.0, 8.0), "R1": (8.0, 8.0), "R2": (11.0, 8.0),
            "U2": (25.0, 15.0), "U3": (31.0, 20.0), "J2": (40.0, 5.0),
        }
        values = {
            "U1": ("MCU controller", "microcontroller controller"),
            "C1": ("100 nF capacitor", "decoupling capacitor on VDD supply"),
            "C2": ("10 uF capacitor", "bulk capacitor on VDD supply"),
            "C3": ("1 uF capacitor", "local filter capacitor"),
            "R1": ("10 kOhm resistor", "reset pull-up for switch-open state"),
            "R2": ("1 kOhm resistor", "LED current limiting resistor"),
            "U2": ("USB interface", "USB transceiver"),
            "U3": ("power regulator", "switching converter"),
            "J2": ("USB connector", "USB-C receptacle"),
        }
        footprints = []
        for ref, (x, y) in positions.items():
            value, description = values[ref]
            footprints.append({"reference": ref, "value": value,
                               "description": description,
                               "footprint_name": "CCad:BenchmarkFootprint",
                               "layer_id": "F.Cu",
                               "position": {"x_mm": x, "y_mm": y}})
        footprints[0]["net_id"] = "+3V3"
        footprints[1]["net_id"] = "+3V3"
        footprints[2]["net_id"] = "+3V3"
        pads = [
            {"id": "J2.1", "component_id": "J2", "pin_number": "1",
             "pin_name": "VBUS", "net_id": "+5V", "position": {"x_mm": 40.0, "y_mm": 5.0}},
            {"id": "J2.5", "component_id": "J2", "pin_number": "5",
             "pin_name": "CC1", "net_id": "CC1", "position": {"x_mm": 40.0, "y_mm": 6.0}},
        ]
        board = {"id": "power-small-board", "footprints": footprints, "pads": pads,
                 "tracks": [{"id": "T1", "net_id": "+3V3", "layer_id": "F.Cu",
                             "start": {"x_mm": 0.0, "y_mm": 0.0},
                             "end": {"x_mm": 10.0, "y_mm": 0.0}},
                            {"id": "T2", "net_id": "SPI_CLK", "layer_id": "F.Cu",
                             "start": {"x_mm": 4.0, "y_mm": 4.0},
                             "end": {"x_mm": 8.0, "y_mm": 4.0}}],
                 "zones": [{"id": "ground-plane", "name": "ground copper zone",
                            "net_id": "GND", "area": {"x_mm": 0, "y_mm": 0,
                                                         "width_mm": 40, "height_mm": 25}}],
                 "design_rules": {"usb_length_match_nm": 100000,
                                  "reset_pullup_ohms": 10000}}
    elif fixture_name == "controller_large":
        footprints = [
            {"reference": "U1", "value": "controller", "description": "main controller",
             "position": {"x_mm": 12, "y_mm": 12}, "layer_id": "F.Cu",
             "net_id": "+3V3"},
            {"reference": "U2", "value": "USB bridge", "description": "USB bridge",
             "position": {"x_mm": 16, "y_mm": 12}, "layer_id": "F.Cu"},
            {"reference": "U4", "value": "RF module", "description": "radio module",
             "position": {"x_mm": 55, "y_mm": 40}, "layer_id": "F.Cu"},
        ]
        for index in range(1, 13):
            reference = f"C{index}"
            x, y = 20 + (index % 6) * 4, 15 + (index // 6) * 5
            footprints.append({"reference": reference, "value": "100 nF capacitor",
                               "description": "controller rail decoupling capacitor",
                               "position": {"x_mm": x, "y_mm": y}, "layer_id": "F.Cu",
                               "net_id": "VDD" if reference in {"C7", "C8"} else "GND"})
        for index in range(1, 10):
            reference = f"R{index}"
            description = ("reset pull-up holds the reset input high when switch open"
                           if reference == "R6" else "signal resistor")
            footprints.append({"reference": reference, "value": "10 kOhm resistor",
                               "description": description,
                               "position": {"x_mm": 45 + index, "y_mm": 12},
                               "layer_id": "F.Cu"})
        for index in range(2, 6):
            reference = f"J{index}"
            footprints.append({"reference": reference, "value": "board connector",
                               "description": "controller board connector",
                               "position": {"x_mm": 5 + index * 2, "y_mm": 65},
                               "layer_id": "F.Cu"})
        pads = [{"id": "J3.1", "component_id": "J3", "pin_number": "1",
                 "pin_name": "SCL", "net_id": "I2C_SCL",
                 "position": {"x_mm": 30, "y_mm": 20}}]
        board = {"id": "controller-large-board", "footprints": footprints, "pads": pads,
                 "tracks": [{"id": "T20", "net_id": "GND", "layer_id": "F.Cu",
                             "start": {"x_mm": 70, "y_mm": 20},
                             "end": {"x_mm": 80, "y_mm": 20}},
                            {"id": "T21", "net_id": "VDD", "layer_id": "F.Cu",
                             "start": {"x_mm": 72, "y_mm": 24},
                             "end": {"x_mm": 83, "y_mm": 24}}],
                 "zones": [{"id": "rf-ground", "name": "RF ground zone",
                            "net_id": "GND", "area": {"x_mm": 50, "y_mm": 35,
                                                         "width_mm": 15, "height_mm": 12}}],
                 "placement_regions": [{"id": "rf-keepout", "name": "RF keepout",
                                        "area": {"origin": {"x_mm": 52, "y_mm": 37},
                                                 "size": {"width_mm": 8, "height_mm": 8}}}],
                 "design_rules": {"reset_pullup_ohms": 10000}}
    else:
        raise ValueError(f"unsupported_project_fixture:{fixture_name}")
    return {"typed_state": {"available": True,
                            "project": {"id": fixture_name, "board": board}},
            "active_pcb_layer_id": "F.Cu"}


def evaluate_case(result_ids: Iterable[str], relevant_ids: set[str],
                  distractor_ids: set[str], *, k_values=K_VALUES) -> dict[str, Any]:
    ranked = list(dict.fromkeys(str(item) for item in result_ids))
    relevant = set(relevant_ids)
    distractors = set(distractor_ids)
    first = next((index for index, item in enumerate(ranked, 1) if item in relevant), None)
    metrics: dict[str, Any] = {
        "mrr": 1.0 / first if first is not None else 0.0,
        "false_positive": int(not relevant and bool(ranked)),
        "hard_negative_rejected": int(not relevant and not ranked),
        "distractor_hits": len(set(ranked).intersection(distractors)),
    }
    for k in k_values:
        top = ranked[:k]
        hits = sum(item in relevant for item in top)
        metrics[f"recall_at_{k}"] = hits / len(relevant) if relevant else 0.0
        metrics[f"precision_at_{k}"] = hits / k
        dcg = sum(1.0 / math.log2(rank + 1)
                  for rank, item in enumerate(top, 1) if item in relevant)
        ideal = sum(1.0 / math.log2(rank + 1)
                    for rank in range(1, min(len(relevant), k) + 1))
        metrics[f"ndcg_at_{k}"] = dcg / ideal if ideal else 0.0
    return metrics


def aggregate_metrics(rows: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = {}
    for row in rows:
        grouped.setdefault(str(row["task"]), []).append(row["metrics"])
    output = {}
    for task, cases in sorted(grouped.items()):
        keys = sorted(set.intersection(*(set(case) for case in cases)))
        output[task] = {"case_count": len(cases), **{
            key: round(statistics.fmean(float(case[key]) for case in cases), 6)
            for key in keys}}
    return output


def _percentile(samples: list[float], percentile: float) -> float | None:
    if not samples:
        return None
    ordered = sorted(samples)
    return round(ordered[max(0, math.ceil(percentile * len(ordered)) - 1)], 6)


def _peak_rss_bytes() -> int | None:
    if os.name == "nt":
        import ctypes
        from ctypes import wintypes

        class Counters(ctypes.Structure):
            _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD),
                        ("PeakWorkingSetSize", ctypes.c_size_t),
                        ("WorkingSetSize", ctypes.c_size_t),
                        ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                        ("QuotaPagedPoolUsage", ctypes.c_size_t),
                        ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                        ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                        ("PagefileUsage", ctypes.c_size_t),
                        ("PeakPagefileUsage", ctypes.c_size_t)]

        counters = Counters()
        counters.cb = ctypes.sizeof(counters)
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.GetCurrentProcess.restype = wintypes.HANDLE
        process = kernel32.GetCurrentProcess()
        psapi = ctypes.WinDLL("psapi", use_last_error=True)
        psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE,
                                               ctypes.POINTER(Counters),
                                               wintypes.DWORD]
        psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
        if psapi.GetProcessMemoryInfo(process, ctypes.byref(counters), counters.cb):
            return int(counters.PeakWorkingSetSize)
        return None
    try:
        import resource
        value = int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)
        return value if sys.platform == "darwin" else value * 1024
    except (ImportError, OSError, ValueError):
        return None


def _snapshot_aliases(snapshot: dict[str, Any]) -> dict[str, str]:
    project = snapshot["typed_state"]["project"]
    board = project["board"]
    aliases = {}
    for item in board.get("footprints", []):
        aliases[f"component:{item['reference']}"] = f"footprint:{item['reference']}"
    for item in board.get("pads", []):
        aliases[f"pad:{item['id']}"] = f"pad:{item['id']}"
    for item in board.get("tracks", []):
        aliases[f"track:{item['id']}"] = f"track:{item['id']}"
    for item in board.get("zones", []):
        aliases[f"zone:{item['id']}"] = f"zone:{item['id']}"
    for item in board.get("placement_regions", []):
        aliases[f"zone:{item['id']}"] = f"placement_region:{item['id']}"
    if board.get("design_rules"):
        aliases["design_rules:board-design-rules"] = "design_rules:board-design-rules"
    for key in board.get("design_rules", {}):
        alias = "usb-length" if key == "usb_length_match_nm" else "reset-pullup"
        aliases[f"constraint:{alias}"] = f"design_rule_setting:board.design_rules.{key}"
    net_ids = {item.get("net_id") for group in ("footprints", "pads", "tracks", "zones")
               for item in board.get(group, []) if item.get("net_id")}
    for net_id in net_ids:
        aliases[f"net:{net_id}"] = f"board_net:{net_id}"
    return aliases


def _seed_memories(manager: MemoryManager, fixture: str) -> dict[str, str]:
    manager.configure({"working_memory": False, "ltm": True, "episodic": False})
    records = [
        ("memory:preferred-track-width", "Preferred signal width",
         "Use 0.25 mm as the default signal track width on this project.", "preference", 5),
        ("memory:old-track-width-correction", "Previous width correction",
         "Earlier revision used 0.20 mm signal track width before correction.", "correction", 1),
        ("memory:rejected-ground-bridge", "Rejected ground bridge",
         "User rejected a copper bridge near the switching node because it couples switching noise.", "correction", 5),
        ("memory:accepted-ground-return", "Accepted ground return",
         "Keep a separate short ground return beside the switching node.", "preference", 3),
    ]
    identities: dict[str, str] = {}
    for benchmark_id, title, content, kind, importance in records:
        entry = manager.add(content, tier="ltm", title=title, scope="project",
                            tags=["retrieval-benchmark", fixture], kind=kind,
                            importance=importance)
        entry_id = entry.get("id") if isinstance(entry, dict) else None
        if not isinstance(entry_id, str) or not entry_id:
            raise RuntimeError("benchmark_memory_add_missing_id")
        identities[entry_id] = benchmark_id
    return identities


def _seed_turns(store: ConversationStore) -> dict[str, str]:
    records = [
        ("usb-placement-decision", "USB connector placement",
         "Place USB connector J2 along the left board edge to preserve the enclosure opening.",
         "Position connector J2 on the left board edge."),
        ("usb-esd-review", "USB ESD review",
         "Review identified the USB ESD device location beside connector J2.",
         "Place the ESD protection device adjacent to the USB connector."),
    ]
    identities = {}
    for turn_id, _label, user_text, assistant_text in records:
        store.ensure_thread("retrieval-benchmark-thread", project_id="retrieval-benchmark")
        messages = [HumanMessage(content=user_text), AIMessage(content=assistant_text)]
        store.append_messages("retrieval-benchmark-thread", messages, turn_id=turn_id,
                              project_id="retrieval-benchmark")
        store.record_turn("retrieval-benchmark-thread", turn_id, user_text, messages,
                          outcome="completed", project_id="retrieval-benchmark",
                          project_revision_before="turn-fixture-v1")
        identities[turn_id] = f"turn:{turn_id}"
    return identities


def run_benchmark(dataset: dict[str, Any], *, split: str, work_dir: Path,
                  warmups: int = 3, repetitions: int = 30,
                  local_semantic: bool = False, ollama_url: str = "http://127.0.0.1:11434",
                  embedding_model: str = "embeddinggemma") -> dict[str, Any]:
    if split not in {"calibration", "held_out"}:
        raise ValueError("benchmark_split_invalid")
    if warmups < 0 or repetitions < 1 or repetitions > 1000:
        raise ValueError("benchmark_run_bounds_invalid")
    work_dir.mkdir(parents=True, exist_ok=True)
    started = time.perf_counter()
    fixture_snapshots = {name: _snapshot(name) for name in ("power_small", "controller_large")}
    project_indexes = {name: ProjectIndex(max_entities=32, max_chars=16000)
                       for name in fixture_snapshots}
    project_aliases = {name: _snapshot_aliases(snapshot)
                       for name, snapshot in fixture_snapshots.items()}
    build_times = {}
    update_times = {}
    for name, snapshot in fixture_snapshots.items():
        before = time.perf_counter_ns()
        project_indexes[name]._sync(snapshot)
        build_times[name] = (time.perf_counter_ns() - before) / 1_000_000
        updated = json.loads(json.dumps(snapshot))
        updated["typed_state"]["project"]["board"].setdefault("texts", []).append(
            {"id": "benchmark-incremental-probe", "text": "incremental retrieval probe"})
        before = time.perf_counter_ns()
        project_indexes[name]._sync(updated)
        update_times[name] = (time.perf_counter_ns() - before) / 1_000_000
        fixture_snapshots[name] = updated

    memory_path = work_dir / "benchmark-memory.json"
    conversation_path = work_dir / "benchmark-conversation.sqlite3"
    manager = MemoryManager(MemoryStore(memory_path), thread_id="retrieval-benchmark-thread",
                            project_id="retrieval-benchmark")
    memory_aliases = _seed_memories(manager, "memory_records")
    semantic_state = {"status": "disabled", "enabled": False, "model": ""}
    if local_semantic:
        failures = manager.configure({"working_memory": False, "ltm": True, "episodic": False,
                                      "semantic": {"enabled": True, "backend": "ollama_local",
                                                   "base_url": ollama_url,
                                                   "model": embedding_model}})
        semantic_state = manager.semantic_state()
        if failures or not semantic_state.get("ready"):
            raise RuntimeError("local_semantic_backend_not_ready")
    store = ConversationStore(conversation_path)
    turn_aliases = _seed_turns(store)
    cases = [case for case in dataset["cases"] if case["split"] == split]
    reports = []
    latencies = []
    for case in cases:
        task = case["task"]
        channels = TASK_CHANNELS.get(task, ())
        timings = []
        result_ids: list[str] = []
        source_revision = ""
        status = "measured"
        retrieval_status = "ready"
        channel_names: list[str] = []
        context_chars = 0
        context_bytes = 0
        retrieval_calls = 0

        def retrieve_once():
            nonlocal result_ids, source_revision, status, retrieval_status
            nonlocal channel_names, context_chars, context_bytes, retrieval_calls
            retrieval_calls += 1
            if task in {"memory"}:
                request = RetrievalRequest(
                    query=case["query"], project_id="retrieval-benchmark",
                    thread_id="retrieval-benchmark-thread", requested_revision="memory-fixture-v1",
                    scope="memory", top_k=10, candidate_budget=32,
                    channels=(RetrievalChannel.LEXICAL, RetrievalChannel.SEMANTIC))
                result = MemoryManagerRetriever(manager).retrieve(request)
                result_ids = [memory_aliases.get(hit.canonical_id,
                                                 f"unmapped:{hit.canonical_id}")
                              for hit in result.hits]
                source_revision = "memory-fixture-v1"
                retrieval_status = result.status.value
                status = "measured" if retrieval_status == "ready" else "unavailable"
                channel_names = sorted({hit.channel.value for hit in result.hits})
                encoded = [json.dumps(hit.content, ensure_ascii=False,
                                      separators=(",", ":")) for hit in result.hits]
                context_chars = sum(len(item) for item in encoded)
                context_bytes = sum(len(item.encode("utf-8")) for item in encoded)
                return
            if task == "historical_turn_record":
                result = store.search_turn_records("retrieval-benchmark-thread",
                                                   case["query"], limit=10)
                result_ids = [turn_aliases.get(str(item.get("turn_id", "")),
                                               f"unmapped:{item.get('turn_id', '')}")
                              for item in result]
                source_revision = "turn-fixture-v1"
                retrieval_status = "ready"
                status = "measured"
                channel_names = ["historical_turn_lexical"] if result else []
                encoded = [json.dumps(item, ensure_ascii=False,
                                      separators=(",", ":")) for item in result]
                context_chars = sum(len(item) for item in encoded)
                context_bytes = sum(len(item.encode("utf-8")) for item in encoded)
                return
            fixture = case["fixture"]
            snapshot = fixture_snapshots[fixture]
            fixture_meta = dataset["fixtures"][fixture]
            request = RetrievalRequest(
                query=case["query"], project_id=fixture, scope="project",
                requested_revision=fixture_meta.get("project_revision", ""),
                entity_types=tuple(case.get("entity_types", ())),
                top_k=10, candidate_budget=100, channels=channels)
            retriever = ProjectIndexRetriever(
                project_indexes[fixture], snapshot, project_id=fixture,
                embedding_backend=manager.semantic_embedding_backend)
            result = retriever.retrieve(request)
            aliases = project_aliases[fixture]
            result_ids = [next((alias for alias, native in aliases.items()
                                if native == hit.canonical_id),
                               f"unmapped:{hit.canonical_id}") for hit in result.hits]
            source_revision = result.revision or fixture_meta.get("source_revision", "")
            retrieval_status = result.status.value
            status = "measured" if retrieval_status in {"ready", "partial"} else "unavailable"
            channel_names = sorted({hit.channel.value for hit in result.hits})
            encoded = [json.dumps(hit.content, ensure_ascii=False,
                                  separators=(",", ":")) for hit in result.hits]
            context_chars = sum(len(item) for item in encoded)
            context_bytes = sum(len(item.encode("utf-8")) for item in encoded)

        for _ in range(warmups):
            retrieve_once()
        retrieval_calls = 0
        for _ in range(repetitions):
            before = time.perf_counter_ns()
            retrieve_once()
            timings.append((time.perf_counter_ns() - before) / 1_000_000)
        latencies.extend(timings)
        metrics = evaluate_case(result_ids, set(case["expected_ids"]),
                                set(case["distractor_ids"]))
        reports.append({
            "case_id": case["id"], "task": task, "fixture": case["fixture"],
            "status": status, "retrieval_status": retrieval_status,
            "semantic_search_allowed": case["semantic_search_allowed"],
            "semantic_backend_status": semantic_state.get("status", "disabled"),
            "requested_channels": ([channel.value for channel in channels]
                                   if task not in {"memory", "historical_turn_record"}
                                   else ([RetrievalChannel.LEXICAL.value,
                                          RetrievalChannel.SEMANTIC.value]
                                         if task == "memory" else
                                         ["historical_turn_lexical"])),
            "channels_available_on_hits": channel_names,
            "source_revision": source_revision,
            "expected_ids": list(case["expected_ids"]), "result_ids": result_ids,
            "metrics": metrics, "channels_used": channel_names,
            "context": {"relevant_facts_included": len(set(result_ids) &
                                                           set(case["expected_ids"])),
                        "irrelevant_facts_included": len(set(result_ids) -
                                                           set(case["expected_ids"])),
                        "bytes": context_bytes,
                        "characters": context_chars,
                        "estimated_tokens_char_div_4": math.ceil(context_chars / 4),
                        "token_estimate_method": "characters_divided_by_4_heuristic",
                        "retrieval_calls": retrieval_calls,
                        "follow_up_tool_calls": None,
                        "follow_up_tool_calls_status": "not_measured_without_agent_model_turn"},
            "timing_ms": {"p50": _percentile(timings, 0.50),
                          "p95": _percentile(timings, 0.95),
                          "p99": _percentile(timings, 0.99) if len(timings) >= 100 else None,
                          "p99_status": "measured" if len(timings) >= 100 else
                                        "sample_count_below_100"},
        })

    false_negative = sum(bool(case["expected_ids"]) and
                         not set(row["result_ids"]).intersection(case["expected_ids"])
                         for case, row in zip(cases, reports))
    negatives = [row for row in reports if not row["expected_ids"]]
    payload = json.dumps(dataset, sort_keys=True, separators=(",", ":")).encode("utf-8")
    git_commit = ""
    try:
        git_commit = subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT,
                                    check=True, capture_output=True, text=True,
                                    timeout=3).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        git_commit = "unavailable"
    memory_disk = memory_path.stat().st_size if memory_path.exists() else 0
    conversation_disk = sum(path.stat().st_size for path in work_dir.glob(
        "benchmark-conversation.sqlite3*"))
    return {
        "schema_version": 1, "benchmark_version": "1.0.0",
        "dataset_id": dataset["dataset_id"], "dataset_version": dataset["dataset_version"],
        "dataset_sha256": hashlib.sha256(payload).hexdigest(), "split": split,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "git_commit": git_commit, "platform": platform.platform(),
        "runtime": {"python": platform.python_version(), "backend": "CCad in-process retrieval",
                    "semantic_enabled": bool(local_semantic),
                    "semantic_status": semantic_state.get("status", "disabled"),
                    "semantic_model": semantic_state.get("model", "") if local_semantic else ""},
        "summary": {"case_count": len(reports), "measured_case_count": sum(
                        row["status"] not in {"unavailable", "failed"} for row in reports),
                    "false_negative_case_count": false_negative,
                    "hard_negative_case_count": len(negatives),
                    "false_positive_case_count": sum(row["metrics"]["false_positive"]
                                                      for row in negatives),
                    "hard_negative_rejection_rate": round(
                        statistics.fmean(row["metrics"]["hard_negative_rejected"]
                                         for row in negatives), 6) if negatives else None},
        "quality_by_task": aggregate_metrics(reports), "cases": reports,
        "systems": {"project_index_build_ms_by_fixture": {
                        key: round(value, 6) for key, value in build_times.items()},
                    "incremental_update_ms_by_fixture": {
                        key: round(value, 6) for key, value in update_times.items()},
                    "query_latency_ms": {"p50": _percentile(latencies, 0.50),
                                         "p95": _percentile(latencies, 0.95),
                                         "p99": _percentile(latencies, 0.99)
                                         if len(latencies) >= 100 else None,
                                         "sample_count": len(latencies)},
                    "peak_rss_bytes": _peak_rss_bytes(),
                    "fixture_memory_store_disk_bytes": memory_disk,
                    "fixture_conversation_store_disk_bytes": conversation_disk,
                    "project_index_disk_bytes": 0,
                    "embedding_latency_ms": {"p50": None, "p95": None,
                                             "status": "not_separately_instrumented"},
                    "benchmark_execution_ms": round((time.perf_counter() - started) * 1000, 6),
                    "product_startup_latency_ms": None,
                    "backend_failure_recovery": "not_measured_no_persistent_search_backend",
                    "follow_up_tool_calls": "not_measured_offline_retrieval_only"},
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", type=Path, default=DATASET_PATH)
    parser.add_argument("--split", choices=("calibration", "held_out"), default="held_out")
    parser.add_argument("--output", type=Path, required=True,
                        help="Write versioned JSON report to an explicit path")
    parser.add_argument("--warmups", type=int, default=3)
    parser.add_argument("--repetitions", type=int, default=30)
    parser.add_argument("--local-semantic", action="store_true",
                        help="Use configured local Ollama embeddings; performs local model calls")
    parser.add_argument("--ollama-url", default="http://127.0.0.1:11434")
    parser.add_argument("--embedding-model", default="embeddinggemma")
    args = parser.parse_args()
    dataset = json.loads(args.dataset.read_text(encoding="utf-8"))
    with tempfile.TemporaryDirectory(prefix="ccad-retrieval-benchmark-") as temp:
        report = run_benchmark(dataset, split=args.split, work_dir=Path(temp),
                                warmups=args.warmups, repetitions=args.repetitions,
                                local_semantic=args.local_semantic,
                                ollama_url=args.ollama_url,
                                embedding_model=args.embedding_model)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                           encoding="utf-8")
    print(json.dumps({"status": "written", "output": str(args.output),
                      "dataset_version": report["dataset_version"],
                      "split": report["split"], "case_count": report["summary"]["case_count"],
                      "false_negative_case_count":
                          report["summary"]["false_negative_case_count"]},
                     separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
