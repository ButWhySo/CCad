#pragma once

#include "ccad_core/diff.hpp"
#include "ccad_core/model.hpp"

#include <string>

namespace ccad {

struct Transaction {
  std::string id;
  std::string command;
  std::string summary;
  std::string before_project_id;
  std::string after_project_id;
  ProjectDiff diff;
};

Transaction buildTransaction(const std::string& id, const std::string& command,
                             const std::string& summary, const Project& before,
                             const Project& after);

std::string dumpTransactionJson(const Transaction& transaction);
std::string dumpProjectDiffJson(const ProjectDiff& diff);

}  // namespace ccad

