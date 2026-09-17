// Test: Agent Orchestrator
// Sprint 208: Phase 8 — Agent Runtime and EDA Evidence Expansion
//
// Validates goal decomposition, task planning, execution dispatch,
// risk classification, dependency tracking, and orchestrator schema.

#include "../src/ccad_core/agent_orchestrator.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <future>

// Helper to synchronously wait for the background AgentRunner to complete a goal
static ccad::AgentGoal wait_for_orchestrate(ccad::AgentOrchestrator& orch, const std::string& desc, const ccad::ProjectContext& ctx) {
    std::promise<ccad::AgentGoal> p;
    auto f = p.get_future();
    orch.set_progress_callback([&p](const ccad::AgentGoal& g) {
        if (g.status == ccad::GoalStatus::Completed || g.status == ccad::GoalStatus::Failed) {
            try { p.set_value(g); } catch(...) {}
        }
    });
    orch.orchestrate(desc, ctx);
    return f.get();
}


// Helper: register a mock tool that returns a success result
static void register_mock_tools(ccad::AgentOrchestrator& orch) {
    // project.review
    orch.register_tool({
        "project.review",
        "Review current project state",
        ccad::TaskRisk::ReadOnly,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"ok\",\"component_count\":4,\"net_count\":5}";
        }
    });

    // project.context
    orch.register_tool({
        "project.context",
        "Get project context",
        ccad::TaskRisk::ReadOnly,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"project_id\":\"test\",\"has_board\":true}";
        }
    });

    // project.object_counts
    orch.register_tool({
        "project.object_counts",
        "Get object counts",
        ccad::TaskRisk::ReadOnly,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"pads\":20,\"tracks\":10,\"vias\":3}";
        }
    });

    // project.diagnostics
    orch.register_tool({
        "project.diagnostics",
        "Run diagnostics",
        ccad::TaskRisk::ReadOnly,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"errors\":0,\"warnings\":2}";
        }
    });

    // pcb.drc
    orch.register_tool({
        "pcb.drc",
        "Run design rule check",
        ccad::TaskRisk::ReadOnly,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"passed\":true,\"violations\":0}";
        }
    });

    // pcb.add-via
    orch.register_tool({
        "pcb.add-via",
        "Add a via to the board",
        ccad::TaskRisk::LowMutation,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"via_added\",\"id\":\"V1\"}";
        }
    });

    // pcb.add-track
    orch.register_tool({
        "pcb.add-track",
        "Add a track to the board",
        ccad::TaskRisk::LowMutation,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"track_added\",\"id\":\"T1\"}";
        }
    });

    // pcb.place-footprint
    orch.register_tool({
        "pcb.place-footprint",
        "Place a footprint on the board",
        ccad::TaskRisk::LowMutation,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"footprint_placed\",\"id\":\"U1\"}";
        }
    });

    // pcb.export
    orch.register_tool({
        "pcb.export",
        "Export board data",
        ccad::TaskRisk::External,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"exported\",\"format\":\"gerber\"}";
        }
    });

    // pcb.add-polygon
    orch.register_tool({
        "pcb.add-polygon",
        "Add a polygon pour to the board",
        ccad::TaskRisk::LowMutation,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"polygon_added\",\"id\":\"P1\"}";
        }
    });

    // sch.place-symbol
    orch.register_tool({
        "sch.place-symbol",
        "Place a symbol on the schematic",
        ccad::TaskRisk::LowMutation,
        "{}",
        [](const std::string& args) -> std::string {
            return "{\"status\":\"symbol_placed\",\"id\":\"S1\"}";
        }
    });
}

static ccad::ProjectContext make_test_context() {
    ccad::ProjectContext ctx;
    ctx.project_id = "test-project";
    ctx.project_file = "test.ccad.json";
    ctx.has_board = true;
    ctx.has_schematic = false;
    ctx.component_count = 4;
    ctx.net_count = 5;
    ctx.pad_count = 20;
    ctx.track_count = 10;
    ctx.via_count = 3;
    ctx.active_layer = "F.Cu";
    ctx.active_net = "VCC";
    return ctx;
}

// ─── Test: Schema ───────────────────────────────────────────────
static void test_orchestrator_schema() {
    std::cout << "  test_orchestrator_schema... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto schema = orch.orchestrator_schema();
    assert(schema.find("\"name\":\"agent_orchestrator\"") != std::string::npos);
    assert(schema.find("\"registered_tool_count\":11") != std::string::npos);

    std::cout << "PASS\n";
}

// ─── Test: Tool Registry ────────────────────────────────────────
static void test_tool_registry() {
    std::cout << "  test_tool_registry... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto tools = orch.list_tools();
    assert(tools.size() == 11);

    auto via_tool = orch.get_tool("pcb.add-via");
    assert(via_tool.has_value());
    assert(via_tool->name == "pcb.add-via");
    assert(via_tool->default_risk == ccad::TaskRisk::LowMutation);

    auto missing = orch.get_tool("nonexistent");
    assert(!missing.has_value());

    std::cout << "PASS\n";
}

