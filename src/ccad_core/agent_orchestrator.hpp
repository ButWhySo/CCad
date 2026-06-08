#pragma once
// Agent Orchestration Layer — Core Types and Engine
// Sprint 210: Orchestrator Architecture Overhaul

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>
#include <map>

namespace ccad {

// ─── Goal Status ────────────────────────────────────────────────
enum class GoalStatus {
    Pending,
    Planning,
    Active,
    Paused,
    Completed,
    Failed,
    Canceled
};
std::string goal_status_string(GoalStatus s);

// ─── Task Status ────────────────────────────────────────────────
enum class TaskStatus {
    Pending,
    Blocked,
    Running,
    Completed,
    Failed,
    Skipped
};
std::string task_status_string(TaskStatus s);

// ─── Task Risk Level ────────────────────────────────────────────
enum class TaskRisk {
    ReadOnly,
    LowMutation,
    HighMutation,
    External
};
std::string task_risk_string(TaskRisk r);

// ─── Agent Task ─────────────────────────────────────────────────
struct AgentTask {
    std::string id;
    std::string description;
    std::string tool_name;
    std::string tool_args_json;
    TaskStatus status = TaskStatus::Pending;
    TaskRisk risk = TaskRisk::ReadOnly;
    std::string result_json;
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
    std::string context_json;
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

// ================================================================
// NEW ARCHITECTURE LAYER DEFINITIONS (Sprint 210)
// ================================================================

struct RunRecord {
    std::string run_id;
    std::string user;
    std::string workspace;
    std::string branch;
};

class IntakeLayer {
public:
    std::string normalize_request(const std::string& input);
    std::string classify_intent(const std::string& input);
    bool run_risk_scan(const std::string& input);
    RunRecord start_session(const std::string& input);
};

class ContextBuilder {
public:
    void load_stable_prompts();
    void load_project_memory();
    void load_repo_map();
    std::string build_context(const ProjectContext& base_ctx);
};

class ToolBroker {
public:
    void register_tool(const OrchestratorTool& tool);
    std::vector<std::string> list_tools() const;
    std::optional<OrchestratorTool> get_tool(const std::string& name) const;
    
    bool check_policy(const OrchestratorTool& tool, const OrchestratorConfig& cfg);
    std::string execute_tool(const std::string& name, const std::string& args_json, const OrchestratorConfig& cfg);

private:
    std::map<std::string, OrchestratorTool> tools_;
};

class Subagent {
public:
    virtual ~Subagent() = default;
    virtual std::string get_name() const = 0;
    virtual std::vector<AgentTask> decompose(const std::string& goal, const ProjectContext& ctx) = 0;
};

class EDAAgent : public Subagent {
public:
    std::string get_name() const override { return "EDAAgent"; }
    std::vector<AgentTask> decompose(const std::string& goal, const ProjectContext& ctx) override;
};

// ─── Agent Orchestrator (Supervisor) ────────────────────────────
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
    ToolBroker tool_broker_;
    IntakeLayer intake_;
    ContextBuilder context_builder_;
    std::vector<std::unique_ptr<Subagent>> available_subagents_;

    int goal_counter_ = 0;

    std::string make_task_id(const std::string& goal_id, int index);
    std::string make_goal_id();
    std::string now_iso() const;
    
    bool check_dependencies(const AgentTask& task, const AgentGoal& goal) const;
};

} // namespace ccad
