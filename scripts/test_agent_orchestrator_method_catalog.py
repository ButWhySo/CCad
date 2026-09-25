"""Contract tests for Python child-process JSON-RPC discovery metadata."""

import importlib.util
import unittest
from pathlib import Path


MODULE = Path(__file__).resolve().parents[1] / "src" / "ccad_agent" / "method_catalog.py"
SPEC = importlib.util.spec_from_file_location("ccad_agent_method_catalog", MODULE)
assert SPEC is not None and SPEC.loader is not None
CATALOG_MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CATALOG_MODULE)


class OrchestratorMethodCatalogTests(unittest.TestCase):
    def setUp(self):
        self.catalog = CATALOG_MODULE.orchestrator_method_catalog()
        self.methods = self.catalog["methods"]
        self.by_name = {entry["name"]: entry for entry in self.methods}

    def test_method_names_are_unique_and_control_plane_is_not_agent_tools(self):
        names = [entry["name"] for entry in self.methods]
        self.assertEqual(len(names), len(set(names)))
        self.assertIn("agent.set_config", self.by_name)
        self.assertEqual(self.by_name["agent.set_config"]["dispatchable"], True)
        for method in self.methods:
            self.assertEqual(method["transport"], "python_json_rpc")
            self.assertFalse(method["agent_tool_callable"])

    def test_memory_runtime_contracts_are_typed_and_safety_distinguished(self):
        state = self.by_name["agent.memory_state"]
        toggle = self.by_name["agent.memory_set_enabled"]
        reset = self.by_name["agent.memory_reset"]
        self.assertEqual(state["params"]["tier"]["type"], "string")
        self.assertIn("semantic", state["response"]["tier_fields"])
        self.assertIn("model_version", state["response"]["semantic_fields"])
        self.assertEqual(toggle["params"]["enabled"]["type"], "boolean")
        self.assertEqual(toggle["read_only"], False)
        self.assertFalse(toggle["approval_required"])
        self.assertTrue(reset["approval_required"])
        self.assertIn("runtime_entries", toggle["response"]["fields"])
        self.assertIn("persistent_entries", toggle["response"]["fields"])
        self.assertTrue(self.by_name["agent.memory_delete"]["approval_required"])
        self.assertTrue(self.by_name["agent.memory_list"]["read_only"])
        self.assertTrue(self.by_name["agent.memory_add"]["params"]["content"]["secret_rejected"])
        self.assertEqual(self.by_name["agent.memory_add"]["params"]["kind"]["enum"],
                         ["fact", "preference", "correction"])
        self.assertIn("kind", self.by_name["agent.memory_add"]["response"]["fields"])
        self.assertEqual(self.by_name["agent.memory_update"]["params"]["kind"]["enum"],
                         ["fact", "preference", "correction"])
        self.assertIn("kind", self.by_name["agent.memory_update"]["response"]["fields"])

    def test_catalog_never_discloses_secret_values(self):
        self.assertFalse(self.catalog["secret_value_visible"])
        for entry in self.methods:
            self.assertFalse(entry.get("secret_value_visible", False))
            for parameter in entry.get("params", {}).values():
                if parameter.get("secret"):
                    self.assertEqual(parameter["type"], "string")


if __name__ == "__main__":
    unittest.main()
