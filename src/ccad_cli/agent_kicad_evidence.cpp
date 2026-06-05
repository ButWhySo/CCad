#include "agent_kicad_evidence.hpp"

#include "ccad_core/json.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ccad_cli {
namespace {

struct EvidenceOptions {
  std::string kind;
  std::string input_path;
  std::string output_path;
  std::string format;
  std::string units;
  std::string severity;
  std::string layers;
  std::string common_layers;
  std::string side;
  std::string kicad_cli;
  bool exit_code_violations = false;
  bool schematic_parity = false;
  bool refill_zones = false;
  bool save_board = false;
  bool generate_map = false;
  bool generate_report = false;
  bool execute = false;
};

struct EvidencePlan {
  EvidenceOptions options;
  std::vector<std::string> command;
  std::string artifact_kind;
  bool writes_files = true;
};

std::string jsonString(const std::string& value) {
  return "\"" + ccad::escapeJson(value) + "\"";
}

std::string jsonArray(const std::vector<std::string>& values) {
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) out << ',';
    out << jsonString(values[i]);
  }
  out << ']';
  return out.str();
}

bool truthy(const std::string& value) {
  return value == "1" || value == "true" || value == "TRUE" || value == "yes" ||
         value == "YES" || value == "on" || value == "ON";
}

std::string optionValue(const std::map<std::string, std::string>& options,
                        const std::string& dashed,
                        const std::string& plain = "") {
  const auto dashed_found = options.find(dashed);
  if (dashed_found != options.end()) return dashed_found->second;
  if (!plain.empty()) {
    const auto plain_found = options.find(plain);
    if (plain_found != options.end()) return plain_found->second;
  }
  return "";
}

bool optionFlag(const std::map<std::string, std::string>& options,
                const std::string& dashed,
                const std::string& plain = "") {
  const auto value = optionValue(options, dashed, plain);
  return truthy(value);
}

std::map<std::string, std::string> parseEvidenceArgs(const std::vector<std::string>& args,
                                                     const std::size_t start) {
  std::map<std::string, std::string> options;
  const std::vector<std::string> value_options = {"--kind",        "--input",
                                                 "--output",      "--format",
                                                 "--units",       "--severity",
                                                 "--layers",      "--common-layers",
                                                 "--side",        "--kicad-cli"};
  const std::vector<std::string> flag_options = {"--exit-code-violations",
                                                "--schematic-parity",
                                                "--refill-zones",
                                                "--save-board",
                                                "--generate-map",
                                                "--generate-report",
                                                "--execute"};
  for (std::size_t i = start; i < args.size(); ++i) {
    const std::string& key = args.at(i);
    bool is_value = false;
    for (const std::string& allowed : value_options) {
      if (key == allowed) {
        is_value = true;
        break;
      }
    }
    if (is_value) {
      if (i + 1 >= args.size()) {
        throw std::runtime_error("missing value for option: " + key);
      }
      if (options.contains(key)) {
        throw std::runtime_error("duplicate option: " + key);
      }
      options.emplace(key, args.at(++i));
      continue;
    }

    bool is_flag = false;
    for (const std::string& allowed : flag_options) {
      if (key == allowed) {
        is_flag = true;
        break;
      }
    }
    if (is_flag) {
      if (options.contains(key)) {
        throw std::runtime_error("duplicate option: " + key);
      }
      options.emplace(key, "true");
      continue;
    }

    throw std::runtime_error("unknown option: " + key);
  }
  return options;
}

EvidenceOptions normalizeOptions(const std::map<std::string, std::string>& options) {
  EvidenceOptions normalized;
  normalized.kind = optionValue(options, "--kind", "kind");
  normalized.input_path = optionValue(options, "--input", "input_path");
  normalized.output_path = optionValue(options, "--output", "output_path");
  normalized.format = optionValue(options, "--format", "format");
  normalized.units = optionValue(options, "--units", "units");
  normalized.severity = optionValue(options, "--severity", "severity");
  normalized.layers = optionValue(options, "--layers", "layers");
  normalized.common_layers = optionValue(options, "--common-layers", "common_layers");
  normalized.side = optionValue(options, "--side", "side");
  normalized.kicad_cli = optionValue(options, "--kicad-cli", "kicad_cli");
  normalized.exit_code_violations =
      optionFlag(options, "--exit-code-violations", "exit_code_violations");
  normalized.schematic_parity = optionFlag(options, "--schematic-parity", "schematic_parity");
  normalized.refill_zones = optionFlag(options, "--refill-zones", "refill_zones");
  normalized.save_board = optionFlag(options, "--save-board", "save_board");
  normalized.generate_map = optionFlag(options, "--generate-map", "generate_map");
  normalized.generate_report = optionFlag(options, "--generate-report", "generate_report");
  normalized.execute = optionFlag(options, "--execute", "execute");
  if (normalized.kind.empty()) throw std::runtime_error("missing required option: --kind");
  if (normalized.input_path.empty()) throw std::runtime_error("missing required option: --input");
  if (normalized.output_path.empty()) throw std::runtime_error("missing required option: --output");
  if (normalized.units.empty()) normalized.units = "mm";
  return normalized;
}

