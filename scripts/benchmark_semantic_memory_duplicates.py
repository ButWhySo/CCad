"""Measure real local-embedding duplicate separation on labeled CCad memories."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from memory_manager import MemoryManager
from semantic_retrieval import OllamaEmbeddingBackend


PAIRS = {
    "duplicate": [
        ("The buck converter steps down the input supply to provide a regulated output voltage.", "A buck regulator converts a higher input rail into a stable lower voltage."),
        ("The ground return should connect the input capacitor to the power stage reference.", "Connect the power stage ground back to the input filter capacitor ground."),
        ("Keep the USB differential pair length matched.", "USB D+ and D- traces need equal electrical length."),
        ("The board outline measures 44 by 30 millimetres.", "PCB dimensions are 44 mm wide and 30 mm tall."),
        ("Use the front copper layer for the main signal route.", "Route the primary signal on F.Cu."),
        ("The connector footprint is J1.", "J1 is the board connector footprint."),
        ("Place the decoupling capacitor next to the IC power pin.", "Put the IC supply bypass capacitor close to its VCC pin."),
        ("The reset input is active low.", "Reset is asserted when its signal is low."),
        ("The output voltage target is 3.3 volts.", "Set the regulator output to 3V3."),
        ("Use two vias to change this signal from the top to bottom copper.", "Transition the net between F.Cu and B.Cu with a pair of vias."),
        ("The board uses metric units.", "All PCB dimensions are specified in millimetres."),
        ("The connector pin one marker faces the board edge.", "Orient connector pin 1 toward the PCB edge."),
        ("The switching frequency is 500 kilohertz.", "The converter switches at 0.5 MHz."),
        ("Do not route copper through the antenna keepout.", "The antenna clearance region must remain free of tracks."),
        ("The schematic power input pin connects to VIN.", "VIN is connected to the symbol's power-in pin."),
        ("The mounting holes are not connected to ground.", "Keep the mechanical mounting holes electrically isolated from GND."),
        ("Use a 0.25 millimetre trace for the power rail.", "Set the supply rail track width to 250 microns."),
        ("The IC reference designator is U3.", "This integrated circuit is labelled U3 on the board."),
        ("The board has four copper layers.", "There are four conductive layers in the PCB stackup."),
        ("The input capacitor is C1.", "C1 is the power input filtering capacitor."),
        ("The output net is named VOUT.", "Use VOUT as the regulator's output net name."),
        ("The via drill is 0.3 millimetres.", "Each via has a 300 micron drill diameter."),
        ("Keep the crystal close to the microcontroller clock pins.", "Locate the oscillator next to the MCU clock inputs."),
        ("The schematic contains one power sheet and one control sheet.", "There are two schematic sheets: power and control."),
    ],
    "distinct": [
        ("The buck converter steps down the input supply to provide a regulated output voltage.", "The buck converter switches at 500 kilohertz."),
        ("The ground return should connect the input capacitor to the power stage reference.", "The ground return must stay 0.5 mm away from the board edge."),
        ("Keep the USB differential pair length matched.", "Keep the USB data pair impedance at 90 ohms."),
        ("The board outline measures 44 by 30 millimetres.", "The PCB outline must fit inside a 50 by 40 mm enclosure."),
        ("Use the front copper layer for the main signal route.", "Use the front copper layer for the 3.3 V power plane."),
        ("The connector footprint is J1.", "J1 pin 1 carries the 5 V input."),
        ("Place the decoupling capacitor next to the IC power pin.", "Use a 100 nF decoupling capacitor at the IC power pin."),
        ("The reset input is active low.", "The reset input has a 10 kilohm pull-up resistor."),
        ("The output voltage target is 3.3 volts.", "The output current limit is 3.3 amperes."),
        ("Use two vias to change this signal from the top to bottom copper.", "Use two vias with a 0.3 mm drill for this signal."),
        ("The board uses metric units.", "The board coordinate origin is at 0,0."),
        ("The connector pin one marker faces the board edge.", "The connector pin one marker is a 1 mm silkscreen triangle."),
        ("The switching frequency is 500 kilohertz.", "The switching frequency must stay below 500 kHz to reduce EMI."),
        ("Do not route copper through the antenna keepout.", "The antenna keepout begins 5 mm from the board edge."),
        ("The schematic power input pin connects to VIN.", "The schematic power input pin is pin number 3."),
        ("The mounting holes are not connected to ground.", "The mounting holes are 3.2 mm in diameter."),
        ("Use a 0.25 millimetre trace for the power rail.", "The power rail current is limited to 0.25 amperes."),
        ("The IC reference designator is U3.", "U3 uses the QFN-32 footprint."),
        ("The board has four copper layers.", "The board has four mounting holes."),
        ("The input capacitor is C1.", "C1 has a capacitance of 10 microfarads."),
        ("The output net is named VOUT.", "The output net VOUT is assigned to B.Cu."),
        ("The via drill is 0.3 millimetres.", "The via annular ring is 0.3 mm wide."),
        ("Keep the crystal close to the microcontroller clock pins.", "The crystal load capacitors connect to ground."),
        ("The schematic contains one power sheet and one control sheet.", "The control sheet contains one processor and one connector."),
    ],
}
WORD = MemoryManager._word
LEXICAL_THRESHOLD = MemoryManager.NEAR_DUPLICATE_THRESHOLD


def lexical_match(left: str, right: str) -> bool:
    a, b = set(WORD.findall(left.casefold())), set(WORD.findall(right.casefold()))
    return (len(a) >= 5 and len(b) >= 5 and
            len(a & b) / len(a | b) >= LEXICAL_THRESHOLD)


def evaluate(base_url: str, model: str, thresholds: list[float]) -> dict:
    if not thresholds or any(not 0.0 <= threshold <= 1.0 for threshold in thresholds):
        raise ValueError("thresholds must be between 0 and 1")
    backend = OllamaEmbeddingBackend(base_url, model)
    state = backend.check_ready()
    if not state["ready"]:
        raise RuntimeError(f"embedding backend not ready: {state['status']}")
    flattened = [text for rows in PAIRS.values() for pair in rows for text in pair]
    vectors = []
    for start in range(0, len(flattened), backend.MAX_TEXTS):
        vectors.extend(backend.embed_similarity_documents(
            flattened[start:start + backend.MAX_TEXTS]))
    if len(vectors) != len(flattened) or len({len(vector) for vector in vectors}) != 1:
        raise RuntimeError("embedding model returned inconsistent vector dimensions")
    scores = {label: [] for label in PAIRS}
    lexical_matches = {label: [] for label in PAIRS}
    offset = 0
    for label, rows in PAIRS.items():
        for _ in rows:
            left, right = vectors[offset:offset + 2]
            offset += 2
            scores[label].append(sum(a * b for a, b in zip(left, right)))
            lexical_matches[label].append(lexical_match(*rows[len(scores[label]) - 1]))

    measurements = []
    lexical_duplicate_hits = sum(lexical_matches["duplicate"])
    lexical_distinct_false_positives = sum(lexical_matches["distinct"])
    for threshold in thresholds:
        duplicate_predictions = [lexical or score >= threshold for lexical, score in
                                 zip(lexical_matches["duplicate"], scores["duplicate"])]
        distinct_predictions = [lexical or score >= threshold for lexical, score in
                               zip(lexical_matches["distinct"], scores["distinct"])]
        tp = sum(duplicate_predictions)
        fp = sum(distinct_predictions)
        fn = len(scores["duplicate"]) - tp
        tn = len(scores["distinct"]) - fp
        measurements.append({
            "threshold": threshold,
            "lexical_duplicate_hits": lexical_duplicate_hits,
            "lexical_false_positives": lexical_distinct_false_positives,
            "true_duplicates": tp,
            "false_duplicate_rejections": fp,
            "missed_duplicates": fn,
            "distinct_memories_kept": tn,
            "precision": tp / (tp + fp) if tp + fp else 1.0,
            "recall": tp / len(scores["duplicate"]),
        })
    return {
        "model": model,
        "model_digest": state["model_version"],
        "dimensions": len(vectors[0]),
        "pairs_per_class": {label: len(values) for label, values in scores.items()},
        "lexical_threshold": LEXICAL_THRESHOLD,
        "lexical_only": {
            "duplicate_hits": lexical_duplicate_hits,
            "distinct_false_positives": lexical_distinct_false_positives,
        },
        "score_ranges": {label: {"minimum": min(values), "maximum": max(values),
                                 "mean": sum(values) / len(values)}
                         for label, values in scores.items()},
        "highest_pair": {
            label: {"pair_number": values.index(max(values)) + 1,
                    "similarity": max(values)}
            for label, values in scores.items()
        },
        "thresholds": measurements,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-url", default="http://127.0.0.1:11434")
    parser.add_argument("--model", default="embeddinggemma")
    parser.add_argument("--threshold", type=float, nargs="+",
                        default=[0.70, 0.75, 0.80, 0.85, 0.90, 0.91, 0.95])
    args = parser.parse_args()
    try:
        print(json.dumps(evaluate(args.base_url, args.model, args.threshold),
                         indent=2, sort_keys=True))
    except (RuntimeError, ValueError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
