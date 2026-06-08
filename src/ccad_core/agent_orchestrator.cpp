// Agent Orchestration Layer — Implementation
// Sprint 210: Orchestrator Architecture Overhaul

#include "agent_orchestrator.hpp"
#include "ccad_core/json.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <regex>
#include <sstream>

namespace ccad {

std::string goal_status_string(GoalStatus s) {
    switch (s) {
        case GoalStatus::Pending:   return "pending";
        case GoalStatus::Planning:  return "planning";
        case GoalStatus::Active:    return "active";
        case GoalStatus::Paused:    return "paused";
        case GoalStatus::Completed: return "completed";
        case GoalStatus::Failed:    return "failed";
        case GoalStatus::Canceled:  return "canceled";
    }
    return "unknown";
}

std::string task_status_string(TaskStatus s) {
    switch (s) {
        case TaskStatus::Pending:   return "pending";
        case TaskStatus::Blocked:   return "blocked";
        case TaskStatus::Running:   return "running";
        case TaskStatus::Completed: return "completed";
        case TaskStatus::Failed:    return "failed";
        case TaskStatus::Skipped:   return "skipped";
    }
    return "unknown";
}

std::string task_risk_string(TaskRisk r) {
    switch (r) {
        case TaskRisk::ReadOnly:     return "read_only";
        case TaskRisk::LowMutation:  return "low_mutation";
        case TaskRisk::HighMutation: return "high_mutation";
        case TaskRisk::External:     return "external";
    }
    return "unknown";
}

std::string AgentTask::to_json() const {
    std::ostringstream out;
    out << "{"
        << "\"id\":\"" << escapeJson(id) << "\","
        << "\"description\":\"" << escapeJson(description) << "\","
        << "\"tool_name\":\"" << escapeJson(tool_name) << "\","
        << "\"tool_args\":" << (tool_args_json.empty() ? "{}" : tool_args_json) << ","
        << "\"status\":\"" << escapeJson(task_status_string(status)) << "\","
        << "\"risk\":\"" << escapeJson(task_risk_string(risk)) << "\","
        << "\"order\":" << order;
    if (!result_json.empty()) {
        out << ",\"result\":" << result_json;
    }
    if (!error_message.empty()) {
        out << ",\"error\":\"" << escapeJson(error_message) << "\"";
    }
    if (!depends_on.empty()) {
        out << ",\"depends_on\":[";
        for (size_t i = 0; i < depends_on.size(); ++i) {
            if (i > 0) out << ",";
            out << "\"" << escapeJson(depends_on[i]) << "\"";
        }
        out << "]";
    }
    if (!created_at.empty()) out << ",\"created_at\":\"" << escapeJson(created_at) << "\"";
    if (!started_at.empty()) out << ",\"started_at\":\"" << escapeJson(started_at) << "\"";
    if (!completed_at.empty()) out << ",\"completed_at\":\"" << escapeJson(completed_at) << "\"";
    out << "}";
    return out.str();
}

std::string AgentGoal::to_json() const {
    std::ostringstream out;
    out << "{"
        << "\"id\":\"" << escapeJson(id) << "\","
        << "\"description\":\"" << escapeJson(description) << "\","
        << "\"status\":\"" << escapeJson(goal_status_string(status)) << "\","
        << "\"total_tasks\":" << total_count << ","
        << "\"completed_tasks\":" << completed_count << ","
        << "\"failed_tasks\":" << failed_count;
    if (!context_json.empty()) {
        out << ",\"context\":" << context_json;
    }
    if (!created_at.empty()) out << ",\"created_at\":\"" << escapeJson(created_at) << "\"";
    if (!started_at.empty()) out << ",\"started_at\":\"" << escapeJson(started_at) << "\"";
    if (!completed_at.empty()) out << ",\"completed_at\":\"" << escapeJson(completed_at) << "\"";
    if (!session_id.empty()) out << ",\"session_id\":\"" << escapeJson(session_id) << "\"";
    
    out << ",\"tasks\":[";
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (i > 0) out << ",";
        out << tasks[i].to_json();
    }
    out << "]}";
    return out.str();
}

