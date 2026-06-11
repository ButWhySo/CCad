#include "ccad_core/transaction.hpp"

#include "ccad_core/json.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>

namespace ccad {
namespace {

bool isBoardObjectType(const std::string& type) {
  return type == "board_outline" || type == "design_rules" || type == "layer" ||
         type == "placement_region" || type == "keepout" || type == "pad" ||
         type == "via" || type == "track" || type == "graphic" || type == "text" ||
         type == "zone" || type == "route_request";
}

bool isSchematicObjectType(const std::string& type) {
  return type == "component" || type == "net" || type == "constraint";
}

bool dirtiesConnectivity(const std::string& type) {
  return type == "pad" || type == "via" || type == "track" || type == "zone" ||
         type == "layer" || type == "board_outline";
}

bool dirtiesSolderMask(const std::string& type) {
  return type == "pad" || type == "via";
}

void pushUnique(std::vector<std::string>& values, const std::string& value) {
  if (std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(value);
  }
}

CommitImpact buildCommitImpact(const ProjectDiff& diff) {
  CommitImpact impact;
  for (const DiffEntry& entry : diff.entries) {
    pushUnique(impact.dirty_object_ids, entry.object_id);
    pushUnique(impact.dirty_object_types, entry.object_type);

    if (isBoardObjectType(entry.object_type)) {
      impact.board_modified = true;
      impact.view_dirty = true;
      impact.drc_dirty = true;
    }

    if (isSchematicObjectType(entry.object_type)) {
      impact.schematic_modified = true;
      impact.erc_dirty = true;
    }

    if (dirtiesConnectivity(entry.object_type)) {
      impact.connectivity_dirty = true;
      impact.ratsnest_dirty = true;
    }

    if (entry.object_type == "board_outline") {
      impact.board_outline_dirty = true;
    }

    if (dirtiesSolderMask(entry.object_type)) {
      impact.solder_mask_dirty = true;
    }
  }
  return impact;
}

void dumpStringArray(std::ostringstream& out, const std::vector<std::string>& values) {
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out << "\"" << escapeJson(values.at(i)) << "\"";
    if (i + 1 < values.size()) {
      out << ", ";
    }
  }
  out << "]";
}

void dumpBoolField(std::ostringstream& out, std::string_view name, const bool value) {
  out << "    \"" << name << "\": " << (value ? "true" : "false") << ",\n";
}

std::string dumpCommitImpactJson(const CommitImpact& impact) {
  std::ostringstream out;
  out << "{\n";
  dumpBoolField(out, "board_modified", impact.board_modified);
  dumpBoolField(out, "schematic_modified", impact.schematic_modified);
  dumpBoolField(out, "view_dirty", impact.view_dirty);
  dumpBoolField(out, "drc_dirty", impact.drc_dirty);
  dumpBoolField(out, "erc_dirty", impact.erc_dirty);
  dumpBoolField(out, "connectivity_dirty", impact.connectivity_dirty);
  dumpBoolField(out, "ratsnest_dirty", impact.ratsnest_dirty);
  dumpBoolField(out, "board_outline_dirty", impact.board_outline_dirty);
  dumpBoolField(out, "solder_mask_dirty", impact.solder_mask_dirty);
  out << "    \"dirty_object_ids\": ";
  dumpStringArray(out, impact.dirty_object_ids);
  out << ",\n";
  out << "    \"dirty_object_types\": ";
  dumpStringArray(out, impact.dirty_object_types);
  out << "\n";
  out << "  }";
  return out.str();
}

}  // namespace

Transaction buildTransaction(const std::string& id, const std::string& command,
                             const std::string& summary, const Project& before,
                             const Project& after) {
  const ProjectDiff diff = diffProjects(before, after);
  return Transaction{
      .id = id,
      .command = command,
      .summary = summary,
      .before_project_id = before.id,
      .after_project_id = after.id,
      .diff = diff,
      .impact = buildCommitImpact(diff),
  };
}

std::string dumpProjectDiffJson(const ProjectDiff& diff) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"summary\": {\n";
  out << "    \"added\": " << diff.added_count << ",\n";
  out << "    \"changed\": " << diff.changed_count << ",\n";
  out << "    \"removed\": " << diff.removed_count << "\n";
  out << "  },\n";
  out << "  \"entries\": [\n";
  for (std::size_t i = 0; i < diff.entries.size(); ++i) {
    const DiffEntry& entry = diff.entries.at(i);
    out << "    {\n";
    out << "      \"change\": \"" << escapeJson(entry.change) << "\",\n";
    out << "      \"message\": \"" << escapeJson(entry.message) << "\",\n";
    out << "      \"object_id\": \"" << escapeJson(entry.object_id) << "\",\n";
    out << "      \"object_type\": \"" << escapeJson(entry.object_type) << "\"\n";
    out << "    }" << (i + 1 == diff.entries.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::string dumpTransactionJson(const Transaction& transaction) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"id\": \"" << escapeJson(transaction.id) << "\",\n";
  out << "  \"command\": \"" << escapeJson(transaction.command) << "\",\n";
  out << "  \"summary\": \"" << escapeJson(transaction.summary) << "\",\n";
  out << "  \"before_project_id\": \"" << escapeJson(transaction.before_project_id) << "\",\n";
  out << "  \"after_project_id\": \"" << escapeJson(transaction.after_project_id) << "\",\n";

  const std::string diff_json = dumpProjectDiffJson(transaction.diff);
  out << "  \"diff\": ";
  for (std::size_t i = 0; i < diff_json.size(); ++i) {
    out << diff_json.at(i);
    if (diff_json.at(i) == '\n' && i + 1 < diff_json.size()) {
      out << "  ";
    }
  }
  out << ",\n";

  const std::string impact_json = dumpCommitImpactJson(transaction.impact);
  out << "  \"impact\": ";
  for (std::size_t i = 0; i < impact_json.size(); ++i) {
    out << impact_json.at(i);
    if (impact_json.at(i) == '\n' && i + 1 < impact_json.size()) {
      out << "  ";
    }
  }
  out << "}\n";
  return out.str();
}

}  // namespace ccad

