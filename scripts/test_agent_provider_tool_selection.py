"""Behavior tests for reducing explicitly requested provider tool schemas."""

import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from typing import cast
from unittest.mock import patch
from pydantic import BaseModel, ValidationError


AGENT_DIR = Path(__file__).resolve().parents[1] / "src" / "ccad_agent"
sys.path.insert(0, str(AGENT_DIR))
import orchestrator  # noqa: E402


class ProviderToolSelectionTests(unittest.TestCase):
    def setUp(self):
        self.old_tools = orchestrator.agent_tools
        self.old_catalog = orchestrator.native_tool_catalog
        orchestrator.native_tool_catalog = [
            {"method": "project.context", "context_requirements": ["project_state"]},
            {"method": "project.object_counts", "context_requirements": ["project_state"]},
            {"method": "project.ping", "context_requirements": []},
            {"method": "ui.add_zone", "context_requirements": [
                "project_state", "project_revision"]},
        ]
        orchestrator.agent_tools = [
            SimpleNamespace(name="ccad_project_context"),
            SimpleNamespace(name="ccad_project_object_counts"),
            SimpleNamespace(name="ccad_project_ping"),
            SimpleNamespace(name="ccad_ui_add_zone"),
            SimpleNamespace(name="ccad_search_memory"),
        ]

    def tearDown(self):
        orchestrator.agent_tools = self.old_tools
        orchestrator.native_tool_catalog = self.old_catalog

    def test_explicit_native_method_names_select_exact_schemas(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.object_counts and project.context."))]
        selected = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual([tool.name for tool in selected], [
            "ccad_project_context", "ccad_project_object_counts"])

    def test_partial_method_name_does_not_select_a_schema(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.object_counts_extra."))]
        selected = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(selected, orchestrator.agent_tools)

    def test_natural_language_turn_keeps_complete_catalog(self):
        messages = [orchestrator.HumanMessage(content="Inspect this board.")]
        selected = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(selected, orchestrator.agent_tools)

    def test_only_latest_user_turn_controls_explicit_schema_selection(self):
        messages = [
            orchestrator.HumanMessage(content="Call project.context."),
            orchestrator.AIMessage(content="Previous answer."),
            orchestrator.HumanMessage(content="Summarize the last result."),
        ]
        selected = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(selected, orchestrator.agent_tools)

    def test_explicit_context_free_method_omits_project_payload(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.ping and report the result."))]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(orchestrator.provider_context_for_tools(
            tools, "private board snapshot"), "")

    def test_object_counts_keeps_required_project_context(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.object_counts and report the result."))]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual([tool.name for tool in tools], [
            "ccad_project_object_counts"])
        self.assertEqual(orchestrator.provider_context_for_tools(
            tools, "typed board snapshot"), "typed board snapshot")

    def test_plain_language_object_count_request_selects_matching_method(self):
        messages = [orchestrator.HumanMessage(content=(
            "What are the footprint, track, and via counts on this board?"))]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual([tool.name for tool in tools], [
            "ccad_project_object_counts"])

    def test_completed_tool_result_disables_repeated_intent_call(self):
        messages = [
            orchestrator.HumanMessage(content=(
                "What are the footprint, track, and via counts on this board?")),
            orchestrator.AIMessage(content="", tool_calls=[{
                "name": "ccad_project_object_counts", "args": {}, "id": "call-1"}]),
            orchestrator.ToolMessage(content=(
                '{"footprints": 1, "tracks": 2, "vias": 1}'),
                tool_call_id="call-1", name="ccad_project_object_counts"),
        ]
        self.assertEqual(orchestrator.provider_tools_for_messages(messages), [])
        prompt = orchestrator.get_system_prompt(
            "the CCad PCB Routing Expert.", [])
        self.assertIn("already returned in this turn", prompt)
        self.assertIn("Do not call any more tools", prompt)

    def test_completed_tool_call_identity_suppresses_adapter_without_tool_name(self):
        messages = [
            orchestrator.HumanMessage(content=(
                "What are the footprint, track, and via counts on this board?")),
            orchestrator.AIMessage(content="", tool_calls=[{
                "name": "ccad_project_object_counts", "args": {}, "id": "call-1"}]),
        ]
        self.assertEqual(orchestrator.provider_tools_for_messages(messages), [])

    def test_completed_tool_result_keeps_other_explicit_tool_available(self):
        messages = [
            orchestrator.HumanMessage(content=(
                "Call project.object_counts and project.context.")),
            orchestrator.AIMessage(content="", tool_calls=[{
                "name": "ccad_project_object_counts", "args": {}, "id": "call-1"}]),
            orchestrator.ToolMessage(content=(
                '{"footprints": 1, "tracks": 2, "vias": 1}'),
                tool_call_id="call-1", name="ccad_project_object_counts"),
        ]
        self.assertEqual([tool.name for tool in
                          orchestrator.provider_tools_for_messages(messages)], [
                              "ccad_project_context"])

    def test_object_count_intent_requires_all_count_terms(self):
        messages = [orchestrator.HumanMessage(content=(
            "Count tracks and vias on this board."))]
        self.assertIs(orchestrator.provider_tools_for_messages(messages),
                      orchestrator.agent_tools)

    def test_explicit_context_dependent_method_keeps_project_payload(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.context and report active layer."))]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(orchestrator.provider_context_for_tools(
            tools, "current typed board context"), "current typed board context")

    def test_narrowed_tool_binding_resolves_unbound_project_state_instruction(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.object_counts and report the result."))]
        tools = orchestrator.provider_tools_for_messages(messages)
        instruction = orchestrator.provider_tool_scope_instruction(tools)
        self.assertIn("`project.object_counts`", instruction)
        self.assertIn("call its exact bound tool now", instruction)
        self.assertIn("Do not call `project.state` or any unbound method", instruction)
        self.assertIn("without inventing a call", instruction)
        self.assertIn("empty JSON object {}", instruction)
        self.assertIn("ccad_project_object_counts", instruction)

    def test_narrowed_system_prompt_does_not_conflict_with_bound_methods(self):
        messages = [orchestrator.HumanMessage(content=(
            "What are the footprint, track, and via counts on this board?"))]
        tools = orchestrator.provider_tools_for_messages(messages)
        prompt = orchestrator.get_system_prompt(
            "the CCad PCB Routing Expert.", tools)
        self.assertIn("Never invent a tool", prompt)
        self.assertIn("project.object_counts", prompt)
        self.assertIn("do not request or refer to any unbound method", prompt)
        self.assertNotIn("Use project.state for complete live", prompt)

    def test_full_catalog_system_prompt_retains_project_state_guidance(self):
        messages = [orchestrator.HumanMessage(content="Inspect this board.")]
        tools = orchestrator.provider_tools_for_messages(messages)
        prompt = orchestrator.get_system_prompt(
            "the CCad PCB Routing Expert.", tools)
        self.assertIn("Use project.state for complete live", prompt)
        self.assertIn("inspect ui.active_layer or ui.active_net", prompt)
        self.assertIn("Never infer an ID from a display name", prompt)

    def test_read_only_tool_calls_do_not_report_approval_pending(self):
        self.assertFalse(orchestrator.tool_calls_require_approval([{
            "name": "ccad_project_object_counts", "args": {}, "id": "call-ro"}]))
        self.assertTrue(orchestrator.tool_calls_require_approval([{
            "name": "ccad_ui_add_zone", "args": {}, "id": "call-write"}]))

    def test_full_catalog_does_not_add_turn_specific_tool_scope_instruction(self):
        messages = [orchestrator.HumanMessage(content="Inspect this board.")]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertIs(tools, orchestrator.agent_tools)
        self.assertEqual(orchestrator.provider_tool_scope_instruction(tools), "")

    def test_provider_schema_forbids_undeclared_arguments_for_zero_arg_method(self):
        tool = orchestrator.build_native_tools([{
            "method": "project.object_counts",
            "description": "Return project and board object counts.",
            "inputSchema": {"type": "object", "properties": {},
                            "additionalProperties": False},
            "read_only": True,
            "context_requirements": ["project_state"],
        }])[0]
        schema = orchestrator.provider_safe_tool_declarations([tool])[0]["function"]["parameters"]
        self.assertIs(schema.get("additionalProperties"), False)
        args_schema = cast(type[BaseModel], tool.args_schema)
        with self.assertRaises(ValidationError):
            args_schema.model_validate({"unexpected": 1})

    def test_catalog_rejects_unbounded_object_arguments(self):
        with self.assertRaisesRegex(ValueError, "open-ended native tool"):
            orchestrator.validate_native_tool_catalog([{
            "method": "project.extension",
            "description": "Accept extension arguments.",
            "inputSchema": {"type": "object", "properties": {},
                            "additionalProperties": True},
            "read_only": True,
            "context_requirements": [],
        }])


class ProviderResponseContractTests(unittest.TestCase):
    def test_empty_text_response_is_not_a_valid_turn(self):
        response = orchestrator.AIMessage(content="  ")
        self.assertFalse(orchestrator.provider_response_has_payload(response))

    def test_empty_text_blocks_are_not_a_valid_turn(self):
        response = orchestrator.AIMessage(content=[{"type": "text", "text": " "}])
        self.assertFalse(orchestrator.provider_response_has_payload(response))

    def test_tool_call_without_text_is_a_valid_response(self):
        response = orchestrator.AIMessage(
            content="", tool_calls=[{"name": "ccad_project_object_counts",
                                     "args": {}, "id": "call-1"}])
        self.assertTrue(orchestrator.provider_response_has_payload(response))

    def test_nonempty_text_is_a_valid_response(self):
        response = orchestrator.AIMessage(content="Board has 4 tracks.")
        self.assertTrue(orchestrator.provider_response_has_payload(response))

    def test_empty_provider_response_gets_distinct_safe_failure(self):
        error = orchestrator.ProviderEmptyResponseError("empty_response")
        self.assertEqual(orchestrator.classify_provider_error(error), "empty_response")
        message = orchestrator.provider_error_user_message(error)
        self.assertIn("response was received", message)
        self.assertIn("empty_response", message)
        self.assertNotIn("stopped before a response", message)

    def test_natural_language_turn_keeps_full_project_context(self):
        messages = [orchestrator.HumanMessage(content="Inspect this board.")]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(orchestrator.provider_context_for_tools(
            tools, "current typed board context"), "current typed board context")


class ProviderRuntimeActivationTests(unittest.TestCase):
    def test_activation_rebuilds_provider_and_publishes_safe_readiness(self):
        old_initialized = orchestrator.provider_initialized
        events = []
        try:
            with (patch.object(orchestrator, "init_provider", return_value=True) as init,
                  patch.object(orchestrator, "active_provider_model",
                               return_value=("ollama", "qwen2.5:3b")),
                  patch.object(orchestrator, "emit", side_effect=events.append)):
                self.assertTrue(orchestrator.activate_provider_runtime())
            init.assert_called_once_with()
            self.assertTrue(orchestrator.provider_initialized)
            self.assertEqual(len(events), 1)
            self.assertEqual(events[0]["method"], "backend_state")
            self.assertEqual(events[0]["params"]["provider"], "ollama")
            self.assertEqual(events[0]["params"]["model"], "qwen2.5:3b")
            self.assertTrue(events[0]["params"]["provider_initialized"])
            self.assertFalse(events[0]["params"]["secret_value_visible"])
        finally:
            orchestrator.provider_initialized = old_initialized


class CheckpointToolReplayTests(unittest.TestCase):
    def test_checkpoint_state_distinguishes_running_result_and_approval(self):
        self.assertEqual(orchestrator.checkpoint_run_state(
            SimpleNamespace(next=(), tasks=())), "completed")
        result_pending = SimpleNamespace(next=("router",), tasks=())
        self.assertEqual(orchestrator.checkpoint_run_state(result_pending),
                         "awaiting_tool_result")
        approval_pending = SimpleNamespace(next=("execute_tool",), tasks=[
            SimpleNamespace(interrupts=[SimpleNamespace(value={
                "approval_required": True})])])
        self.assertEqual(orchestrator.checkpoint_run_state(approval_pending),
                         "awaiting_tool_approval")

    def test_checkpoint_replay_emits_one_broker_request_per_call_id(self):
        old_saver = orchestrator.checkpoint_saver
        old_wait_enabled = orchestrator.broker_wait_enabled
        old_events = set(orchestrator.checkpoint_tool_events)
        emitted = []
        try:
            orchestrator.checkpoint_saver = object()
            orchestrator.broker_wait_enabled = True
            orchestrator.checkpoint_tool_events.clear()
            with (patch.object(orchestrator, "dispatch_checkpointed_tool",
                               return_value=('{}', {})) as dispatch,
                  patch.object(orchestrator, "emit", side_effect=emitted.append)):
                first = orchestrator.dispatch_client_tool(
                    "project.object_counts", {})
                second = orchestrator.dispatch_client_tool(
                    "project.object_counts", {})
            self.assertEqual(first, "{}")
            self.assertEqual(second, "{}")
            self.assertEqual(dispatch.call_count, 2)
            self.assertEqual(len(emitted), 1)
            self.assertEqual(emitted[0]["params"]["call_id"],
                             orchestrator.checkpoint_tool_call_id(
                                 "project.object_counts", {}))
        finally:
            orchestrator.checkpoint_saver = old_saver
            orchestrator.broker_wait_enabled = old_wait_enabled
            orchestrator.checkpoint_tool_events.clear()
            orchestrator.checkpoint_tool_events.update(old_events)


if __name__ == "__main__":
    unittest.main()
