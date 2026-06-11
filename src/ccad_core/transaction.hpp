#pragma once

#include "ccad_core/diff.hpp"
#include "ccad_core/model.hpp"

#include <string>

namespace ccad {

struct CommitImpact {
  bool board_modified = false;
  bool schematic_modified = false;
  bool view_dirty = false;
  bool drc_dirty = false;
  bool erc_dirty = false;
  bool connectivity_dirty = false;
  bool ratsnest_dirty = false;
  bool board_outline_dirty = false;
  bool solder_mask_dirty = false;
  std::vector<std::string> dirty_object_ids;
  std::vector<std::string> dirty_object_types;
};

struct Transaction {
  std::string id;
  std::string command;
  std::string summary;
  std::string before_project_id;
  std::string after_project_id;
  ProjectDiff diff;
  CommitImpact impact;
};

Transaction buildTransaction(const std::string& id, const std::string& command,
                             const std::string& summary, const Project& before,
                             const Project& after);

std::string dumpTransactionJson(const Transaction& transaction);
std::string dumpProjectDiffJson(const ProjectDiff& diff);

}  // namespace ccad

