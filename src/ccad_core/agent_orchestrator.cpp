// Agent Orchestration Layer — Implementation
// Sprint 208: Phase 8

#include "agent_orchestrator.hpp"
#include "ccad_core/json.hpp" // for escapeJson
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

AgentOrchestrator::AgentOrchestrator() = default;

void AgentOrchestrator::set_config(const OrchestratorConfig& cfg) {
    config_ = cfg;
}

OrchestratorConfig AgentOrchestrator::get_config() const {
    return config_;
}

void AgentOrchestrator::register_tool(const OrchestratorTool& tool) {
    tools_[tool.name] = tool;
}

std::vector<std::string> AgentOrchestrator::list_tools() const {
    std::vector<std::string> names;
    for (auto& [name, _] : tools_) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

std::optional<OrchestratorTool> AgentOrchestrator::get_tool(
    const std::string& name) const {
    auto it = tools_.find(name);
    if (it != tools_.end()) return it->second;
    return std::nullopt;
}

AgentGoal AgentOrchestrator::plan(const std::string& goal_description,
                                  const ProjectContext& context) {
    AgentGoal goal;
    goal.id = make_goal_id();
    goal.description = goal_description;
    goal.context_json = context.to_json();
    goal.status = GoalStatus::Planning;
    goal.created_at = now_iso();

    goal.tasks = decompose_goal(goal_description, context);
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
    if (goal.status == GoalStatus::Canceled ||
        goal.status == GoalStatus::Completed) {
        return goal;
    }

    goal.status = GoalStatus::Active;
    goal.started_at = now_iso();

    for (auto& task : goal.tasks) {
        if (task.status != TaskStatus::Pending &&
            task.status != TaskStatus::Blocked) {
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

        if (config_.require_approval &&
            task.risk != TaskRisk::ReadOnly &&
            !config_.auto_execute_reads) {
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
    } else if (goal.failed_count > 0 &&
               goal.completed_count + goal.failed_count == goal.total_count) {
        goal.status = GoalStatus::Failed;
        goal.completed_at = now_iso();
    }

    return goal;
}

AgentTask AgentOrchestrator::execute_task(AgentGoal& goal,
                                          const std::string& task_id) {
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
        task.result_json = dispatch_tool(task.tool_name, task.tool_args_json);
        task.status = TaskStatus::Completed;
        task.completed_at = now_iso();
    } catch (const std::exception& e) {
        task.status = TaskStatus::Failed;
        task.error_message = e.what();
        task.completed_at = now_iso();
    }

    return task;
}

AgentGoal AgentOrchestrator::orchestrate(
    const std::string& goal_description,
    const ProjectContext& context) {
    auto goal = plan(goal_description, context);
    if (goal.status == GoalStatus::Failed) return goal;
    return execute(goal);
}

void AgentOrchestrator::cancel(AgentGoal& goal) {
    goal.status = GoalStatus::Canceled;
    goal.completed_at = now_iso();
    for (auto& t : goal.tasks) {
        if (t.status == TaskStatus::Pending ||
            t.status == TaskStatus::Blocked ||
            t.status == TaskStatus::Running) {
            t.status = TaskStatus::Skipped;
            t.error_message = "goal_canceled";
        }
    }
}

std::string AgentOrchestrator::goal_status_json(
    const AgentGoal& goal) const {
    return goal.to_json();
}

std::string AgentOrchestrator::task_list_json(
    const AgentGoal& goal) const {
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
        << "\"version\":\"0.1.0\","
        << "\"description\":\"CCad Agent Orchestration Layer - deterministic goal decomposition and kernel tool dispatch\","
        << "\"config\":" << config_.to_json() << ","
        << "\"registered_tools\":[";
    
    bool first = true;
    for (auto& [name, tool] : tools_) {
        if (!first) out << ",";
        first = false;
        out << "{"
            << "\"name\":\"" << escapeJson(name) << "\","
            << "\"description\":\"" << escapeJson(tool.description) << "\","
            << "\"risk\":\"" << escapeJson(task_risk_string(tool.default_risk)) << "\"";
        if (!tool.parameter_schema_json.empty()) {
            out << ",\"parameters\":" << tool.parameter_schema_json;
        }
        out << "}";
    }
    
    out << "],"
        << "\"registered_tool_count\":" << tools_.size() << ","
        << "\"capabilities\":[\"goal_decomposition\",\"task_planning\",\"dependency_tracking\",\"tool_dispatch\",\"result_observation\",\"risk_classification\",\"dry_run\",\"approval_gating\"],"
        << "\"provider_execution_enabled\":" << (config_.provider_execution_enabled ? "true" : "false") << ","
        << "\"project_mutation_enabled\":" << (config_.project_mutation_enabled ? "true" : "false")
        << "}";
    return out.str();
}

std::vector<AgentTask> AgentOrchestrator::decompose_goal(
    const std::string& goal, const ProjectContext& ctx) {

    std::string lower_goal = goal;
    std::transform(lower_goal.begin(), lower_goal.end(),
                   lower_goal.begin(), ::tolower);

    if (lower_goal.find("add") != std::string::npos &&
        lower_goal.find("via") != std::string::npos) {
        return plan_add_via(goal, ctx);
    }
    if (lower_goal.find("route") != std::string::npos ||
        (lower_goal.find("add") != std::string::npos &&
         lower_goal.find("track") != std::string::npos)) {
        return plan_add_track(goal, ctx);
    }
    if (lower_goal.find("place") != std::string::npos ||
        (lower_goal.find("add") != std::string::npos &&
         lower_goal.find("component") != std::string::npos) ||
        (lower_goal.find("add") != std::string::npos &&
         lower_goal.find("footprint") != std::string::npos)) {
        return plan_add_component(goal, ctx);
    }
    if (lower_goal.find("drc") != std::string::npos ||
        lower_goal.find("design rule") != std::string::npos ||
        lower_goal.find("check") != std::string::npos) {
        return plan_run_drc(goal, ctx);
    }
    if (lower_goal.find("export") != std::string::npos ||
        lower_goal.find("gerber") != std::string::npos ||
        lower_goal.find("drill") != std::string::npos) {
        return plan_export(goal, ctx);
    }
    if (lower_goal.find("review") != std::string::npos ||
        lower_goal.find("inspect") != std::string::npos ||
        lower_goal.find("status") != std::string::npos) {
        return plan_review(goal, ctx);
    }

    return plan_generic(goal, ctx);
}

std::vector<AgentTask> AgentOrchestrator::plan_add_via(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask review;
    review.description = "Review current board state before adding via";
    review.tool_name = "project.review";
    review.tool_args_json = "{}";
    review.risk = TaskRisk::ReadOnly;
    tasks.push_back(review);

    AgentTask drc_before;
    drc_before.description = "Run DRC to check current board health";
    drc_before.tool_name = "pcb.drc";
    drc_before.tool_args_json = "{}";
    drc_before.risk = TaskRisk::ReadOnly;
    tasks.push_back(drc_before);

    AgentTask add_via;
    add_via.description = "Add via as requested: " + goal;
    add_via.tool_name = "pcb.add-via";
    add_via.tool_args_json = "{\"goal_text\":\"" + escapeJson(goal) + "\"}";
    add_via.risk = TaskRisk::LowMutation;
    add_via.depends_on = {""};
    tasks.push_back(add_via);

    AgentTask drc_after;
    drc_after.description = "Run DRC to verify via placement doesn't violate rules";
    drc_after.tool_name = "pcb.drc";
    drc_after.tool_args_json = "{}";
    drc_after.risk = TaskRisk::ReadOnly;
    tasks.push_back(drc_after);

    return tasks;
}

std::vector<AgentTask> AgentOrchestrator::plan_add_track(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask review;
    review.description = "Review board state and net connectivity";
    review.tool_name = "project.review";
    review.tool_args_json = "{}";
    review.risk = TaskRisk::ReadOnly;
    tasks.push_back(review);

    AgentTask add_track;
    add_track.description = "Add track/route: " + goal;
    add_track.tool_name = "pcb.add-track";
    add_track.tool_args_json = "{\"goal_text\":\"" + escapeJson(goal) + "\"}";
    add_track.risk = TaskRisk::LowMutation;
    tasks.push_back(add_track);

    AgentTask drc;
    drc.description = "Run DRC after routing";
    drc.tool_name = "pcb.drc";
    drc.tool_args_json = "{}";
    drc.risk = TaskRisk::ReadOnly;
    tasks.push_back(drc);

    return tasks;
}

std::vector<AgentTask> AgentOrchestrator::plan_add_component(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask review;
    review.description = "Review current board/schematic state";
    review.tool_name = "project.review";
    review.tool_args_json = "{}";
    review.risk = TaskRisk::ReadOnly;
    tasks.push_back(review);

    AgentTask place;
    place.description = "Place component: " + goal;
    place.tool_name = "pcb.place-footprint";
    place.tool_args_json = "{\"goal_text\":\"" + escapeJson(goal) + "\"}";
    place.risk = TaskRisk::LowMutation;
    tasks.push_back(place);

    AgentTask drc;
    drc.description = "Run DRC after placement";
    drc.tool_name = "pcb.drc";
    drc.tool_args_json = "{}";
    drc.risk = TaskRisk::ReadOnly;
    tasks.push_back(drc);

    return tasks;
}

std::vector<AgentTask> AgentOrchestrator::plan_run_drc(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask drc;
    drc.description = "Run Design Rule Check: " + goal;
    drc.tool_name = "pcb.drc";
    drc.tool_args_json = "{}";
    drc.risk = TaskRisk::ReadOnly;
    tasks.push_back(drc);

    AgentTask review;
    review.description = "Review DRC results";
    review.tool_name = "project.diagnostics";
    review.tool_args_json = "{}";
    review.risk = TaskRisk::ReadOnly;
    tasks.push_back(review);

    return tasks;
}

std::vector<AgentTask> AgentOrchestrator::plan_export(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask drc;
    drc.description = "Run DRC before export";
    drc.tool_name = "pcb.drc";
    drc.tool_args_json = "{}";
    drc.risk = TaskRisk::ReadOnly;
    tasks.push_back(drc);

    AgentTask exp;
    exp.description = "Export: " + goal;
    exp.tool_name = "pcb.export";
    exp.tool_args_json = "{\"goal_text\":\"" + escapeJson(goal) + "\"}";
    exp.risk = TaskRisk::External;
    tasks.push_back(exp);

    return tasks;
}

std::vector<AgentTask> AgentOrchestrator::plan_review(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask review;
    review.description = "Review project: " + goal;
    review.tool_name = "project.review";
    review.tool_args_json = "{}";
    review.risk = TaskRisk::ReadOnly;
    tasks.push_back(review);

    AgentTask counts;
    counts.description = "Get object counts";
    counts.tool_name = "project.object_counts";
    counts.tool_args_json = "{}";
    counts.risk = TaskRisk::ReadOnly;
    tasks.push_back(counts);

    AgentTask diag;
    diag.description = "Run diagnostics";
    diag.tool_name = "project.diagnostics";
    diag.tool_args_json = "{}";
    diag.risk = TaskRisk::ReadOnly;
    tasks.push_back(diag);

    return tasks;
}

std::vector<AgentTask> AgentOrchestrator::plan_generic(
    const std::string& goal, const ProjectContext& /*ctx*/) {
    std::vector<AgentTask> tasks;

    AgentTask context_task;
    context_task.description = "Gather project context for goal: " + goal;
    context_task.tool_name = "project.context";
    context_task.tool_args_json = "{}";
    context_task.risk = TaskRisk::ReadOnly;
    tasks.push_back(context_task);

    AgentTask plan_task;
    plan_task.description = "Goal requires LLM planning (provider not enabled): " + goal;
    plan_task.tool_name = "agent.plan_with_provider";
    plan_task.tool_args_json = "{\"goal\":\"" + escapeJson(goal) + "\"}";
    plan_task.risk = TaskRisk::External;
    plan_task.status = TaskStatus::Blocked;
    plan_task.error_message = "provider_execution_disabled";
    tasks.push_back(plan_task);

    return tasks;
}

std::string AgentOrchestrator::dispatch_tool(
    const std::string& tool_name, const std::string& args_json) {

    auto it = tools_.find(tool_name);
    if (it == tools_.end()) {
        std::ostringstream err;
        err << "{\"error\":\"tool_not_registered\",\"tool_name\":\"" << escapeJson(tool_name) << "\"}";
        return err.str();
    }

    if (!config_.project_mutation_enabled &&
        it->second.default_risk != TaskRisk::ReadOnly) {
        std::ostringstream err;
        err << "{\"error\":\"project_mutation_disabled\",\"tool_name\":\"" << escapeJson(tool_name) << "\"}";
        return err.str();
    }

    return it->second.execute(args_json);
}

bool AgentOrchestrator::check_dependencies(
    const AgentTask& task, const AgentGoal& goal) const {
    for (auto& dep_id : task.depends_on) {
        if (dep_id.empty()) continue;
        for (auto& t : goal.tasks) {
            if (t.id == dep_id && t.status != TaskStatus::Completed) {
                return false;
            }
        }
    }
    return true;
}

std::string AgentOrchestrator::make_task_id(
    const std::string& goal_id, int index) {
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
