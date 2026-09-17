#include "agent_orchestrator_cli.hpp"
#include "common.hpp"
#include "ccad_core/agent_orchestrator.hpp"
#include "ccad_core/json.hpp"
#include "app.hpp"

#include <iostream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <memory>
#include <vector>
#include <initializer_list>

namespace ccad_cli {

namespace {

std::string extractStringValue(const std::string& json, const std::string& key) {
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos) return "";
  pos += search.length();
  while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
  if (pos < json.length() && json[pos] == '"') {
    pos++;
    std::string value;
    bool escaped = false;
    for (; pos < json.length(); ++pos) {
      const char c = json[pos];
      if (escaped) {
        if (c == 'n') value += '\n';
        else if (c == 'r') value += '\r';
        else if (c == 't') value += '\t';
        else if (c == '"' || c == '\\' || c == '/') value += c;
        else { value += '\\'; value += c; }
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        return value;
      } else {
        value += c;
      }
    }
  }
  return "";
}

std::string formatError(const std::string& id, int code, const std::string& message) {
  std::ostringstream out;
  out << "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": " << code << ", \"message\": \"" << ccad::escapeJson(message) << "\"}";
  if (!id.empty()) {
    out << ", \"id\": " << id;
  } else {
    out << ", \"id\": null";
  }
  out << "}";
  return out.str();
}

std::string formatSuccess(const std::string& id, const std::string& resultJson) {
  std::ostringstream out;
  out << "{\"jsonrpc\": \"2.0\", \"result\": " << resultJson << ", \"id\": " << id << "}";
  return out.str();
}

std::string optionOrEmpty(const std::map<std::string, std::string>& options,
                          const std::string& key) {
  const auto found = options.find(key);
  if (found == options.end()) {
    return "";
  }
  return found->second;
}

std::string executeCliTool(const std::string& tool, const std::string& json) {
    std::vector<std::string> command{"ccad"};
    const auto add = [&](const std::string& value) { command.push_back(value); };
    const auto required = [&](const std::string& key) { return extractStringValue(json, key); };
    const auto appendOption = [&](const std::string& option, const std::string& key) {
        const std::string value = required(key);
        if (!value.empty()) { add(option); add(value); }
    };
    const auto requireFields = [&](std::initializer_list<const char*> fields) -> std::string {
        for (const char* field : fields) {
            if (required(field).empty()) {
                return std::string("{\"error\":\"missing_parameter\",\"parameter\":\"") +
                       ccad::escapeJson(field) + "\"}";
            }
        }
        return {};
    };

    if (tool == "pcb.add-via") {
        if (const std::string error = requireFields({"file", "id", "net", "x_mm", "y_mm", "diameter_mm", "drill_mm"}); !error.empty()) return error;
        add("pcb"); add("add-via");
        appendOption("--file", "file"); appendOption("--id", "id"); appendOption("--net", "net");
        appendOption("--x-mm", "x_mm"); appendOption("--y-mm", "y_mm");
        appendOption("--diameter-mm", "diameter_mm"); appendOption("--drill-mm", "drill_mm");
    } else if (tool == "pcb.add-track") {
        if (const std::string error = requireFields({"file", "id", "net", "layer", "start_x_mm", "start_y_mm", "end_x_mm", "end_y_mm", "width_mm"}); !error.empty()) return error;
        add("pcb"); add("add-track");
        appendOption("--file", "file"); appendOption("--id", "id"); appendOption("--net", "net");
        appendOption("--layer", "layer"); appendOption("--start-x-mm", "start_x_mm");
        appendOption("--start-y-mm", "start_y_mm"); appendOption("--end-x-mm", "end_x_mm");
        appendOption("--end-y-mm", "end_y_mm"); appendOption("--width-mm", "width_mm");
    } else if (tool == "pcb.place-footprint") {
        if (const std::string error = requireFields({"file", "footprint", "component", "at_x_mm", "at_y_mm", "layer"}); !error.empty()) return error;
        add("pcb"); add("place-footprint");
        appendOption("--file", "file"); appendOption("--footprint", "footprint");
        appendOption("--component", "component"); appendOption("--at-x-mm", "at_x_mm");
        appendOption("--at-y-mm", "at_y_mm"); appendOption("--layer", "layer");
        appendOption("--rotation-deg", "rotation_deg");
    } else if (tool == "pcb.export") {
        if (const std::string error = requireFields({"file", "output"}); !error.empty()) return error;
        add("pcb"); add("export-kicad");
        appendOption("--file", "file"); appendOption("--output", "output");
    } else if (tool == "pcb.drc") {
        if (const std::string error = requireFields({"file"}); !error.empty()) return error;
        add("drc"); add(required("file"));
    } else if (tool == "pcb.add-zone") {
        if (const std::string error = requireFields({"file", "id", "layers", "x_mm", "y_mm", "width_mm", "height_mm"}); !error.empty()) return error;
        add("pcb"); add("add-zone");
        appendOption("--file", "file"); appendOption("--id", "id"); appendOption("--layers", "layers");
        appendOption("--x-mm", "x_mm"); appendOption("--y-mm", "y_mm");
        appendOption("--width-mm", "width_mm"); appendOption("--height-mm", "height_mm");
        appendOption("--name", "name"); appendOption("--net", "net"); appendOption("--priority", "priority");
        appendOption("--clearance-mm", "clearance_mm"); appendOption("--min-thickness-mm", "min_thickness_mm");
        appendOption("--pad-connection", "pad_connection");
    } else if (tool == "pcb.add-keepout") {
        if (const std::string error = requireFields({"file", "id", "kind", "x_mm", "y_mm", "width_mm", "height_mm"}); !error.empty()) return error;
        add("pcb"); add("add-keepout");
        appendOption("--file", "file"); appendOption("--id", "id"); appendOption("--kind", "kind");
        appendOption("--x-mm", "x_mm"); appendOption("--y-mm", "y_mm");
        appendOption("--width-mm", "width_mm"); appendOption("--height-mm", "height_mm");
    } else {
        return "{\"error\":\"tool_adapter_unavailable\"}";
    }

    std::vector<char*> argv;
    argv.reserve(command.size());
    for (std::string& value : command) argv.push_back(value.data());
    std::ostringstream stdout_capture;
    std::ostringstream stderr_capture;
    std::streambuf* old_stdout = std::cout.rdbuf(stdout_capture.rdbuf());
    std::streambuf* old_stderr = std::cerr.rdbuf(stderr_capture.rdbuf());
    const int exit_code = run(static_cast<int>(argv.size()), argv.data());
    std::cout.rdbuf(old_stdout);
    std::cerr.rdbuf(old_stderr);
    return "{\"exit_code\":" + std::to_string(exit_code) +
           ",\"stdout\":\"" + ccad::escapeJson(stdout_capture.str()) +
           "\",\"stderr\":\"" + ccad::escapeJson(stderr_capture.str()) + "\"}";
}

// Build a context from project file
ccad::ProjectContext buildContext(const std::string& project_path) {
    ccad::ProjectContext context;
    if (!project_path.empty()) {
        try {
            ccad::Project proj = loadProjectFile(project_path);
            context.project_file = project_path;
            context.has_board = !proj.boards.empty();
            if (const ccad::Schematic* schematic = ccad::primarySchematic(proj)) {
                context.has_schematic = !schematic->symbols.empty();
                context.component_count = schematic->symbols.size();
            }
            if (context.has_board) {
                const auto& b = proj.boards[0];
                context.pad_count = b.pads.size();
                context.track_count = b.tracks.size();
                context.via_count = b.vias.size();
                context.zone_count = b.zones.size();
            }
        } catch (...) {
            // Ignore load errors for context building
        }
    }
    return context;
}

// A global orchestrator instance for the CLI process (if maintaining state)
std::unique_ptr<ccad::AgentOrchestrator> g_orchestrator;

ccad::AgentOrchestrator& getOrchestrator() {
    if (!g_orchestrator) {
        g_orchestrator = std::make_unique<ccad::AgentOrchestrator>();
        
        // Register read-only tools backed by the current project snapshot.
        // Mutation tools remain explicit adapters until their argument schema is
        // wired to ccad_cli::run() with captured stdout/stderr.
        g_orchestrator->register_tool({
            "project.review", "Review project", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string& args) -> std::string {
                return buildContext(extractStringValue(args, "project_path")).to_json();
            }
        });
        g_orchestrator->register_tool({
            "pcb.drc", "Run DRC", ccad::TaskRisk::ReadOnly,
            R"({"type":"object","required":["file"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.drc", args); }
        });
        g_orchestrator->register_tool({
            "pcb.add-zone", "Add Zone", ccad::TaskRisk::LowMutation,
            R"({"type":"object","required":["file","id","layers","x_mm","y_mm","width_mm","height_mm"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.add-zone", args); }
        });
        g_orchestrator->register_tool({
            "pcb.add-keepout", "Add Keepout", ccad::TaskRisk::LowMutation,
            R"({"type":"object","required":["file","id","kind","x_mm","y_mm","width_mm","height_mm"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.add-keepout", args); }
        });
        g_orchestrator->register_tool({
            "pcb.add-via", "Add Via", ccad::TaskRisk::LowMutation,
            R"({"type":"object","required":["file","id","net","x_mm","y_mm","diameter_mm","drill_mm"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.add-via", args); }
        });
        g_orchestrator->register_tool({
            "pcb.add-track", "Add Track", ccad::TaskRisk::LowMutation,
            R"({"type":"object","required":["file","id","net","layer","start_x_mm","start_y_mm","end_x_mm","end_y_mm","width_mm"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.add-track", args); }
        });
        g_orchestrator->register_tool({
            "pcb.place-footprint", "Place Footprint", ccad::TaskRisk::LowMutation,
            R"({"type":"object","required":["file","footprint","component","at_x_mm","at_y_mm","layer"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.place-footprint", args); }
        });
        g_orchestrator->register_tool({
            "pcb.export", "Export KiCad Board", ccad::TaskRisk::External,
            R"({"type":"object","required":["file","output"]})",
            [](const std::string& args) -> std::string { return executeCliTool("pcb.export", args); }
        });
        g_orchestrator->register_tool({
            "project.diagnostics", "Diagnostics", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string& args) -> std::string {
                return buildContext(extractStringValue(args, "project_path")).to_json();
            }
        });
        g_orchestrator->register_tool({
            "project.context", "Context", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string& args) -> std::string {
                return buildContext(extractStringValue(args, "project_path")).to_json();
            }
        });
        g_orchestrator->register_tool({
            "project.object_counts", "Counts", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string& args) -> std::string {
                return buildContext(extractStringValue(args, "project_path")).to_json();
            }
        });
    }
    return *g_orchestrator;
}

} // namespace

