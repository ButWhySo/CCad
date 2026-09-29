"""Measure real local-embedding duplicate separation on labeled CCad memories."""

from __future__ import annotations

import argparse
import hashlib
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
        ("Connect the regulator's enable input to the MCU GPIO.", "Wire the MCU control pin to the regulator enable pin."),
        ("The feedback divider sets the output voltage.", "Output voltage is determined by the resistor divider on FB."),
        ("Route the high-current loop with the shortest practical path.", "Minimize the length of the switching regulator's high-current loop."),
        ("The USB connector is mounted on the board edge.", "Place the USB receptacle at the PCB edge."),
        ("Keep the analog input away from the switching node.", "Separate the analog input trace from the switch-node copper."),
        ("Use a solid ground plane beneath the RF section.", "Provide continuous ground copper under the radio circuitry."),
        ("The status LED is driven by GPIO2.", "GPIO2 controls the indicator LED."),
        ("The input connector accepts 12 volts DC.", "Supply the board through its input connector with 12 V DC."),
        ("The thermal relief has four spokes.", "Use four copper spokes for the pad's thermal connection."),
        ("The board thickness is 1.6 millimetres.", "Specify a 1.6 mm finished PCB thickness."),
        ("The oscillator frequency is 16 megahertz.", "Use a 16 MHz crystal for the clock source."),
        ("The test point exposes the reset net.", "Connect the reset signal to the test point."),
        ("The connector footprint uses a 2.54 millimetre pitch.", "Set the connector pin spacing to 0.1 inch."),
        ("Keep the return path directly beneath the signal route.", "Route the ground return under the corresponding signal trace."),
        ("The board origin is at the lower-left corner.", "Set coordinate zero at the PCB's bottom-left corner."),
        ("The power switch is rated for 2 amperes.", "Use a 2 A rated switch on the power input."),
        ("Place the programming header near the board edge.", "Locate the debug connector close to the PCB edge."),
        ("The pull-up resistor connects SDA to 3.3 volts.", "Pull the I2C SDA line up to the 3V3 rail through the resistor."),
        ("The mounting pattern is 20 millimetres by 30 millimetres.", "Space the mounting holes on a 20 x 30 mm rectangle."),
        ("The MOSFET gate resistor is 10 ohms.", "Use a 10 Ω series resistor at the transistor gate."),
        ("Keep the differential pair on the same copper layer.", "Do not split the paired traces across different PCB layers."),
        ("The schematic sheet is named Power.", "Name the power-conversion schematic page Power."),
        ("The input fuse is rated at 1 ampere.", "Fit a 1 A fuse on the supply input."),
        ("Connect the exposed pad to ground with thermal vias.", "Ground the IC's exposed thermal pad using vias."),
        ("The board uses a 10 kilohm NTC thermistor.", "Use a 10 kΩ negative-temperature-coefficient sensor on the PCB."),
        ("The top copper pour is assigned to GND.", "Assign the F.Cu zone to the ground net."),
        ("Place the reset button beside the USB connector.", "Position the reset switch next to the USB receptacle."),
        ("The LED current is limited by a 1 kilohm resistor.", "A 1 kΩ series resistor sets the indicator LED current."),
        ("Use a 50 ohm trace for the RF feed.", "Set the antenna feedline characteristic impedance to 50 Ω."),
        ("The schematic's main supply net is called VIN.", "Name the primary input power net VIN."),
        ("The test pad is on the back copper layer.", "Place the measurement pad on B.Cu."),
        ("The board outline is rounded at the corners.", "Add corner radii to the PCB perimeter."),
        ("Use a 0.5 millimetre clearance around the antenna.", "Maintain 500 µm clearance from the RF antenna region."),
        ("The regulator has a 3.3 volt output.", "Set the LDO output to 3V3."),
        ("The SPI clock net is named SCLK.", "Label the serial peripheral clock signal SCLK."),
        ("Put the bulk capacitor close to the power connector.", "Locate the input reservoir capacitor next to the supply receptacle."),
        ("The board has two mounting slots.", "Include a pair of elongated mechanical mounting holes."),
        ("Connect the shield can to chassis ground.", "Bond the RF shield enclosure to the chassis-ground net."),
        ("Use a 0.2 millimetre via drill.", "Specify a 200 µm finished drill for the vias."),
        ("The controller communicates over I2C.", "Use I²C as the microcontroller's peripheral bus."),
        ("The protection diode is placed across the input supply.", "Connect the transient suppressor in parallel with the power input."),
        ("The board edge connector has 12 contacts.", "Use a twelve-position card-edge connector."),
        ("The regulator's switching node is SW.", "Name the converter switch-node net SW."),
        ("Place the crystal load capacitor close to the crystal.", "Locate the oscillator's load capacitor beside the resonator."),
        ("The board's maximum height is 8 millimetres.", "Keep total PCB assembly height under 8 mm."),
        ("Connect the ESD protection device next to the external port.", "Place the electrostatic-discharge suppressor by the I/O connector."),
        ("The schematic uses hierarchical labels between sheets.", "Connect pages through hierarchical sheet labels."),
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
        ("Connect the regulator's enable input to the MCU GPIO.", "The regulator's enable pin is active low."),
        ("The feedback divider sets the output voltage.", "The feedback divider uses two 10 kilohm resistors."),
        ("Route the high-current loop with the shortest practical path.", "The power stage should use 2 ounce copper."),
        ("The USB connector is mounted on the board edge.", "The USB connector supports USB 3.0 speeds."),
        ("Keep the analog input away from the switching node.", "The analog input has a range of 0 to 3.3 volts."),
        ("Use a solid ground plane beneath the RF section.", "The RF section operates at 2.4 gigahertz."),
        ("The status LED is driven by GPIO2.", "GPIO2 is configured as an open-drain output."),
        ("The input connector accepts 12 volts DC.", "The input connector is a barrel jack with a 5.5 mm shell."),
        ("The thermal relief has four spokes.", "The thermal relief gap is 0.3 millimetres."),
        ("The board thickness is 1.6 millimetres.", "The finished board weighs 1.6 grams."),
        ("The oscillator frequency is 16 megahertz.", "The oscillator has a 16 pF load capacitance."),
        ("The test point exposes the reset net.", "The reset signal is pulled high during normal operation."),
        ("The connector footprint uses a 2.54 millimetre pitch.", "The connector has 2.54 mm-wide pads."),
        ("Keep the return path directly beneath the signal route.", "The return path should connect to the ground plane at both ends."),
        ("The board origin is at the lower-left corner.", "The lower-left mounting hole is located at (0, 0)."),
        ("The power switch is rated for 2 amperes.", "The power switch has two mechanical poles."),
        ("Place the programming header near the board edge.", "The programming header uses a 1.27 mm pitch."),
        ("The pull-up resistor connects SDA to 3.3 volts.", "The SDA pull-up resistor has a value of 4.7 kilohms."),
        ("The mounting pattern is 20 millimetres by 30 millimetres.", "The PCB measures 20 by 30 millimetres overall."),
        ("The MOSFET gate resistor is 10 ohms.", "The MOSFET drain current is limited to 10 amperes."),
        ("Keep the differential pair on the same copper layer.", "The differential pair should have 100 ohm differential impedance."),
        ("The schematic sheet is named Power.", "The Power sheet contains a 12 V input connector."),
        ("The input fuse is rated at 1 ampere.", "The fuse has a 1 A time-delay characteristic."),
        ("Connect the exposed pad to ground with thermal vias.", "The exposed pad has a 3 mm by 3 mm copper area."),
        ("The board uses a 10 kilohm NTC thermistor.", "The thermistor measures 10 degrees Celsius at room temperature."),
        ("The top copper pour is assigned to GND.", "The ground zone is set to a 0.25 mm clearance."),
        ("Place the reset button beside the USB connector.", "The reset button is a 6 mm tactile switch."),
        ("The LED current is limited by a 1 kilohm resistor.", "The LED forward voltage is 1 kilovolt."),
        ("Use a 50 ohm trace for the RF feed.", "The RF trace is 50 millimetres long."),
        ("The schematic's main supply net is called VIN.", "The VIN net carries a maximum of 2 amperes."),
        ("The test pad is on the back copper layer.", "The test pad is 2 mm in diameter."),
        ("The board outline is rounded at the corners.", "The board outline has a 2 mm corner radius."),
        ("Use a 0.5 millimetre clearance around the antenna.", "The antenna keepout is 0.5 square centimetres."),
        ("The regulator has a 3.3 volt output.", "The regulator can supply 3.3 amperes."),
        ("The SPI clock net is named SCLK.", "The SPI clock frequency is 10 MHz."),
        ("Put the bulk capacitor close to the power connector.", "The bulk capacitor has a 1000 µF value."),
        ("The board has two mounting slots.", "The board has two mounting holes."),
        ("Connect the shield can to chassis ground.", "The shield can is made from 0.2 mm steel."),
        ("Use a 0.2 millimetre via drill.", "The via annular ring is 0.2 mm wide."),
        ("The controller communicates over I2C.", "The controller supports I2C addresses 0x20 through 0x27."),
        ("The protection diode is placed across the input supply.", "The protection diode has a 40 V reverse rating."),
        ("The board edge connector has 12 contacts.", "The edge connector pads are 1.2 mm wide."),
        ("The regulator's switching node is SW.", "The switch-node copper area is 10 square millimetres."),
        ("Place the crystal load capacitor close to the crystal.", "The load capacitor is 18 pF."),
        ("The board's maximum height is 8 millimetres.", "The board's maximum width is 8 centimetres."),
        ("Connect the ESD protection device next to the external port.", "The protection device clamps at 8 kilovolts."),
        ("The schematic uses hierarchical labels between sheets.", "The sheet connector uses hierarchical pins."),
    ],
}
WORD = MemoryManager._word
LEXICAL_THRESHOLD = MemoryManager.NEAR_DUPLICATE_THRESHOLD
DATASET_VERSION = "ccad-memory-dedupe-v2"