std::string ProjectContext::to_json() const {
    std::ostringstream out;
    out << "{"
        << "\"project_id\":\"" << escapeJson(project_id) << "\","
        << "\"project_file\":\"" << escapeJson(project_file) << "\","
        << "\"has_board\":" << (has_board ? "true" : "false") << ","
        << "\"has_schematic\":" << (has_schematic ? "true" : "false") << ","
        << "\"component_count\":" << component_count << ","
        << "\"net_count\":" << net_count << ","
        << "\"pad_count\":" << pad_count << ","
        << "\"track_count\":" << track_count << ","
        << "\"via_count\":" << via_count << ","
        << "\"zone_count\":" << zone_count << ","
        << "\"drc_error_count\":" << drc_error_count << ","
        << "\"erc_error_count\":" << erc_error_count;
    if (!active_layer.empty()) out << ",\"active_layer\":\"" << escapeJson(active_layer) << "\"";
    if (!active_net.empty()) out << ",\"active_net\":\"" << escapeJson(active_net) << "\"";
    if (!board_outline_json.empty()) {
        out << ",\"board_outline\":" << board_outline_json;
    }
    out << "}";
    return out.str();
}

std::string OrchestratorConfig::to_json() const {
    std::ostringstream out;
    out << "{"
        << "\"dry_run\":" << (dry_run ? "true" : "false") << ","
        << "\"require_approval\":" << (require_approval ? "true" : "false") << ","
        << "\"auto_execute_reads\":" << (auto_execute_reads ? "true" : "false") << ","
        << "\"max_tasks_per_goal\":" << max_tasks_per_goal << ","
        << "\"max_retry_count\":" << max_retry_count << ","
        << "\"provider_execution_enabled\":" << (provider_execution_enabled ? "true" : "false") << ","
        << "\"project_mutation_enabled\":" << (project_mutation_enabled ? "true" : "false")
        << "}";
    return out.str();
}

// ─── IntakeLayer ────────────────────────────────────────────────
std::string IntakeLayer::normalize_request(const std::string& input) {
    return input;
}

std::string IntakeLayer::classify_intent(const std::string& /*input*/) {
    return "EDA";
}

bool IntakeLayer::run_risk_scan(const std::string& /*input*/) {
    return true;
}

RunRecord IntakeLayer::start_session(const std::string& /*input*/) {
    return {"run_001", "user", "workspace", "main"};
}

// ─── ContextBuilder ─────────────────────────────────────────────
void ContextBuilder::load_stable_prompts() {}
void ContextBuilder::load_project_memory() {}
void ContextBuilder::load_repo_map() {}
std::string ContextBuilder::build_context(const ProjectContext& base_ctx) {
    return base_ctx.to_json();
}

// ─── ToolBroker ─────────────────────────────────────────────────
void ToolBroker::register_tool(const OrchestratorTool& tool) {
    tools_[tool.name] = tool;
}

