#include "ccad_core/transaction.hpp"

#include "ccad_core/json.hpp"

#include <sstream>

namespace ccad {

Transaction buildTransaction(const std::string& id, const std::string& command,
                             const std::string& summary, const Project& before,
                             const Project& after) {
  return Transaction{
      .id = id,
      .command = command,
      .summary = summary,
      .before_project_id = before.id,
      .after_project_id = after.id,
      .diff = diffProjects(before, after),
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
  out << "}\n";
  return out.str();
}

}  // namespace ccad

