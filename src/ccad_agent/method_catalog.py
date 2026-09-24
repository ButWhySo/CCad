def orchestrator_method_catalog():
    """Describe only the child-process JSON-RPC controls and their safety."""
    catalog = {
        "schema_version": 1,
        "methods": [
            {"name": "agent.methods", "read_only": True},
            {"name": "agent.context_state", "read_only": True, "secrets": False,
             "params": {"thread_id": {"type": "string", "optional": True,
                                         "description": "Opaque session identity"}},
             "response": {"method": "context_state_snapshot", "fields": [
                 "thread_id", "revision", "content_emitted", "secret_value_visible"]}},
            {"name": "agent.memory_state", "read_only": True, "secrets": False,
             "params": {"tier": {"type": "string", "optional": True,
                                    "enum": ["stm", "ltm", "episodic"]}},
             "response": {"method": "memory_state", "fields": [
                 "tiers", "secret_value_visible"]}},
            {"name": "agent.memory_set_enabled", "read_only": False, "secrets": False,
             "approval_required": False,
             "side_effect": "persist_memory_preference_and_update_runtime_cache",
             "params": {"tier": {"type": "string", "enum": ["stm", "ltm", "episodic"]},
                        "enabled": {"type": "boolean"}},
             "response": {"method": "memory_state", "fields": [
                 "tier", "enabled", "runtime_entries", "persistent_entries",
                 "loaded_into_process", "secret_value_visible"]}},
            {"name": "agent.memory_reset", "read_only": False, "secrets": False,
             "approval_required": True,
             "params": {"tier": {"type": "string", "optional": True,
                                    "enum": ["stm", "ltm", "episodic"]}},
             "response": {"method": "memory_reset", "fields": [
                 "tier", "removed", "secret_value_visible"]}},
            {"name": "agent.memory_list", "read_only": True, "secrets": False,
             "params": {"tier": {"type": "string", "optional": True,
                                   "enum": ["stm", "ltm", "episodic"]},
                        "scope": {"type": "string", "optional": True}},
             "response": {"method": "memory_state", "fields": [
                 "entries", "tier", "secret_value_visible"]}},
            {"name": "agent.memory_add", "read_only": False, "secrets": True,
             "side_effect": "persist_enabled_memory_tier",
             "params": {"tier": {"type": "string", "enum": ["stm", "ltm", "episodic"]},
                        "scope": {"type": "string"}, "title": {"type": "string"},
                        "content": {"type": "string", "secret_rejected": True}},
             "response": {"method": "memory_added", "fields": [
                 "id", "tier", "scope", "secret_value_visible"]}},
            {"name": "agent.memory_update", "read_only": False, "secrets": True,
             "side_effect": "update_owned_memory_record",
             "params": {"id": {"type": "string"},
                        "content": {"type": "string", "secret_rejected": True}},
             "response": {"method": "memory_updated", "fields": [
                 "id", "updated", "secret_value_visible"]}},
            {"name": "agent.memory_delete", "read_only": False, "secrets": False,
             "approval_required": True,
             "side_effect": "delete_owned_memory_record",
             "params": {"id": {"type": "string"},
                        "confirmed": {"type": "boolean", "required": True}},
             "response": {"method": "memory_deleted", "fields": [
                 "removed", "secret_value_visible"]}},
            {"name": "agent.pending_calls", "read_only": True, "secrets": False,
             "params": {"thread_id": {"type": "string", "optional": True,
                                         "description": "Opaque session identity"}},
             "response": {"method": "pending_calls_state", "fields": [
                "thread_id", "process_call_ids", "checkpoint_call_ids", "count",
                 "approval_required", "approval_reason", "secret_value_visible"]}},
            {"name": "agent.list_models", "read_only": True,
             "network_access": "provider_specific",
             "network_access_by_provider": {"openai": "explicit_refresh", "anthropic": "explicit_refresh",
                                              "google_gemini": "explicit_refresh", "openrouter": "explicit_refresh",
                                              "cerebras": "explicit_refresh", "ollama": "explicit_local_refresh"},
             "provider_normalization": "trim_lowercase",
             "providers": ["openai", "anthropic", "google_gemini", "openrouter", "cerebras", "ollama"],
             "params": {"provider": {"type": "string", "enum": ["openai", "anthropic", "google_gemini", "openrouter", "cerebras", "ollama"],
                                        "default": "openai"}},
             "response": {"method": "provider_models", "fields": [
                 "provider", "ok", "error", "error_detail", "models",
                 "count", "network_access", "source", "source_kind", "source_url"],
                 "model_fields": ["id", "name", "display_name", "owned_by",
                                  "context_length", "architecture", "capabilities",
                                  "supported_parameters", "supported_generation_methods"]}},
            {"name": "agent.set_thread_id", "read_only": False,
             "secrets": False,
             "params": {"thread_id": {"type": "string", "optional": False,
                                         "description": "Opaque session identity"}},
             "response": {"method": "thread_state", "fields": [
                 "configured", "secret_value_visible"]}},
            {"name": "agent.resume_thread", "read_only": False,
             "secrets": False,
             "params": {"resume": {"type": "object", "optional": True}},
             "responses": ["thread_state", "thread_resumed"],
              "response": {"method": "thread_state", "fields": [
                  "resumable", "reason", "thread_id", "next", "checkpoint_id"]},
             "response_contracts": {"thread_resumed": {"fields": [
                 "thread_id", "next", "message_count"]}}},
            {"name": "agent.set_provider_secret", "read_only": False,
             "secrets": True, "approval_required": True,
             "params": {"provider": {"type": "string"},
                         "secret": {"type": "string", "secret": True}},
             "response": {"method": "provider_secret_result", "fields": [
                 "provider", "configured", "execution_enabled",
                 "error", "error_category", "network_access",
                 "secret_value_visible"]}},
            {"name": "agent.test_provider", "read_only": False,
            "secrets": True, "side_effect": "transient_provider_probe",
             "params": {"provider": {"type": "string"},
                         "model": {"type": "string", "optional": True},
                         "secret": {"type": "string", "optional": True, "secret": True}},
             "responses": ["provider_test_result", "provider_state", "backend_state", "message"],
             "response": {"method": "provider_test_result", "fields": [
                 "provider", "model", "configured", "execution_enabled",
                 "network_access", "error", "error_category",
                 "secret_value_visible"]}},
            {"name": "agent.test_provider_connection", "read_only": False,
             "secrets": True, "approval_required": True,
             "side_effect": "one_live_provider_request",
             "params": {"provider": {"type": "string"},
                        "model": {"type": "string", "optional": True},
                        "secret": {"type": "string", "optional": True, "secret": True}},
             "response": {"method": "provider_connection_result", "fields": [
                 "provider", "model", "connected", "response_preview",
                 "error_category", "http_status", "network_access",
                 "tool_executed", "request_count", "secret_value_visible"]}},
            {"name": "agent.get_marketplace_catalog", "read_only": True,
             "network_access": "none", "secrets": False,
             "response": {"method": "marketplace_catalog", "fields": [
                 "plugins", "workflows"]}},
            {"name": "agent.get_config", "read_only": True, "secrets": False,
             "response": {"method": "config_state", "fields": [
                 "theme", "grid", "autosave", "restore_session", "provider", "model",
                 "sandbox_mode", "approval_policy", "project_name", "project_path",
                 "trust_level", "memory", "hooks", "personalisation", "mcp_servers",
                 "plugins", "workflows"]}},
            {"name": "agent.langfuse_status", "read_only": True, "secrets": False,
             "response": {"method": "observability_state", "fields": [
                 "configured", "enabled", "exporter_initialized", "backend",
                 "last_test", "reason", "error_type", "secret_value_visible"]}},
            {"name": "agent.langfuse_set_secret", "read_only": False,
             "secrets": True, "approval_required": True,
             "params": {"public_key": {"type": "string", "secret": True},
                        "secret_key": {"type": "string", "secret": True}},
             "response": {"method": "observability_state", "fields": [
                 "configured", "enabled", "exporter_initialized", "backend",
                 "last_test", "reason", "error_type", "secret_value_visible"]}},
            {"name": "agent.langfuse_test", "read_only": False, "secrets": False,
             "side_effect": "bounded_observability_export",
             "response": {"method": "observability_state", "fields": [
                 "configured", "enabled", "exporter_initialized", "backend",
                 "last_test", "reason", "error_type", "secret_value_visible"]}},
            {"name": "agent.set_config", "read_only": False, "secrets": False,
             "side_effect": "persist_non_secret_preferences",
             "params": {"config": {"type": "object", "optional": False}},
             "response": {"method": "message", "fields": [
                 "text", "secret_value_visible"]}},
            {"name": "agent.langfuse_set_config", "read_only": False, "secrets": False,
             "side_effect": "persist_non_secret_preferences",
             "deprecated_alias_for": "agent.set_config"},
            {"name": "agent.observability_status", "read_only": True, "secrets": False,
             "deprecated_alias_for": "agent.langfuse_status"},
            {"name": "agent.set_observability_secret", "read_only": False,
             "secrets": True, "deprecated_alias_for": "agent.langfuse_set_secret"},
            {"name": "agent.test_export", "read_only": False, "secrets": False,
             "deprecated_alias_for": "agent.langfuse_test"},
            {"name": "agent.set_tool_catalog", "read_only": False,
             "secrets": False, "side_effect": "runtime_tool_registration",
             "params": {"catalog": {"type": "array", "optional": False}},
             "response": {"method": "tool_catalog_state", "fields": [
                 "accepted", "method_count", "tool_count", "error",
                 "secret_value_visible"]}},
            {"name": "agent.mcp_status", "read_only": True, "network_access": "none",
             "secrets": False, "response": {"method": "mcp_status", "fields": [
                 "servers", "configured", "runtime", "process_execution"]}},
            {"name": "agent.mcp_plan", "read_only": True, "network_access": "none",
             "secrets": False, "response": {"method": "mcp_plan", "fields": [
                 "servers", "launch_allowed", "reason"]}},
            {"name": "agent.activate_provider", "read_only": False,
             "network_access": "adapter_initialization_only", "secrets": False,
             "params": {"provider": {"type": "string"},
                        "model": {"type": "string", "optional": True}},
             "response": {"method": "provider_activation_result", "fields": [
                 "provider", "model", "initialized", "secret_value_visible"]}},
            {"name": "agent.generate_component", "read_only": False,
             "provider_call": True, "secrets": False,
             "params": {"prompt": {"type": "string"},
                         "type": {"type": "string", "optional": True},
                         "package": {"type": "string", "optional": True}},
             "responses": ["generated_component", "component_generation_failed", "message"],
             "response": {"method": "generated_component", "fields": [
                 "pins", "name"]},
             "response_contracts": {"component_generation_failed": {"fields": [
                 "category", "message"]}}},
            {"name": "agent.cancel_tool", "read_only": False,
             "approval_required": False, "side_effect": "cancel_wait_only"},
            {"name": "tool_result", "read_only": False,
             "approval_required": True, "side_effect": "client_authorized_result"},
            {"name": "human_message", "read_only": False, "provider_call": True,
             "preflight": "intake_guard",
             "params": {"thread_id": {"type": "string", "optional": True,
                                         "description": "Opaque session identity"}},
             "responses": ["intake_state", "context_state", "provider_state",
                           "backend_state", "message", "tool_call",
                           "tool_canceled"],
             "response_contracts": {
                 "intake_state": {"fields": [
                     "accepted", "category", "secret_value_visible"]},
                 "provider_state": {"fields": [
                     "provider", "model", "configured", "execution_enabled",
                     "network_access", "error", "error_category",
                     "secret_value_visible"]},
                 "backend_state": {"fields": [
                     "runtime", "ready", "provider_initialized",
                     "network_access", "secret_value_visible"]},
                 "message": {"fields": [
                     "text", "kind", "category", "secret_value_visible"]},
                "tool_call": {"fields": [
                    "tool", "args", "call_id", "approval_required",
                    "approval_reason"]},
                 "tool_canceled": {"fields": [
                     "call_id", "reason"]},
                 "context_state": {"fields": [
                     "thread_id", "revision", "previous_revision", "changed", "change_kind",
                     "content_present", "content_size", "original_content_size",
                     "context_limit", "truncated", "content_emitted", "sources",
                     "memory_content_emitted", "memory_entry_count",
                     "history_message_count", "context_schema_version"]}},
             "secret_value_visible": False},
        ],
        "secret_value_visible": False,
    }
    methods = catalog["methods"]
    # This is a control-plane catalog, not the model-callable tool list. Keep
    # transport and dispatchability explicit so clients cannot infer otherwise.
    for method in methods:
        method.setdefault("transport", "python_json_rpc")
        method.setdefault("dispatchable", True)
        method["agent_tool_callable"] = False
        method.setdefault("secrets", False)
    return catalog