std::string kicadExecutable(const EvidenceOptions& options) {
  if (!options.kicad_cli.empty()) return options.kicad_cli;
  const char* env = std::getenv("CCAD_KICAD_CLI");
  if (env != nullptr && env[0] != '\0') return env;
  return "kicad-cli";
}

bool hasPathSeparator(const std::string& value) {
  return value.find('/') != std::string::npos || value.find('\\') != std::string::npos;
}

std::vector<std::string> executableCandidates(const std::filesystem::path& path) {
  std::vector<std::string> candidates{path.string()};
#ifdef _WIN32
  if (path.extension().empty()) {
    candidates.push_back((path.string() + ".exe"));
    candidates.push_back((path.string() + ".cmd"));
    candidates.push_back((path.string() + ".bat"));
  }
#endif
  return candidates;
}

bool executableExists(const std::string& executable) {
  if (executable.empty()) return false;
  if (hasPathSeparator(executable)) {
    for (const std::string& candidate : executableCandidates(executable)) {
      std::error_code error;
      if (std::filesystem::exists(candidate, error) &&
          !std::filesystem::is_directory(candidate, error)) {
        return true;
      }
    }
    return false;
  }

  const char* path_env = std::getenv("PATH");
  if (path_env == nullptr) return false;
#ifdef _WIN32
  constexpr char separator = ';';
#else
  constexpr char separator = ':';
#endif
  std::string path_list(path_env);
  std::size_t start = 0;
  while (start <= path_list.size()) {
    const std::size_t end = path_list.find(separator, start);
    const std::string item =
        path_list.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!item.empty()) {
      const std::filesystem::path base = std::filesystem::path(item) / executable;
      for (const std::string& candidate : executableCandidates(base)) {
        std::error_code error;
        if (std::filesystem::exists(candidate, error) &&
            !std::filesystem::is_directory(candidate, error)) {
          return true;
        }
      }
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return false;
}

void appendReportSeverity(std::vector<std::string>& command, const std::string& severity) {
  if (severity.empty()) return;
  if (severity == "all") {
    command.push_back("--severity-all");
  } else if (severity == "error" || severity == "errors") {
    command.push_back("--severity-error");
  } else if (severity == "warning" || severity == "warnings") {
    command.push_back("--severity-warning");
  } else if (severity == "exclusion" || severity == "exclusions") {
    command.push_back("--severity-exclusions");
  } else {
    throw std::runtime_error("unsupported KiCad severity: " + severity);
  }
}

void appendOutput(std::vector<std::string>& command, const std::string& output_path) {
  command.push_back("--output");
  command.push_back(output_path);
}

EvidencePlan buildPlan(const EvidenceOptions& options) {
  EvidencePlan plan;
  plan.options = options;
  plan.command.push_back(kicadExecutable(options));

  if (options.kind == "pcb-drc") {
    plan.artifact_kind = "pcb_drc_report";
    plan.command.insert(plan.command.end(), {"pcb", "drc"});
    appendOutput(plan.command, options.output_path);
    plan.command.push_back("--format");
    plan.command.push_back(options.format.empty() ? "json" : options.format);
    plan.command.push_back("--units");
    plan.command.push_back(options.units);
    appendReportSeverity(plan.command, options.severity);
    if (options.exit_code_violations) plan.command.push_back("--exit-code-violations");
    if (options.schematic_parity) plan.command.push_back("--schematic-parity");
    if (options.refill_zones) plan.command.push_back("--refill-zones");
    if (options.save_board) plan.command.push_back("--save-board");
  } else if (options.kind == "sch-erc") {
    plan.artifact_kind = "schematic_erc_report";
    plan.command.insert(plan.command.end(), {"sch", "erc"});
    appendOutput(plan.command, options.output_path);
    plan.command.push_back("--format");
    plan.command.push_back(options.format.empty() ? "json" : options.format);
    plan.command.push_back("--units");
    plan.command.push_back(options.units);
    appendReportSeverity(plan.command, options.severity);
    if (options.exit_code_violations) plan.command.push_back("--exit-code-violations");
  } else if (options.kind == "pcb-export-gerbers") {
    plan.artifact_kind = "gerber_output_directory";
    plan.command.insert(plan.command.end(), {"pcb", "export", "gerbers"});
    appendOutput(plan.command, options.output_path);
    if (!options.layers.empty()) {
      plan.command.push_back("--layers");
      plan.command.push_back(options.layers);
    }
    if (!options.common_layers.empty()) {
      plan.command.push_back("--common-layers");
      plan.command.push_back(options.common_layers);
    }
  } else if (options.kind == "pcb-export-drill") {
    plan.artifact_kind = "drill_output_directory";
    plan.command.insert(plan.command.end(), {"pcb", "export", "drill"});
    appendOutput(plan.command, options.output_path);
    plan.command.push_back("--format");
    plan.command.push_back(options.format.empty() ? "excellon" : options.format);
    plan.command.push_back("--excellon-units");
    plan.command.push_back(options.units == "mils" ? "in" : options.units);
    if (options.generate_map) plan.command.push_back("--generate-map");
    if (options.generate_report) plan.command.push_back("--generate-report");
  } else if (options.kind == "pcb-export-pos") {
    plan.artifact_kind = "position_file";
    plan.command.insert(plan.command.end(), {"pcb", "export", "pos"});
    appendOutput(plan.command, options.output_path);
    plan.command.push_back("--format");
    plan.command.push_back(options.format.empty() ? "csv" : options.format);
    plan.command.push_back("--units");
    plan.command.push_back(options.units == "mils" ? "in" : options.units);
    plan.command.push_back("--side");
    plan.command.push_back(options.side.empty() ? "both" : options.side);
  } else if (options.kind == "pcb-export-ipc2581") {
    plan.artifact_kind = "ipc2581_file";
    plan.command.insert(plan.command.end(), {"pcb", "export", "ipc2581"});
    appendOutput(plan.command, options.output_path);
    plan.command.push_back("--units");
    plan.command.push_back(options.units == "mils" ? "mm" : options.units);
  } else if (options.kind == "pcb-export-odb") {
    plan.artifact_kind = "odb_output";
    plan.command.insert(plan.command.end(), {"pcb", "export", "odb"});
    appendOutput(plan.command, options.output_path);
    plan.command.push_back("--units");
    plan.command.push_back(options.units == "mils" ? "mm" : options.units);
  } else {
    throw std::runtime_error("unsupported KiCad evidence kind: " + options.kind);
  }

  plan.command.push_back(options.input_path);
  return plan;
}

std::string planJson(const EvidencePlan& plan, const std::string& kind_field) {
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"" << kind_field << "\":\"ccad_agent_kicad_evidence_" << (kind_field == "plan_kind" ? "plan" : "dry_run")
      << "\",\"tool\":\"kicad-cli\","
      << "\"command_kind\":" << jsonString(plan.options.kind) << ","
      << "\"input_path\":" << jsonString(plan.options.input_path) << ","
      << "\"artifact_path\":" << jsonString(plan.options.output_path) << ","
      << "\"artifact_kind\":" << jsonString(plan.artifact_kind) << ","
      << "\"writes_files\":" << (plan.writes_files ? "true" : "false") << ","
      << "\"network_access\":false,"
      << "\"project_file_secret_storage\":false,"
      << "\"command\":" << jsonArray(plan.command) << "}";
  return out.str();
}

std::string shellQuote(const std::string& arg) {
#ifdef _WIN32
  std::string quoted = "\"";
  for (const char ch : arg) {
    if (ch == '"') quoted += "\\\"";
    else quoted += ch;
  }
  quoted += "\"";
  return quoted;
#else
  std::string quoted = "'";
  for (const char ch : arg) {
    if (ch == '\'') quoted += "'\\''";
    else quoted += ch;
  }
  quoted += "'";
  return quoted;
#endif
}

std::string commandLine(const std::vector<std::string>& command) {
  std::ostringstream out;
  for (std::size_t i = 0; i < command.size(); ++i) {
    if (i > 0) out << ' ';
    out << shellQuote(command[i]);
  }
  return out.str();
}

}  // namespace