std::vector<std::string> ToolBroker::list_tools() const {
    std::vector<std::string> names;
    for (auto& [name, _] : tools_) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

std::optional<OrchestratorTool> ToolBroker::get_tool(const std::string& name) const {
    auto it = tools_.find(name);
    if (it != tools_.end()) return it->second;
    return std::nullopt;
}

bool ToolBroker::check_policy(const OrchestratorTool& tool, const OrchestratorConfig& cfg) {
    if (!cfg.project_mutation_enabled && tool.default_risk != TaskRisk::ReadOnly) {
        return false;
    }
    return true;
}

std::string ToolBroker::execute_tool(const std::string& name, const std::string& args_json, const OrchestratorConfig& cfg) {
    auto it = tools_.find(name);
    if (it == tools_.end()) {
        return "{\"error\":\"tool_not_registered\",\"tool_name\":\"" + escapeJson(name) + "\"}";
    }
    if (!check_policy(it->second, cfg)) {
        return "{\"error\":\"project_mutation_disabled\",\"tool_name\":\"" + escapeJson(name) + "\"}";
    }
    return it->second.execute(args_json);
}

// ─── EDAAgent ───────────────────────────────────────────────────
AgentTask make_task(const std::string& desc, const std::string& tool, const std::string& args, TaskRisk risk) {
    AgentTask t;
    t.description = desc;
    t.tool_name = tool;
    t.tool_args_json = args;
    t.risk = risk;
    return t;
}

std::vector<AgentTask> EDAAgent::decompose(const std::string& goal, const ProjectContext& /*ctx*/) {
    std::string lower_goal = goal;
    std::transform(lower_goal.begin(), lower_goal.end(), lower_goal.begin(), ::tolower);
    std::vector<AgentTask> tasks;

    if (lower_goal.find("add") != std::string::npos && lower_goal.find("via") != std::string::npos) {
        tasks.push_back(make_task("Review current board state before adding via", "project.review", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Run DRC to check current board health", "pcb.drc", "{}", TaskRisk::ReadOnly));
        auto add = make_task("Add via as requested: " + goal, "pcb.add-via", "{\"goal_text\":\"" + escapeJson(goal) + "\"}", TaskRisk::LowMutation);
        add.depends_on.push_back("");
        tasks.push_back(add);
        tasks.push_back(make_task("Run DRC to verify via placement doesn't violate rules", "pcb.drc", "{}", TaskRisk::ReadOnly));
        return tasks;
    }
    if (lower_goal.find("route") != std::string::npos || (lower_goal.find("add") != std::string::npos && lower_goal.find("track") != std::string::npos)) {
        tasks.push_back(make_task("Review board state and net connectivity", "project.review", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Add track/route: " + goal, "pcb.add-track", "{\"goal_text\":\"" + escapeJson(goal) + "\"}", TaskRisk::LowMutation));
        tasks.push_back(make_task("Run DRC after routing", "pcb.drc", "{}", TaskRisk::ReadOnly));
        return tasks;
    }
    if (lower_goal.find("place") != std::string::npos || (lower_goal.find("add") != std::string::npos && lower_goal.find("component") != std::string::npos) || (lower_goal.find("add") != std::string::npos && lower_goal.find("footprint") != std::string::npos)) {
        tasks.push_back(make_task("Review current board/schematic state", "project.review", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Place component: " + goal, "pcb.place-footprint", "{\"goal_text\":\"" + escapeJson(goal) + "\"}", TaskRisk::LowMutation));
        tasks.push_back(make_task("Run DRC after placement", "pcb.drc", "{}", TaskRisk::ReadOnly));
        return tasks;
    }
    if (lower_goal.find("drc") != std::string::npos || lower_goal.find("design rule") != std::string::npos || lower_goal.find("check") != std::string::npos) {
        tasks.push_back(make_task("Run Design Rule Check: " + goal, "pcb.drc", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Review DRC results", "project.diagnostics", "{}", TaskRisk::ReadOnly));
        return tasks;
    }
    if (lower_goal.find("export") != std::string::npos || lower_goal.find("gerber") != std::string::npos || lower_goal.find("drill") != std::string::npos) {
        tasks.push_back(make_task("Run DRC before export", "pcb.drc", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Export: " + goal, "pcb.export", "{\"goal_text\":\"" + escapeJson(goal) + "\"}", TaskRisk::External));
        return tasks;
    }
    if (lower_goal.find("review") != std::string::npos || lower_goal.find("inspect") != std::string::npos || lower_goal.find("status") != std::string::npos) {
        tasks.push_back(make_task("Review project: " + goal, "project.review", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Get object counts", "project.object_counts", "{}", TaskRisk::ReadOnly));
        tasks.push_back(make_task("Run diagnostics", "project.diagnostics", "{}", TaskRisk::ReadOnly));
        return tasks;
    }
    
    // Generic
    tasks.push_back(make_task("Gather project context for goal: " + goal, "project.context", "{}", TaskRisk::ReadOnly));
    auto plan_task = make_task("Goal requires LLM planning (provider not enabled): " + goal, "agent.plan_with_provider", "{\"goal\":\"" + escapeJson(goal) + "\"}", TaskRisk::External);
    plan_task.status = TaskStatus::Blocked;
    plan_task.error_message = "provider_execution_disabled";
    tasks.push_back(plan_task);
    return tasks;
}

// ─── AgentOrchestrator ──────────────────────────────────────────
AgentOrchestrator::AgentOrchestrator() {
    available_subagents_.push_back(std::make_unique<EDAAgent>());
}

void AgentOrchestrator::set_config(const OrchestratorConfig& cfg) {
    config_ = cfg;
}

OrchestratorConfig AgentOrchestrator::get_config() const {
    return config_;
}

void AgentOrchestrator::register_tool(const OrchestratorTool& tool) {
    tool_broker_.register_tool(tool);
}

std::vector<std::string> AgentOrchestrator::list_tools() const {
    return tool_broker_.list_tools();
}

std::optional<OrchestratorTool> AgentOrchestrator::get_tool(const std::string& name) const {
    return tool_broker_.get_tool(name);
}

std::string AgentOrchestrator::execute_tool(const std::string& name, const std::string& args_json, const OrchestratorConfig& cfg) {
    return tool_broker_.execute_tool(name, args_json, cfg);
}

AgentGoal AgentOrchestrator::plan(const std::string& goal_description, const ProjectContext& context) {
    AgentGoal goal;
    goal.id = make_goal_id();
    goal.description = intake_.normalize_request(goal_description);
    intake_.run_risk_scan(goal.description);
    intake_.start_session(goal.description);
    
    goal.context_json = context_builder_.build_context(context);
    goal.status = GoalStatus::Planning;
    goal.created_at = now_iso();

    // Use the first subagent for decomposition (EDAAgent)
    if (!available_subagents_.empty()) {
        goal.tasks = available_subagents_[0]->decompose(goal.description, context);
    }
    
    goal.total_count = static_cast<int>(goal.tasks.size());

    for (int i = 0; i < (int)goal.tasks.size(); ++i) {
        goal.tasks[i].id = make_task_id(goal.id, i);
        goal.tasks[i].order = i;
        goal.tasks[i].created_at = now_iso();
    }

    goal.status = goal.tasks.empty() ? GoalStatus::Failed : GoalStatus::Pending;
    if (goal.tasks.empty()) {
        AgentTask fallback;
        fallback.id = make_task_id(goal.id, 0);
        fallback.description = "Could not decompose goal into executable tasks";
        fallback.status = TaskStatus::Failed;
        fallback.error_message = "No matching decomposition pattern for: " + goal_description;
        fallback.created_at = now_iso();
        goal.tasks.push_back(fallback);
        goal.total_count = 1;
        goal.failed_count = 1;
    }

    return goal;
}

AgentGoal AgentOrchestrator::execute(AgentGoal& goal) {
    if (goal.status == GoalStatus::Canceled || goal.status == GoalStatus::Completed) {
        return goal;
    }
    goal.status = GoalStatus::Active;
    goal.started_at = now_iso();

    for (auto& task : goal.tasks) {
        if (task.status != TaskStatus::Pending && task.status != TaskStatus::Blocked) {
            continue;
        }
        if (!check_dependencies(task, goal)) {
            task.status = TaskStatus::Blocked;
            continue;
        }
        if (config_.dry_run) {
            task.status = TaskStatus::Skipped;
            task.error_message = "dry_run_only";
            continue;
        }
        if (config_.require_approval && task.risk != TaskRisk::ReadOnly && !config_.auto_execute_reads) {
            task.status = TaskStatus::Blocked;
            task.error_message = "approval_required";
            continue;
        }
        if (task.risk == TaskRisk::ReadOnly || !config_.require_approval) {
            task = execute_task(goal, task.id);
        } else if (config_.auto_execute_reads && task.risk == TaskRisk::ReadOnly) {
            task = execute_task(goal, task.id);
        } else {
            task.status = TaskStatus::Blocked;
            task.error_message = "approval_required";
        }
    }

    goal.completed_count = 0;
    goal.failed_count = 0;
    for (auto& t : goal.tasks) {
        if (t.status == TaskStatus::Completed) goal.completed_count++;
        if (t.status == TaskStatus::Failed) goal.failed_count++;
    }

    if (goal.completed_count == goal.total_count) {
        goal.status = GoalStatus::Completed;
        goal.completed_at = now_iso();
    } else if (goal.failed_count > 0 && goal.completed_count + goal.failed_count == goal.total_count) {
        goal.status = GoalStatus::Failed;
        goal.completed_at = now_iso();
    }
    return goal;
}

AgentTask AgentOrchestrator::execute_task(AgentGoal& goal, const std::string& task_id) {
    AgentTask* task_ptr = nullptr;
    for (auto& t : goal.tasks) {
        if (t.id == task_id) { task_ptr = &t; break; }
    }
    if (!task_ptr) {
        AgentTask err;
        err.id = task_id;
        err.status = TaskStatus::Failed;
        err.error_message = "task_not_found";
        return err;
    }

    auto& task = *task_ptr;
    task.status = TaskStatus::Running;
    task.started_at = now_iso();

    try {
        task.result_json = tool_broker_.execute_tool(task.tool_name, task.tool_args_json, config_);
        task.status = TaskStatus::Completed;
        task.completed_at = now_iso();
    } catch (const std::exception& e) {
        task.status = TaskStatus::Failed;
        task.error_message = e.what();
        task.completed_at = now_iso();
    }
    return task;
}

AgentGoal AgentOrchestrator::orchestrate(const std::string& goal_description, const ProjectContext& context) {
    auto goal = plan(goal_description, context);
    if (goal.status == GoalStatus::Failed) return goal;
    return execute(goal);
}

void AgentOrchestrator::cancel(AgentGoal& goal) {
    goal.status = GoalStatus::Canceled;
    goal.completed_at = now_iso();
    for (auto& t : goal.tasks) {
        if (t.status == TaskStatus::Pending || t.status == TaskStatus::Blocked || t.status == TaskStatus::Running) {
            t.status = TaskStatus::Skipped;
            t.error_message = "goal_canceled";
        }
    }
}

std::string AgentOrchestrator::goal_status_json(const AgentGoal& goal) const {
    return goal.to_json();
}

std::string AgentOrchestrator::task_list_json(const AgentGoal& goal) const {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < goal.tasks.size(); ++i) {
        if (i > 0) out << ",";
        out << goal.tasks[i].to_json();
    }
    out << "]";
    return out.str();
}

std::string AgentOrchestrator::orchestrator_schema() const {
    std::ostringstream out;
    out << "{"
        << "\"name\":\"agent_orchestrator\","
        << "\"version\":\"0.2.0\","
        << "\"description\":\"CCad Agent Orchestration Layer (Sprint 210 refactor) - multi-agent framework\","
        << "\"config\":" << config_.to_json() << ","
        << "\"registered_tools\":[";
    
    bool first = true;
    for (auto name : tool_broker_.list_tools()) {
        auto tool = tool_broker_.get_tool(name);
        if (!tool) continue;
        if (!first) out << ",";
        first = false;
        out << "{"
            << "\"name\":\"" << escapeJson(tool->name) << "\","
            << "\"description\":\"" << escapeJson(tool->description) << "\","
            << "\"risk\":\"" << escapeJson(task_risk_string(tool->default_risk)) << "\"";
        if (!tool->parameter_schema_json.empty()) {
            out << ",\"parameters\":" << tool->parameter_schema_json;
        }
        out << "}";
    }
    
    out << "],"
        << "\"registered_tool_count\":" << tool_broker_.list_tools().size() << ","
        << "\"capabilities\":[\"intake_layer\",\"context_fabric\",\"supervisor_orchestrator\",\"subagents\",\"tool_broker\"],"
        << "\"provider_execution_enabled\":" << (config_.provider_execution_enabled ? "true" : "false") << ","
        << "\"project_mutation_enabled\":" << (config_.project_mutation_enabled ? "true" : "false")
        << "}";
    return out.str();
}

bool AgentOrchestrator::check_dependencies(const AgentTask& task, const AgentGoal& goal) const {
    for (auto& dep_id : task.depends_on) {
        if (dep_id.empty()) continue;
        for (auto& t : goal.tasks) {
            if (t.id == dep_id && t.status != TaskStatus::Completed) return false;
        }
    }
    return true;
}

std::string AgentOrchestrator::make_task_id(const std::string& goal_id, int index) {
    return goal_id + "_t" + std::to_string(index);
}

std::string AgentOrchestrator::make_goal_id() {
    return "goal_" + std::to_string(++goal_counter_);
}

std::string AgentOrchestrator::now_iso() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &time_t);
#else
    gmtime_r(&time_t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace ccad