// ─── Test: Plan Via Goal ────────────────────────────────────────
static void test_plan_add_via() {
    std::cout << "  test_plan_add_via... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Add a via at position 10,15 on net VCC", ctx);

    assert(goal.status == ccad::GoalStatus::Pending);
    assert(goal.tasks.size() == 2);
    assert(goal.tasks[0].tool_name == "project.context");
    assert(goal.tasks[1].tool_name == "agent.plan_with_provider");
    assert(goal.total_count == 2);

    std::cout << "PASS\n";
}

// ─── Test: Plan DRC Goal ────────────────────────────────────────
static void test_plan_drc() {
    std::cout << "  test_plan_drc... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Run DRC check on the board", ctx);

    assert(goal.status == ccad::GoalStatus::Pending);
    assert(goal.tasks.size() == 2);
    assert(goal.tasks[0].tool_name == "project.context");
    assert(goal.tasks[1].tool_name == "agent.plan_with_provider");

    std::cout << "PASS\n";
}

// ─── Test: Plan Review Goal ─────────────────────────────────────
static void test_plan_review() {
    std::cout << "  test_plan_review... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Review the current board status", ctx);

    assert(goal.status == ccad::GoalStatus::Pending);
    assert(goal.tasks.size() == 2);
    assert(goal.tasks[0].tool_name == "project.context");
    assert(goal.tasks[1].tool_name == "agent.plan_with_provider");

    std::cout << "PASS\n";
}

// ─── Test: Plan Generic Goal ────────────────────────────────────
static void test_plan_generic() {
    std::cout << "  test_plan_generic... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Do something unusual with the board", ctx);

    assert(goal.status == ccad::GoalStatus::Pending);
    assert(goal.tasks.size() == 2);
    assert(goal.tasks[0].tool_name == "project.context");
    assert(goal.tasks[1].tool_name == "agent.plan_with_provider");
    assert(goal.tasks[1].status == ccad::TaskStatus::Blocked);

    std::cout << "PASS\n";
}

// ─── Test: Plan KiCad Parity Goal ───────────────────────────────
static void test_plan_kicad_parity() {
    std::cout << "  test_plan_kicad_parity... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Place a symbol, route a track, and pour a polygon", ctx);

    assert(goal.status == ccad::GoalStatus::Pending);
    
    std::cout << "PASS\n";
}

// ─── Test: Execute DRC Goal ─────────────────────────────────────
static void test_execute_drc_goal() {
    std::cout << "  test_execute_drc_goal... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    ccad::OrchestratorConfig cfg;
    cfg.require_approval = false;  // Auto-execute everything
    orch.set_config(cfg);

    auto ctx = make_test_context();
    auto goal = wait_for_orchestrate(orch, "Run DRC check", ctx);

    assert(goal.status == ccad::GoalStatus::Completed);
    assert(goal.completed_count == 2);
    assert(goal.failed_count == 0);
    assert(goal.tasks[0].status == ccad::TaskStatus::Completed);
    assert(goal.tasks[1].status == ccad::TaskStatus::Completed);

    std::cout << "PASS\n";
}

// ─── Test: Execute Review Goal ──────────────────────────────────
static void test_execute_review_goal() {
    std::cout << "  test_execute_review_goal... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    ccad::OrchestratorConfig cfg;
    cfg.require_approval = false;
    orch.set_config(cfg);

    auto ctx = make_test_context();
    auto goal = wait_for_orchestrate(orch, "Review board status", ctx);

    assert(goal.status == ccad::GoalStatus::Completed);
    assert(goal.completed_count == 2);

    std::cout << "PASS\n";
}

// ─── Test: Dry Run ──────────────────────────────────────────────
static void test_dry_run() {
    std::cout << "  test_dry_run... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    ccad::OrchestratorConfig cfg;
    cfg.dry_run = true;
    orch.set_config(cfg);

    auto ctx = make_test_context();
    auto goal = wait_for_orchestrate(orch, "Add via at 10,15", ctx);

    // All tasks should be skipped in dry run
    for (auto& t : goal.tasks) {
        assert(t.status == ccad::TaskStatus::Skipped);
        assert(t.error_message == "dry_run_only");
    }

    std::cout << "PASS\n";
}

// ─── Test: Cancel Goal ──────────────────────────────────────────
static void test_cancel_goal() {
    std::cout << "  test_cancel_goal... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Add via at 10,15", ctx);
    orch.cancel(goal);

    assert(goal.status == ccad::GoalStatus::Canceled);
    for (auto& t : goal.tasks) {
        assert(t.status == ccad::TaskStatus::Skipped);
        assert(t.error_message == "goal_canceled");
    }

    std::cout << "PASS\n";
}