std::string agentKiCadEvidenceSchemaJson() {
  return "{\"schema_version\":1,"
         "\"schema_kind\":\"ccad_agent_kicad_evidence_schema\","
         "\"surface\":\"headless_cli\","
         "\"tool\":\"kicad-cli\","
         "\"tool_reference\":\"official_kicad_cli\","
         "\"execution_default\":\"dry_run_or_plan_only\","
         "\"network_access\":false,"
         "\"project_file_secret_storage\":false,"
         "\"supported_kinds\":[\"pcb-drc\",\"sch-erc\",\"pcb-export-gerbers\","
         "\"pcb-export-drill\",\"pcb-export-pos\",\"pcb-export-ipc2581\","
         "\"pcb-export-odb\"],"
         "\"common_fields\":[\"kind\",\"input_path\",\"output_path\",\"kicad_cli\"],"
         "\"report_fields\":[\"format\",\"units\",\"severity\",\"exit_code_violations\"],"
         "\"export_fields\":[\"layers\",\"common_layers\",\"side\",\"generate_map\","
         "\"generate_report\"],"
         "\"methods\":[\"agent.kicad_evidence_plan\","
         "\"agent.kicad_evidence_dry_run\",\"agent.kicad_evidence_run\"],"
         "\"notes\":[\"Use plan before run\",\"Use dry-run to check executable presence\","
         "\"Run requires explicit execute flag and policy approval\"]}";
}

