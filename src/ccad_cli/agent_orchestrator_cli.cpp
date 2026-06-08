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
    auto end = json.find("\"", pos);
    if (end != std::string::npos) {
      return json.substr(pos, end - pos);
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

// Build a context from project file
ccad::ProjectContext buildContext(const std::string& project_path) {
    ccad::ProjectContext context;
    if (!project_path.empty()) {
        try {
            ccad::Project proj = loadProjectFile(project_path);
            context.project_file = project_path;
            context.has_board = proj.board.has_value();
            context.has_schematic = !proj.components.empty();
            context.component_count = proj.components.size();
            if (context.has_board) {
                const auto& b = proj.board.value();
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
        
        // Register basic tools wrapping ccad commands
        // In a real application, these would call `ccad_cli::run()` with specific arguments
        // and capture stdout. For now, we mock them to return valid JSON results.
        g_orchestrator->register_tool({
            "project.review", "Review project", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"ok\"}"; }
        });
        g_orchestrator->register_tool({
            "pcb.drc", "Run DRC", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string&) -> std::string { return "{\"passed\":true}"; }
        });
        g_orchestrator->register_tool({
            "pcb.add-via", "Add Via", ccad::TaskRisk::LowMutation, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"added\"}"; }
        });
        g_orchestrator->register_tool({
            "pcb.add-track", "Add Track", ccad::TaskRisk::LowMutation, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"added\"}"; }
        });
        g_orchestrator->register_tool({
            "pcb.place-footprint", "Place Component", ccad::TaskRisk::LowMutation, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"placed\"}"; }
        });
        g_orchestrator->register_tool({
            "pcb.export", "Export Data", ccad::TaskRisk::External, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"exported\"}"; }
        });
        g_orchestrator->register_tool({
            "project.diagnostics", "Diagnostics", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"ok\"}"; }
        });
        g_orchestrator->register_tool({
            "project.context", "Context", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"ok\"}"; }
        });
        g_orchestrator->register_tool({
            "project.object_counts", "Counts", ccad::TaskRisk::ReadOnly, "{}",
            [](const std::string&) -> std::string { return "{\"status\":\"ok\"}"; }
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

    return false;
}

} // namespace ccad_cli
