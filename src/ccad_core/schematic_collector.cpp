#include "ccad_core/schematic_collector.hpp"

#include <algorithm>
#include <stdexcept>

namespace ccad {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

bool typeSupportedInCurrentModel(const std::string& kicad_type) {
  return kicad_type == "SCH_SYMBOL_T" || kicad_type == "SCH_LINE_T" ||
         kicad_type == "SCH_LABEL_T" || kicad_type == "SCH_GLOBAL_LABEL_T" ||
         kicad_type == "SCH_HIER_LABEL_T" || kicad_type == "SCH_DIRECTIVE_LABEL_T" ||
         kicad_type == "SCH_TEXT_T" || kicad_type == "SCH_TEXTBOX_T" ||
         kicad_type == "SCH_SHAPE_T" || kicad_type == "SCH_FIELD_T" ||
         kicad_type == "SCH_SHEET_T" || kicad_type == "SCH_SHEET_PIN_T" ||
         kicad_type == "SCH_BITMAP_T" || kicad_type == "SCH_BUS_WIRE_ENTRY_T" ||
         kicad_type == "SCH_BUS_BUS_ENTRY_T" || kicad_type == "SCH_RULE_AREA_T" ||
         kicad_type == "SCH_TABLE_T" || kicad_type == "SCH_TABLECELL_T" ||
         kicad_type == "SCH_MARKER_T" || kicad_type == "SCH_JUNCTION_T" ||
         kicad_type == "SCH_NO_CONNECT_T" || kicad_type == "SCH_GROUP_T";
}

void appendUnsupportedTypes(SchematicCollectorReport& report) {
  for (const std::string& kicad_type : report.kicad_scan_types) {
    if (!typeSupportedInCurrentModel(kicad_type) &&
        !contains(report.unsupported_kicad_types, kicad_type)) {
      report.unsupported_kicad_types.push_back(kicad_type);
    }
  }
}

void maybeAppendCandidate(std::vector<SchematicCollectorCandidate>& primary,
                          std::vector<SchematicCollectorCandidate>& /*secondary*/,
                          const SchematicCollectorCandidate& candidate,
                          const SchematicCollectorGuide& /*guide*/) {
  // Schematic collector doesn't have layers currently.
  // We place everything in primary bucket by default.
  SchematicCollectorCandidate row = candidate;
  row.collection_bucket = "primary";
  primary.push_back(row);
}

SchematicCollectorCandidate makeCandidate(const std::string& type,
                                          const std::string& id,
                                          const std::string& kicad_type,
                                          const std::string& net_id = "") {
  SchematicCollectorCandidate candidate;
  candidate.type = type;
  candidate.id = id;
  candidate.kicad_type = kicad_type;
  candidate.net_id = net_id;
  return candidate;
}

void collectType(const Schematic& sch,
                 const std::string& kicad_type,
                 const SchematicCollectorGuide& guide,
                 std::vector<SchematicCollectorCandidate>& primary,
                 std::vector<SchematicCollectorCandidate>& secondary) {
  if (kicad_type == "SCH_SYMBOL_T") {
    for (const SchSymbol& symbol : sch.symbols) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("symbol", symbol.id, kicad_type),
                           guide);
    }
    for (const SchPowerSymbol& psym : sch.power_symbols) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("power_symbol", psym.id, kicad_type, psym.net_id),
                           guide);
    }
  } else if (kicad_type == "SCH_LINE_T") {
    for (const SchWire& wire : sch.wires) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("wire", wire.id, kicad_type, wire.net_id),
                           guide);
    }
    for (const SchBus& bus : sch.buses) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("bus", bus.id, kicad_type, bus.bus_id),
                           guide);
    }
  } else if (kicad_type == "SCH_LABEL_T" || kicad_type == "SCH_GLOBAL_LABEL_T" ||
             kicad_type == "SCH_HIER_LABEL_T" || kicad_type == "SCH_DIRECTIVE_LABEL_T") {
    for (const SchLabel& label : sch.labels) {
      bool match = false;
      if (kicad_type == "SCH_LABEL_T" && label.type == LabelType::Local) match = true;
      if (kicad_type == "SCH_GLOBAL_LABEL_T" && label.type == LabelType::Global) match = true;
      if (kicad_type == "SCH_HIER_LABEL_T" && label.type == LabelType::Hierarchical) match = true;
      // SCH_DIRECTIVE_LABEL_T is not implemented in LabelType yet
      if (match) {
        maybeAppendCandidate(primary, secondary,
                             makeCandidate("label", label.id, kicad_type, label.net_id),
                             guide);
      }
    }
  } else if (kicad_type == "SCH_TEXTBOX_T") {
    for (const SchTextBox& textbox : sch.textboxes) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("textbox", textbox.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_SHAPE_T") {
    for (const SchGraphic& graphic : sch.graphics) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("graphic", graphic.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_SHEET_T") {
    for (const SchSheet& sheet : sch.sheets) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("sheet", sheet.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_SHEET_PIN_T") {
    for (const SchSheet& sheet : sch.sheets) {
      for (const SchSheetPin& pin : sheet.pins) {
        maybeAppendCandidate(primary, secondary,
                             makeCandidate("sheet_pin", pin.name, kicad_type),
                             guide);
      }
    }
  } else if (kicad_type == "SCH_BITMAP_T") {
    for (const SchBitmap& bitmap : sch.bitmaps) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("bitmap", bitmap.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_BUS_WIRE_ENTRY_T" || kicad_type == "SCH_BUS_BUS_ENTRY_T") {
    for (const SchBusEntry& entry : sch.bus_entries) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("bus_entry", entry.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_RULE_AREA_T") {
    for (const SchRuleArea& area : sch.rule_areas) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("rule_area", area.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_TABLE_T") {
    // Not implemented
  } else if (kicad_type == "SCH_MARKER_T") {
    for (const SchMarker& marker : sch.markers) {
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("marker", marker.id, kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_JUNCTION_T") {
    for (const SchJunction& junc : sch.junctions) {
      (void)junc;
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("junction", "", kicad_type),
                           guide);
    }
  } else if (kicad_type == "SCH_NO_CONNECT_T") {
    for (const SchNoConnect& nc : sch.no_connects) {
      (void)nc;
      maybeAppendCandidate(primary, secondary,
                           makeCandidate("no_connect", "", kicad_type),
                           guide);
    }
  }
}

}  // namespace

std::vector<std::string> schematicCollectorScanTypes(const SchematicCollectorScanSet scan_set) {
  switch (scan_set) {
    case SchematicCollectorScanSet::all_items:
      return {"SCH_SHAPE_T", "SCH_TEXT_T", "SCH_TEXTBOX_T", "SCH_TABLECELL_T",
              "SCH_LABEL_T", "SCH_GLOBAL_LABEL_T", "SCH_HIER_LABEL_T", "SCH_DIRECTIVE_LABEL_T",
              "SCH_FIELD_T", "SCH_SYMBOL_T", "SCH_SHEET_PIN_T", "SCH_SHEET_T",
              "SCH_BITMAP_T", "SCH_LINE_T", "SCH_BUS_WIRE_ENTRY_T", "SCH_BUS_BUS_ENTRY_T",
              "SCH_JUNCTION_T", "SCH_RULE_AREA_T", "SCH_TABLE_T", "SCH_GROUP_T",
              "SCH_MARKER_T", "SCH_NO_CONNECT_T"};
    case SchematicCollectorScanSet::editable_items:
      return {"SCH_SHAPE_T", "SCH_TEXT_T", "SCH_TEXTBOX_T", "SCH_TABLECELL_T",
              "SCH_LABEL_T", "SCH_GLOBAL_LABEL_T", "SCH_HIER_LABEL_T", "SCH_DIRECTIVE_LABEL_T",
              "SCH_FIELD_T", "SCH_SYMBOL_T", "SCH_SHEET_PIN_T", "SCH_SHEET_T",
              "SCH_BITMAP_T", "SCH_LINE_T", "SCH_BUS_WIRE_ENTRY_T", "SCH_JUNCTION_T",
              "SCH_RULE_AREA_T", "SCH_GROUP_T"};
    case SchematicCollectorScanSet::movable_items:
      return {"SCH_MARKER_T", "SCH_JUNCTION_T", "SCH_NO_CONNECT_T", "SCH_BUS_BUS_ENTRY_T",
              "SCH_BUS_WIRE_ENTRY_T", "SCH_LINE_T", "SCH_BITMAP_T", "SCH_SHAPE_T",
              "SCH_TEXT_T", "SCH_TEXTBOX_T", "SCH_TABLE_T", "SCH_TABLECELL_T",
              "SCH_LABEL_T", "SCH_GLOBAL_LABEL_T", "SCH_HIER_LABEL_T", "SCH_DIRECTIVE_LABEL_T",
              "SCH_FIELD_T", "SCH_SYMBOL_T", "SCH_SHEET_PIN_T", "SCH_SHEET_T",
              "SCH_RULE_AREA_T", "SCH_GROUP_T"};
    case SchematicCollectorScanSet::field_owners:
      return {"SCH_SYMBOL_T", "SCH_SHEET_T", "SCH_LABEL_LOCATE_ANY_T"};
    case SchematicCollectorScanSet::deletable_items:
      return {"LIB_SYMBOL_T", "SCH_MARKER_T", "SCH_JUNCTION_T", "SCH_LINE_T",
              "SCH_BUS_BUS_ENTRY_T", "SCH_BUS_WIRE_ENTRY_T", "SCH_SHAPE_T",
              "SCH_RULE_AREA_T", "SCH_TEXT_T", "SCH_TEXTBOX_T", "SCH_TABLECELL_T",
              "SCH_TABLE_T", "SCH_LABEL_T", "SCH_GLOBAL_LABEL_T", "SCH_HIER_LABEL_T",
              "SCH_DIRECTIVE_LABEL_T", "SCH_NO_CONNECT_T", "SCH_SHEET_T",
              "SCH_SHEET_PIN_T", "SCH_SYMBOL_T", "SCH_FIELD_T", "SCH_BITMAP_T",
              "SCH_GROUP_T"};
  }
  return {};
}

SchematicCollectorScanSet parseSchematicCollectorScanSet(const std::string_view name) {
  if (name == "all_items") return SchematicCollectorScanSet::all_items;
  if (name == "editable_items") return SchematicCollectorScanSet::editable_items;
  if (name == "movable_items") return SchematicCollectorScanSet::movable_items;
  if (name == "field_owners") return SchematicCollectorScanSet::field_owners;
  if (name == "deletable_items") return SchematicCollectorScanSet::deletable_items;
  throw std::invalid_argument("unknown schematic collector scan set");
}

std::string schematicCollectorScanSetName(const SchematicCollectorScanSet scan_set) {
  switch (scan_set) {
    case SchematicCollectorScanSet::all_items: return "all_items";
    case SchematicCollectorScanSet::editable_items: return "editable_items";
    case SchematicCollectorScanSet::movable_items: return "movable_items";
    case SchematicCollectorScanSet::field_owners: return "field_owners";
    case SchematicCollectorScanSet::deletable_items: return "deletable_items";
  }
  return "";
}

SchematicCollectorReport collectSchematicItems(const Schematic& sch,
                                               const SchematicCollectorScanSet scan_set,
                                               const SchematicCollectorGuide& guide) {
  SchematicCollectorReport report;
  report.scan_set = schematicCollectorScanSetName(scan_set);
  report.kicad_scan_types = schematicCollectorScanTypes(scan_set);
  appendUnsupportedTypes(report);

  std::vector<SchematicCollectorCandidate> primary;
  std::vector<SchematicCollectorCandidate> secondary;
  for (const std::string& kicad_type : report.kicad_scan_types) {
    collectType(sch, kicad_type, guide, primary, secondary);
  }

  report.primary_count = static_cast<int>(primary.size());
  report.secondary_count = static_cast<int>(secondary.size());
  report.candidates.reserve(primary.size() + secondary.size());
  report.candidates.insert(report.candidates.end(), primary.begin(), primary.end());
  report.candidates.insert(report.candidates.end(), secondary.begin(), secondary.end());
  return report;
}

}  // namespace ccad