// ─── Test: Goal JSON Serialization ──────────────────────────────
static void test_goal_json() {
    std::cout << "  test_goal_json... ";
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);

    auto ctx = make_test_context();
    auto goal = orch.plan("Run DRC", ctx);

    auto json = orch.goal_status_json(goal);
    assert(json.find("\"id\":") != std::string::npos);
    assert(json.find("\"status\":\"pending\"") != std::string::npos);
    assert(json.find("\"total_tasks\":2") != std::string::npos);

    auto tasks = orch.task_list_json(goal);
    assert(tasks.find("\"project.context\"") != std::string::npos);
    assert(tasks.find("\"agent.plan_with_provider\"") != std::string::npos);
    assert(tasks.find("\"status\":\"blocked\"") != std::string::npos);
    assert(tasks.find("\"provider_execution_disabled\"") != std::string::npos);

    std::cout << "PASS\n";
}

// ─── Test: Unregistered Tool ────────────────────────────────────
static void test_unregistered_tool_dispatch() {
    std::cout << "  test_unregistered_tool_dispatch... ";
    ccad::AgentOrchestrator orch;
    // Don't register any tools

    auto ctx = make_test_context();

    ccad::OrchestratorConfig cfg;
    cfg.require_approval = false;
    orch.set_config(cfg);

    auto goal = wait_for_orchestrate(orch, "Run DRC", ctx);

    // Tasks should still complete but with tool_not_registered results
    for (auto& t : goal.tasks) {
        if (t.status == ccad::TaskStatus::Completed) {
            assert(t.result_json.find("\"error\":\"tool_not_registered\"") != std::string::npos);
        }
    }

    std::cout << "PASS\n";
}

// ─── Test: Project Context JSON ─────────────────────────────────
static void test_project_context_json() {
    std::cout << "  test_project_context_json... ";

    auto ctx = make_test_context();
    auto json = ctx.to_json();

    assert(json.find("\"project_id\":\"test-project\"") != std::string::npos);
    assert(json.find("\"has_board\":true") != std::string::npos);
    assert(json.find("\"pad_count\":20") != std::string::npos);

    std::cout << "PASS\n";
}

static void test_intake_layer() {
    ccad::IntakeLayer intake;
    assert(intake.normalize_request("  route\n  the   PCB  ") == "route the PCB");
    assert(intake.classify_intent("run DRC on the board") == "PCB");
    assert(intake.classify_intent("check schematic ERC") == "Schematic");
    assert(intake.classify_intent("run SPICE simulation") == "Simulation");
    assert(intake.run_risk_scan("inspect board") == true);
    assert(intake.run_risk_scan("delete the old project") == false);
}

static void test_mutation_requires_approval() {
    ccad::AgentOrchestrator orch;
    register_mock_tools(orch);
    ccad::OrchestratorConfig cfg;
    const auto result = orch.execute_tool("pcb.add-via", "{}", cfg);
    assert(result.find("\"error\":\"approval_required\"") != std::string::npos);
    cfg.approved_tool_name = "pcb.add-via";
    cfg.approved_tool_token = "test-token-1";
    const auto approved = orch.execute_tool("pcb.add-via", "{}", cfg);
    assert(approved.find("\"status\":\"via_added\"") != std::string::npos);
    const auto replay = orch.execute_tool("pcb.add-via", "{}", cfg);
    assert(replay.find("\"error\":\"approval_token_consumed\"") != std::string::npos);
    const auto other = orch.execute_tool("pcb.add-track", "{}", cfg);
    assert(other.find("\"error\":\"approval_required\"") != std::string::npos);
    const auto preview = orch.execute_tool("pcb.add-via", "{\"dry_run\":true,\"x_mm\":10}", ccad::OrchestratorConfig{});
    assert(preview.find("\"status\":\"dry_run\"") != std::string::npos);
    assert(preview.find("approval_required") == std::string::npos);
}

// ─── Main ───────────────────────────────────────────────────────
int main() {
    std::cout << "Agent Orchestrator Tests\n";
    std::cout << "========================\n";

    test_orchestrator_schema();
    test_tool_registry();
    test_plan_add_via();
    test_plan_drc();
    test_plan_review();
    test_plan_generic();
    test_plan_kicad_parity();
    test_execute_drc_goal();
    test_execute_review_goal();
    test_dry_run();
    test_cancel_goal();
    test_goal_json();
    test_unregistered_tool_dispatch();
    test_project_context_json();
    test_intake_layer();
    test_mutation_requires_approval();

    std::cout << "\nAll 14 orchestrator tests passed!\n";
    return 0;
}
