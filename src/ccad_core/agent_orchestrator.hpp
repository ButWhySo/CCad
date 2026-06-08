#pragma once
// Agent Orchestration Layer — Core Types and Engine
// Sprint 208: Phase 8 — Agent Runtime and EDA Evidence Expansion
//
// The orchestration layer provides:
// 1. Goal representation and decomposition
// 2. Task planning and dependency tracking
// 3. Tool dispatch through the kernel command surface
// 4. Result observation and state management
// 5. Context management for agent workflows

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>
#include <map>

namespace ccad {

// ─── Goal Status ────────────────────────────────────────────────
enum class GoalStatus {
    Pending,      // Received but not yet planned
    Planning,     // Being decomposed into tasks
    Active,       // Tasks are being executed
    Paused,       // Execution paused by user/policy
    Completed,    // All tasks completed successfully
    Failed,       // One or more tasks failed terminally
    Canceled      // Canceled by user
};

std::string goal_status_string(GoalStatus s);

// ─── Task Status ────────────────────────────────────────────────
enum class TaskStatus {
    Pending,      // Not yet started
    Blocked,      // Waiting on dependency
    Running,      // Currently executing
    Completed,    // Finished successfully
    Failed,       // Execution failed
    Skipped       // Skipped (dependency failed or user skip)
};

std::string task_status_string(TaskStatus s);

// ─── Task Risk Level ────────────────────────────────────────────
enum class TaskRisk {
    ReadOnly,     // No mutations — safe to auto-execute
    LowMutation,  // Minor project mutation — needs confirmation
    HighMutation, // Major structural change — requires approval
    External      // Calls external process — always requires approval
};

std::string task_risk_string(TaskRisk r);

// ─── Agent Task ─────────────────────────────────────────────────
struct AgentTask {
    std::string id;
    std::string description;
    std::string tool_name;
    std::string tool_args_json;          // Arguments as JSON string
    TaskStatus status = TaskStatus::Pending;
    TaskRisk risk = TaskRisk::ReadOnly;
    std::string result_json;             // Result as JSON string
    std::string error_message;
    std::vector<std::string> depends_on;
    int order = 0;
    std::string created_at;
    std::string started_at;
    std::string completed_at;

    std::string to_json() const;
};

// ─── Agent Goal ─────────────────────────────────────────────────
struct AgentGoal {
    std::string id;
    std::string description;
    std::string context_json;            // Project context snapshot
    GoalStatus status = GoalStatus::Pending;
    std::vector<AgentTask> tasks;
    std::string created_at;
    std::string started_at;
    std::string completed_at;
    std::string session_id;
    int completed_count = 0;
    int failed_count = 0;
    int total_count = 0;

    std::string to_json() const;
};

// ─── Tool Registration ──────────────────────────────────────────
struct OrchestratorTool {
    std::string name;
    std::string description;
    TaskRisk default_risk;
    std::string parameter_schema_json;
    // execute: takes args JSON string, returns result JSON string
    std::function<std::string(const std::string&)> execute;
};

// ─── Project Context Snapshot ───────────────────────────────────
struct ProjectContext {
    std::string project_id;
    std::string project_file;
    bool has_board = false;
    bool has_schematic = false;
    int component_count = 0;
    int net_count = 0;
    int pad_count = 0;
    int track_count = 0;
    int via_count = 0;
    int zone_count = 0;
    int drc_error_count = 0;
    int erc_error_count = 0;
    std::string active_layer;
    std::string active_net;
    std::string board_outline_json;

    std::string to_json() const;
};

// ─── Orchestrator Configuration ─────────────────────────────────
struct OrchestratorConfig {
    bool dry_run = false;
    bool require_approval = true;
    bool auto_execute_reads = true;
    int max_tasks_per_goal = 50;
    int max_retry_count = 2;
    bool provider_execution_enabled = false;
    bool project_mutation_enabled = true;

    std::string to_json() const;
};

// ─── Agent Orchestrator ─────────────────────────────────────────
class AgentOrchestrator {
public:
    AgentOrchestrator();

    void set_config(const OrchestratorConfig& cfg);
    OrchestratorConfig get_config() const;

    void register_tool(const OrchestratorTool& tool);
    std::vector<std::string> list_tools() const;
    std::optional<OrchestratorTool> get_tool(const std::string& name) const;

    AgentGoal plan(const std::string& goal_description,
                   const ProjectContext& context);

    AgentGoal execute(AgentGoal& goal);
    AgentTask execute_task(AgentGoal& goal, const std::string& task_id);
    AgentGoal orchestrate(const std::string& goal_description,
                          const ProjectContext& context);
    void cancel(AgentGoal& goal);

    std::string goal_status_json(const AgentGoal& goal) const;
    std::string task_list_json(const AgentGoal& goal) const;
    std::string orchestrator_schema() const;

private:
    OrchestratorConfig config_;
    std::map<std::string, OrchestratorTool> tools_;
    int goal_counter_ = 0;

    std::vector<AgentTask> decompose_goal(const std::string& goal,
                                          const ProjectContext& ctx);
    std::vector<AgentTask> plan_add_via(const std::string& goal,
                                        const ProjectContext& ctx);
    std::vector<AgentTask> plan_add_track(const std::string& goal,
                                          const ProjectContext& ctx);
    std::vector<AgentTask> plan_add_component(const std::string& goal,
                                              const ProjectContext& ctx);
    std::vector<AgentTask> plan_run_drc(const std::string& goal,
                                        const ProjectContext& ctx);
    std::vector<AgentTask> plan_export(const std::string& goal,
                                       const ProjectContext& ctx);
    std::vector<AgentTask> plan_review(const std::string& goal,
                                       const ProjectContext& ctx);
    std::vector<AgentTask> plan_generic(const std::string& goal,
                                        const ProjectContext& ctx);

    std::string dispatch_tool(const std::string& tool_name,
                              const std::string& args_json);
    bool check_dependencies(const AgentTask& task,
                           const AgentGoal& goal) const;
    std::string make_task_id(const std::string& goal_id, int index);
    std::string make_goal_id();
    std::string now_iso() const;
};

} // namespace ccad
