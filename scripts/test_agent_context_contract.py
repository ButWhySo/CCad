"""No-network contract checks for bounded agent conversation context."""

from pathlib import Path
import json
import os
import subprocess
import sys


SOURCE = Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py"
ROOT = SOURCE.parents[2]
CATALOG = SOURCE.with_name("method_catalog.py")
text = SOURCE.read_text(encoding="utf-8") + CATALOG.read_text(encoding="utf-8")

assert 'CCAD_AGENT_HISTORY_LIMIT' in text
assert 'def bound_session_history(messages):' in text
assert 'min(64, max(4, limit))' in text
assert 'session_messages = bound_session_history(final_state["messages"])' in text
assert 'memory_content_emitted' in text
assert 'from context_package import (build_context_package' in text
assert 'package = build_context_package(' in text
assert 'memory_entry_count' in text
assert 'history_message_count' in text
assert 'context_schema_version' in text
assert 'memory_manifest' in text
assert 'turn_context_version' in text
assert 'context_cache_hit' in text
assert 'context.assemble' in text
assert 'memory.retrieve' in text
assert '"previous_revision": previous_context_revision' in text
assert '"change_kind": context_change_kind' in text
assert '"response_contracts": {' in text
assert '"intake_state": {"fields": [' in text
human_start = text.index("def handle_human_message(req):")
human_handler = text[human_start:]
assert human_handler.index("telemetry_runtime.start_agent_turn(") < human_handler.index(
    '"context.assemble"')
invoke_start = text.index("def invoke_agent_run(state):")
invoke_end = text.index("\ndef get_system_prompt", invoke_start)
assert "telemetry_runtime.begin_turn()" not in text[invoke_start:invoke_end]
assert "telemetry_runtime.finish_agent_turn()" in text

env = os.environ.copy()
env.update({"CCAD_AGENT_DEFER_PROVIDER_INIT": "1", "PYTHONNOUSERSITE": "1",
            "PYTHONPATH": str(SOURCE.parent)})
requests = [{"method": "agent.methods", "params": {}}]
requests.append({"method": "agent.context_state", "params": {}})
requests.append({"method": "agent.pending_calls", "params": {}})
requests.extend({"method": "human_message", "params": {
    "text": "summarize", "context": context, "thread_id": thread_id}}
    for context, thread_id in (("board A", "thread-a"),
                               ("board B", "thread-b"),
                               ("board B", "thread-b")))
requests.append({"method": "human_message", "params": {
    "text": "/clear", "context": "board B"}})
requests.append({"method": "agent.context_state", "params": {}})
geometry_project = json.loads((ROOT / "artifacts" / "demos" /
                               "sprint160-placement-crash-ci-final.ccad.json").read_text(
                                   encoding="utf-8"))
geometry_project["board"].setdefault("footprints", []).extend((
    {"reference": "JAC1", "value": "AC input",
     "footprint_name": "Connector_PinHeader_2.54mm", "layer_id": "F.Cu",
     "position": {"x_nm": 8_000_000, "y_nm": 17_000_000}},
    {"reference": "C_NEAR", "value": "100 nF", "footprint_name": "C_0402",
     "layer_id": "F.Cu", "position": {"x_nm": 10_000_000, "y_nm": 17_000_000}},
))
geometry_project["board"].setdefault("placement_regions", []).append({
    "id": "PR_SPRINT997", "kind": "placement",
    "area": {"x_nm": 7_000_000, "y_nm": 16_000_000,
             "width_nm": 5_000_000, "height_nm": 2_000_000},
})
requests.append({"method": "human_message", "params": {
    "text": "Which PCB footprints are within 5 mm of JAC1, and which PCB objects "
            "intersect placement region PR_SPRINT997?",
    "context": json.dumps({"typed_state": {"available": True,
                                              "project": geometry_project}}),
    "thread_id": "thread-geometry-relations"}})
payload = "\n".join(json.dumps(request) for request in requests) + "\n"
run = subprocess.run([sys.executable, str(SOURCE)], input=payload, text=True,
                     capture_output=True, env=env, check=False)