int agentOrchestratorCommand(const std::vector<std::string>& args) {
    if (args.empty()) return 1;

    try {
        if (args[0] == "orchestrator-schema") {
            std::cout << getOrchestrator().orchestrator_schema() << "\n";
            return 0;
        }

        if (args[0] == "plan" || args[0] == "orchestrate") {
            const std::map<std::string, std::string> options =
                parseOptions(args, 1, {"--project", "--goal", "--dry-run"});
            
            const std::string goal = requireOption(options, "--goal");
            const std::string project_path = optionOrEmpty(options, "--project");
            const bool dry_run = optionOrEmpty(options, "--dry-run") == "true";

            auto context = buildContext(project_path);
            auto& orch = getOrchestrator();
            
            ccad::OrchestratorConfig cfg;
            cfg.dry_run = dry_run;
            cfg.require_approval = false; // For CLI headless tests, assume approved or handled by external
            orch.set_config(cfg);

            if (args[0] == "plan") {
                auto plan = orch.plan(goal, context);
                std::cout << orch.goal_status_json(plan) << "\n";
            } else {
                auto exec = orch.orchestrate(goal, context);
                std::cout << orch.goal_status_json(exec) << "\n";
            }
            return 0;
        }

    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 2;
    }

    return 1; // Not handled
}

