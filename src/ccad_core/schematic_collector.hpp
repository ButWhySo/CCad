#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace ccad {

enum class SchematicCollectorScanSet {
  all_items,
  editable_items,
  movable_items,
  field_owners,
  deletable_items,
};

struct SchematicCollectorGuide {
  bool include_secondary = true;
};

struct SchematicCollectorCandidate {
  std::string type;
  std::string id;
  std::string kicad_type;
  std::string collection_bucket;
  std::string net_id;
};

struct SchematicCollectorReport {
  std::string kicad_collector = "SCH_COLLECTOR";
  std::string parity_scope = "collector_scan_set_first_slice";
  std::string scan_set;
  std::vector<std::string> kicad_scan_types;
  std::vector<std::string> unsupported_kicad_types;
  std::vector<SchematicCollectorCandidate> candidates;
  int primary_count = 0;
  int secondary_count = 0;
};

std::vector<std::string> schematicCollectorScanTypes(SchematicCollectorScanSet scan_set);
SchematicCollectorScanSet parseSchematicCollectorScanSet(std::string_view name);
std::string schematicCollectorScanSetName(SchematicCollectorScanSet scan_set);
SchematicCollectorReport collectSchematicItems(const Schematic& sch,
                                               SchematicCollectorScanSet scan_set,
                                               const SchematicCollectorGuide& guide);

}  // namespace ccad