assert run.returncode == 0, run.stderr
lines = [json.loads(line) for line in run.stdout.splitlines()
         if line.strip().startswith("{")]
methods = next(item["params"] for item in lines
               if item.get("method") == "agent_methods")
human_contract = next(item for item in methods["methods"]
                      if item["name"] == "human_message")
assert human_contract["params"]["thread_id"]["optional"] is True
assert human_contract["params"]["thread_id"]["type"] == "string"
pending_contract = next(item for item in methods["methods"]
                        if item["name"] == "agent.pending_calls")
assert pending_contract["params"]["thread_id"]["optional"] is True
assert pending_contract["params"]["thread_id"]["type"] == "string"
assert pending_contract["response"]["method"] == "pending_calls_state"
context_contract = next(item for item in methods["methods"]
                        if item["name"] == "agent.context_state")
assert context_contract["params"]["thread_id"]["optional"] is True
assert context_contract["params"]["thread_id"]["type"] == "string"
set_thread_contract = next(item for item in methods["methods"]
                           if item["name"] == "agent.set_thread_id")
assert set_thread_contract["params"]["thread_id"]["optional"] is False
resume_contract = next(item for item in methods["methods"]
                       if item["name"] == "agent.resume_thread")
assert resume_contract["params"]["resume"]["type"] == "object"
assert resume_contract["params"]["resume"]["optional"] is True
assert "thread_resumed" in resume_contract["responses"]
assert "checkpoint_id" in resume_contract["response"]["fields"]
assert "message_count" in resume_contract["response_contracts"]["thread_resumed"]["fields"]
secret_contract = next(item for item in methods["methods"]
                       if item["name"] == "agent.set_provider_secret")
assert secret_contract["secrets"] is True
assert secret_contract["params"]["secret"]["secret"] is True
assert "secret_value_visible" in secret_contract["response"]["fields"]
probe_contract = next(item for item in methods["methods"]
                      if item["name"] == "agent.test_provider")
assert probe_contract["secrets"] is True
assert probe_contract["params"]["secret"]["secret"] is True
assert "backend_state" in probe_contract["responses"]
market_contract = next(item for item in methods["methods"]
                       if item["name"] == "agent.get_marketplace_catalog")
assert market_contract["read_only"] is True
assert market_contract["response"]["fields"] == ["plugins", "workflows"]
config_contract = next(item for item in methods["methods"]
                       if item["name"] == "agent.get_config")
assert config_contract["read_only"] is True
assert "provider" in config_contract["response"]["fields"]
assert "mcp_servers" in config_contract["response"]["fields"]
set_config_contract = next(item for item in methods["methods"]
                           if item["name"] == "agent.set_config")
assert set_config_contract["params"]["config"]["type"] == "object"
assert "secret_value_visible" in set_config_contract["response"]["fields"]
assert "secret_key_fragments" in text
assert "rejected_secret_keys" in text
assert "sanitize_persisted_config" in text
assert "isinstance(value, list)" in text
component_contract = next(item for item in methods["methods"]
                          if item["name"] == "agent.generate_component")
assert component_contract["params"]["prompt"]["type"] == "string"
assert "generated_component" in component_contract["responses"]
assert "component_generation_failed" in component_contract["responses"]
assert component_contract["response"]["fields"] == ["pins", "name"]
assert component_contract["response_contracts"]["component_generation_failed"]["fields"] == ["category", "message"]
for field in ("thread_id", "process_call_ids", "checkpoint_call_ids", "count", "approval_required",
              "approval_reason", "secret_value_visible"):
    assert field in pending_contract["response"]["fields"]
assert "context_state" in human_contract["response_contracts"]
assert '"project_retrieval_near_component_count"' in text
assert '"project_retrieval_region_member_count"' in text
assert '"project_retrieval_block_net_count"' in text
assert "provider_state" in human_contract["responses"]
assert "backend_state" in human_contract["responses"]
assert "change_kind" in human_contract["response_contracts"]["context_state"]["fields"]
assert "thread_id" in human_contract["response_contracts"]["context_state"]["fields"]
for field in ("project_retrieval_near_component_count",
              "project_retrieval_region_member_count",
              "project_retrieval_block_net_count", "project_retrieval_stats"):
    assert field in human_contract["response_contracts"]["context_state"]["fields"]
