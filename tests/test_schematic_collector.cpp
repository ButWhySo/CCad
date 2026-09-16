#include "ccad_core/schematic_collector.hpp"

#include <iostream>
#include <string>

using namespace ccad;

static void assertEqual(int actual, int expected, const std::string& message) {
  if (actual != expected) {
    std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
    exit(1);
  }
}

static void testSchematicCollector() {
  Schematic sch;
  sch.id = "sch1";
  
  SchSymbol sym;
  sym.id = "U1";
  sch.symbols.push_back(sym);

  SchWire wire;
  wire.id = "W1";
  sch.wires.push_back(wire);

  SchLabel label;
  label.id = "L1";
  label.type = LabelType::Local;
  sch.labels.push_back(label);
  
  SchLabel glabel;
  glabel.id = "GL1";
  glabel.type = LabelType::Global;
  sch.labels.push_back(glabel);

  SchLabel directive;
  directive.id = "DL1";
  directive.type = LabelType::Directive;
  sch.labels.push_back(directive);

  SchPowerSymbol pwr;
  pwr.id = "P1";
  sch.power_symbols.push_back(pwr);

  SchTextBox txt;
  txt.id = "T1";
  sch.textboxes.push_back(txt);

  SchematicCollectorGuide guide;
  SchematicCollectorReport report = collectSchematicItems(sch, SchematicCollectorScanSet::all_items, guide);
  
  assertEqual(static_cast<int>(report.candidates.size()), 7, "all_items should find 7 components");

  report = collectSchematicItems(sch, SchematicCollectorScanSet::editable_items, guide);
  // editable_items misses power symbols maybe? No, power_symbols map to SCH_SYMBOL_T which is editable
  assertEqual(static_cast<int>(report.candidates.size()), 7, "editable_items should find 7 components");

  report = collectSchematicItems(sch, SchematicCollectorScanSet::field_owners, guide);
  // field_owners maps to SCH_SYMBOL_T (U1 and P1) and SCH_SHEET_T and SCH_LABEL_LOCATE_ANY_T. Wait, SCH_LABEL_LOCATE_ANY_T is not implemented yet in the type map for field_owners in SchematicCollector.
  // We mapped field_owners to SCH_SYMBOL_T, SCH_SHEET_T, SCH_LABEL_LOCATE_ANY_T
  assertEqual(static_cast<int>(report.candidates.size()), 2, "field_owners should find 2 components");
}

int main() {
  testSchematicCollector();
  std::cout << "OK\n";
  return 0;
}