std::string agentKiCadEvidencePlanJson(const std::map<std::string, std::string>& options) {
  return planJson(buildPlan(normalizeOptions(options)), "plan_kind");
}

std::string agentKiCadEvidencePlanJson(const std::vector<std::string>& args,
                                       const std::size_t start) {
  return agentKiCadEvidencePlanJson(parseEvidenceArgs(args, start));
}

std::string agentKiCadEvidenceDryRunJson(const std::map<std::string, std::string>& options) {
  const EvidencePlan plan = buildPlan(normalizeOptions(options));
  const std::string executable = plan.command.empty() ? "" : plan.command.front();
  const bool configured = options.contains("--kicad-cli") || options.contains("kicad_cli") ||
                          std::getenv("CCAD_KICAD_CLI") != nullptr;
  const bool found = executableExists(executable);
  const bool input_found = std::filesystem::exists(plan.options.input_path);
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"dry_run_kind\":\"ccad_agent_kicad_evidence_dry_run\","
      << "\"tool\":\"kicad-cli\","
      << "\"command_kind\":" << jsonString(plan.options.kind) << ","
      << "\"would_execute\":false,"
      << "\"executable_configured\":" << (configured ? "true" : "false") << ","
      << "\"executable_found\":" << (found ? "true" : "false") << ","
      << "\"input_found\":" << (input_found ? "true" : "false") << ","
      << "\"ready_to_execute\":" << (found && input_found ? "true" : "false") << ","
      << "\"network_access\":false,"
      << "\"project_file_secret_storage\":false,"
      << "\"artifact_path\":" << jsonString(plan.options.output_path) << ","
      << "\"command\":" << jsonArray(plan.command) << "}";
  return out.str();
}

std::string agentKiCadEvidenceDryRunJson(const std::vector<std::string>& args,
                                         const std::size_t start) {
  return agentKiCadEvidenceDryRunJson(parseEvidenceArgs(args, start));
}

std::string agentKiCadEvidenceRunJson(const std::map<std::string, std::string>& options) {
  const EvidenceOptions normalized = normalizeOptions(options);
  const EvidencePlan plan = buildPlan(normalized);
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"run_kind\":\"ccad_agent_kicad_evidence_run\","
      << "\"tool\":\"kicad-cli\","
      << "\"command_kind\":" << jsonString(normalized.kind) << ","
      << "\"artifact_path\":" << jsonString(normalized.output_path) << ","
      << "\"command\":" << jsonArray(plan.command) << ",";
  if (!normalized.execute) {
    out << "\"executed\":false,\"reason\":\"execute_flag_required\","
        << "\"network_access\":false,\"project_file_secret_storage\":false}";
    return out.str();
  }
  if (!executableExists(plan.command.front())) {
    out << "\"executed\":false,\"reason\":\"executable_not_found\","
        << "\"network_access\":false,\"project_file_secret_storage\":false}";
    return out.str();
  }
  const std::string line = commandLine(plan.command);
  const int exit_code = std::system(line.c_str());
  out << "\"executed\":true,"
      << "\"exit_code\":" << exit_code << ","
      << "\"command_line\":" << jsonString(line) << ","
      << "\"network_access\":false,\"project_file_secret_storage\":false}";
  return out.str();
}

std::string agentKiCadEvidenceRunJson(const std::vector<std::string>& args,
                                      const std::size_t start) {
  return agentKiCadEvidenceRunJson(parseEvidenceArgs(args, start));
}

}  // namespace ccad_cli