assert "intake_state" in human_contract["response_contracts"]
assert "accepted" in human_contract["response_contracts"]["intake_state"]["fields"]
assert "provider_state" in human_contract["response_contracts"]
provider_fields = human_contract["response_contracts"]["provider_state"]["fields"]
assert "execution_enabled" in provider_fields
assert "error" in provider_fields
assert "error_category" in provider_fields
assert "execution_ready" not in provider_fields
assert "backend_state" in human_contract["response_contracts"]
assert "ready" in human_contract["response_contracts"]["backend_state"]["fields"]
assert "message" in human_contract["response_contracts"]
assert "text" in human_contract["response_contracts"]["message"]["fields"]
assert "tool_canceled" in human_contract["responses"]
assert "reason" in human_contract["response_contracts"]["tool_canceled"]["fields"]
assert "tool_call" in human_contract["response_contracts"]
tool_fields = human_contract["response_contracts"]["tool_call"]["fields"]
assert "tool" in tool_fields
assert "call_id" in tool_fields
assert "name" not in tool_fields
assert "approval_required" in tool_fields
assert "side_effect" not in tool_fields
snapshots = [item for item in lines if item.get("method") == "context_state_snapshot"]
assert snapshots[0]["params"]["thread_id"] == "ccad-local"
assert snapshots[0]["params"]["revision"] == ""
assert snapshots[-1]["params"]["revision"] == ""
assert snapshots[-1]["params"]["content_emitted"] is False
assert snapshots[-1]["params"]["secret_value_visible"] is False
pending_snapshot = next(item for item in lines
                        if item.get("method") == "pending_calls_state")
assert pending_snapshot["params"]["count"] == 0
assert pending_snapshot["params"]["thread_id"] == "ccad-local"
assert pending_snapshot["params"]["approval_required"] is False
assert pending_snapshot["params"]["approval_reason"] == ""
events = [item for item in lines if item.get("method") == "context_state"]
assert len(events) == 5, run.stdout + run.stderr
assert events[0]["params"]["previous_revision"] == ""
assert events[0]["params"]["change_kind"] == "initial"
assert events[0]["params"]["thread_id"] == "thread-a"
assert events[1]["params"]["previous_revision"] == ""
assert events[1]["params"]["change_kind"] == "initial"
assert events[1]["params"]["thread_id"] == "thread-b"
assert events[2]["params"]["previous_revision"] == events[1]["params"]["revision"]
assert events[2]["params"]["change_kind"] == "unchanged"
assert events[2]["params"]["thread_id"] == "thread-b"
for event in events[:-1]:
    params = event["params"]
    assert params["context_schema_version"] == 3
    assert params["content_emitted"] is False
    assert params["memory_content_emitted"] is False
    assert params["memory_manifest"]["contents_included"] is False
    assert params["turn_context_version"] >= 1
    assert len(params["turn_context_signal_digest"]) == 24
    assert params["memory_token_budget"] >= 64
    assert params["history_message_count"] >= 0
    assert params["project_retrieval_near_component_count"] == 0
    assert params["project_retrieval_region_member_count"] == 0
    assert params["project_retrieval_block_net_count"] == 0
    assert params["project_retrieval_stats"]["near_component_match_count"] == 0
    assert params["project_retrieval_stats"]["region_member_match_count"] == 0
    assert "project_snapshot" in params["sources"]
geometry_event = events[-1]["params"]
assert geometry_event["thread_id"] == "thread-geometry-relations"
assert geometry_event["project_retrieval_near_component_count"] == 1
assert geometry_event["project_retrieval_region_member_count"] > 0
assert geometry_event["project_retrieval_block_net_count"] == 0
assert geometry_event["project_retrieval_stats"]["near_component_match_count"] == 1
assert geometry_event["project_retrieval_stats"]["region_member_match_count"] > 0
print("PASS agent context history boundary contract; no network")