def dataset_digest() -> str:
    encoded = json.dumps(PAIRS, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def validate_dataset() -> None:
    if set(PAIRS) != {"duplicate", "distinct"}:
        raise ValueError("dataset must contain duplicate and distinct classes")
    counts = {label: len(rows) for label, rows in PAIRS.items()}
    if counts["duplicate"] != counts["distinct"] or counts["duplicate"] < 40:
        raise ValueError("dataset needs balanced classes with at least 40 labeled pairs each")
    for label, rows in PAIRS.items():
        normalized = set()
        for pair in rows:
            if len(pair) != 2 or any(not text.strip() for text in pair):
                raise ValueError(f"invalid {label} pair")
            key = tuple(text.casefold().strip() for text in pair)
            if key in normalized or key[::-1] in normalized:
                raise ValueError(f"duplicate {label} pair")
            normalized.add(key)


def lexical_match(left: str, right: str) -> bool:
    a, b = set(WORD.findall(left.casefold())), set(WORD.findall(right.casefold()))
    return (len(a) >= 5 and len(b) >= 5 and
            len(a & b) / len(a | b) >= LEXICAL_THRESHOLD)


def evaluate(base_url: str, model: str, thresholds: list[float]) -> dict:
    validate_dataset()
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
        false_positive_numbers = [index + 1 for index, (lexical, score) in
                                  enumerate(zip(lexical_matches["distinct"],
                                                scores["distinct"]))
                                  if lexical or score >= threshold]
        measurements.append({
            "threshold": threshold,
            "lexical_duplicate_hits": lexical_duplicate_hits,
            "lexical_false_positives": lexical_distinct_false_positives,
            "true_duplicates": tp,
            "false_duplicate_rejections": fp,
            "missed_duplicates": fn,
            "distinct_memories_kept": tn,
            "false_positive_pair_numbers": false_positive_numbers,
            "precision": tp / (tp + fp) if tp + fp else 1.0,
            "recall": tp / len(scores["duplicate"]),
        })
    return {
        "dataset_version": DATASET_VERSION,
        "dataset_sha256": dataset_digest(),
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
                        default=[0.91, 0.93, 0.95, 0.96, 0.97, 0.98, 0.99])
    parser.add_argument("--validate-only", action="store_true",
                        help="validate labeled dataset without contacting Ollama")
    args = parser.parse_args()
    try:
        if args.validate_only:
            validate_dataset()
            print(json.dumps({"dataset_version": DATASET_VERSION,
                              "dataset_sha256": dataset_digest(),
                              "pairs_per_class": {key: len(value) for key, value in PAIRS.items()}},
                             indent=2, sort_keys=True))
            return 0
        print(json.dumps(evaluate(args.base_url, args.model, args.threshold),
                         indent=2, sort_keys=True))
    except (RuntimeError, ValueError) as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
