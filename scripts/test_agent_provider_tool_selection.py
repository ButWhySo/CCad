"""Behavior tests for reducing explicitly requested provider tool schemas."""

import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch


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

    def test_explicit_context_dependent_method_keeps_project_payload(self):
        messages = [orchestrator.HumanMessage(content=(
            "Call project.context and report active layer."))]
        tools = orchestrator.provider_tools_for_messages(messages)
        self.assertEqual(orchestrator.provider_context_for_tools(
            tools, "current typed board context"), "current typed board context")

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


if __name__ == "__main__":
    unittest.main()