bool handleOrchestratorJsonRpc(const std::string& method,
                               const std::string& line,
                               const std::string& id,
                               bool allow_read,
                               bool allow_write) {
    auto& orch = getOrchestrator();

    if (method == "agent.orchestrator_schema") {
        std::cout << formatSuccess(id, orch.orchestrator_schema()) << "\n";
        std::cout.flush();
        return true;
    }
    if (method == "agent.plan") {
        std::string goal = extractStringValue(line, "goal");
        std::string project_path = extractStringValue(line, "project_path");
        if (goal.empty()) {
            std::cout << formatError(id, -32602, "Invalid params: goal required") << "\n";
        } else {
            auto context = buildContext(project_path);
            auto plan = orch.plan(goal, context);
            std::cout << formatSuccess(id, orch.goal_status_json(plan)) << "\n";
        }
        std::cout.flush();
        return true;
    }
    if (method == "agent.orchestrate") {
        std::string goal = extractStringValue(line, "goal");
        std::string project_path = extractStringValue(line, "project_path");
        if (goal.empty()) {
            std::cout << formatError(id, -32602, "Invalid params: goal required") << "\n";
        } else {
            if (!allow_write) {
                std::cout << formatError(id, -32604, "Approval required: orchestrator mutates project") << "\n";
            } else {
                auto context = buildContext(project_path);
                auto exec = orch.orchestrate(goal, context);
                std::cout << formatSuccess(id, orch.goal_status_json(exec)) << "\n";
            }
        }
        std::cout.flush();
        return true;
    }
    if (method == "agent.tool_call") {
        const std::string tool_name = extractStringValue(line, "name");
        const auto tool = orch.get_tool(tool_name);
        if (!tool) {
            std::cout << formatError(id, -32601, "Unknown orchestrator tool: " + tool_name) << "\n";
        } else if ((tool->default_risk != ccad::TaskRisk::ReadOnly) && !allow_write) {
            std::cout << formatError(id, -32604, "Approval required: tool mutates or exports project") << "\n";
        } else {
            ccad::OrchestratorConfig cfg = orch.get_config();
            cfg.require_approval = false;
            std::cout << formatSuccess(id, orch.execute_tool(tool_name, line, cfg)) << "\n";
        }
        std::cout.flush();
        return true;
    }

    return false;
}

} // namespace ccad_cli
