#include "agent_observability_config.hpp"

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace ccad_cli {
namespace {

struct EnvVarSpec {
  std::string name;
  std::string purpose;
  bool secret;
};

bool envPresent(const std::string& name) {
  const char* value = std::getenv(name.c_str());
  return value != nullptr && value[0] != '\0';
}

void appendEscaped(std::ostringstream& out, const std::string& value) {
  out << '"';
  for (const char ch : value) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << ch;
        break;
    }
  }
  out << '"';
}

void appendStringArray(std::ostringstream& out, const std::vector<std::string>& values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) out << ',';
    appendEscaped(out, values[i]);
  }
  out << ']';
}

const std::vector<std::string>& traceSpanPlan() {
  static const std::vector<std::string> spans = {
      "agent.run",       "prompt.assembly",     "model.call",
      "tool.call",       "gui.map_query",       "gui.screenshot_capture",
      "project.drc",     "project.erc",         "file.write",
      "retry",           "interrupt",           "final_verification"};
  return spans;
}

const std::vector<EnvVarSpec>& traceEnvVars() {
  static const std::vector<EnvVarSpec> vars = {
      {"OTEL_EXPORTER_OTLP_ENDPOINT", "OTLP collector or backend endpoint", false},
      {"OTEL_EXPORTER_OTLP_TRACES_ENDPOINT", "trace-specific OTLP endpoint override", false},
      {"OTEL_EXPORTER_OTLP_PROTOCOL", "otlp/grpc or otlp/http protocol selector", false},
      {"OTEL_EXPORTER_OTLP_HEADERS", "OTLP authorization or tenant headers", true},
      {"OTEL_SERVICE_NAME", "service name attached to emitted spans", false},
      {"CCAD_TRACE_EXPORT_ENABLED", "explicit CCad opt-in switch for telemetry export", false},
      {"CCAD_TRACE_BACKEND", "preferred backend such as langfuse or local_collector", false}};
  return vars;
}

void appendEnvSchema(std::ostringstream& out) {
  out << '[';
  const auto& vars = traceEnvVars();
  for (std::size_t i = 0; i < vars.size(); ++i) {
    if (i > 0) out << ',';
    out << "{\"name\":";
    appendEscaped(out, vars[i].name);
    out << ",\"purpose\":";
    appendEscaped(out, vars[i].purpose);
    out << ",\"secret\":" << (vars[i].secret ? "true" : "false") << "}";
  }
  out << ']';
}

bool traceExportExplicitlyEnabled() {
  const char* value = std::getenv("CCAD_TRACE_EXPORT_ENABLED");
  if (value == nullptr) return false;
  const std::string enabled(value);
  return enabled == "1" || enabled == "true" || enabled == "TRUE" || enabled == "on" ||
         enabled == "ON";
}

}  // namespace

std::string agentTraceExportSchemaJson() {
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"schema_kind\":\"ccad_agent_trace_export_schema\","
      << "\"surface\":\"headless_cli\","
      << "\"export_enabled\":false,"
      << "\"network_export_supported\":false,"
      << "\"network_probe_supported\":false,"
      << "\"semantic_convention\":\"opentelemetry_gen_ai\","
      << "\"semantic_convention_status\":\"development\","
      << "\"semantic_symbols\":[\"gen_ai\"],"
      << "\"backends\":[\"langfuse\",\"otlp_http\",\"otlp_grpc\",\"local_collector\"],"
      << "\"span_plan\":";
  appendStringArray(out, traceSpanPlan());
  out << ",\"env_vars\":";
  appendEnvSchema(out);
  out << ",\"redaction_policy_method\":\"agent.trace_redaction_policy\","
      << "\"template_method\":\"agent.trace_export_template\","
      << "\"dry_run_method\":\"agent.trace_export_dry_run\","
      << "\"reference_notes\":[\"OpenTelemetry GenAI semantic conventions are tracked as a versioned mapping\","
      << "\"Langfuse accepts OTLP traces through its OpenTelemetry endpoint\","
      << "\"Header and authorization values are never printed by CCad\"]}";
  return out.str();
}

std::string agentTraceExportTemplateJson() {
  return "{\"schema_version\":1,"
         "\"template_kind\":\"ccad_agent_trace_export_template\","
         "\"surface\":\"headless_cli\","
         "\"default_export_enabled\":false,"
         "\"secret_values_present\":false,"
         "\"endpoint_env\":\"OTEL_EXPORTER_OTLP_ENDPOINT\","
         "\"traces_endpoint_env\":\"OTEL_EXPORTER_OTLP_TRACES_ENDPOINT\","
         "\"headers_env\":\"OTEL_EXPORTER_OTLP_HEADERS\","
         "\"service_name_env\":\"OTEL_SERVICE_NAME\","
         "\"backend_env\":\"CCAD_TRACE_BACKEND\","
         "\"enabled_env\":\"CCAD_TRACE_EXPORT_ENABLED\","
         "\"backend_options\":[\"langfuse\",\"otlp_http\",\"otlp_grpc\",\"local_collector\"],"
         "\"langfuse_endpoint_hint\":\"/api/public/otel\","
         "\"store_secret_values\":false,"
         "\"export_prompt_content\":false,"
         "\"export_tool_payloads\":false,"
         "\"export_screenshots\":false,"
         "\"export_design_files\":false,"
         "\"notes\":[\"Set OTLP environment variables outside project files\","
         "\"Keep CCAD_TRACE_EXPORT_ENABLED unset or false until explicit user opt-in\","
         "\"Run agent.trace_export_dry_run to inspect presence and redaction only\"]}";
}

std::string agentTraceRedactionPolicyJson() {
  return "{\"schema_version\":1,"
         "\"policy_kind\":\"ccad_agent_trace_redaction_policy\","
         "\"surface\":\"headless_cli\","
         "\"secret_values\":\"never\","
         "\"export_prompt_content_by_default\":false,"
         "\"export_tool_payloads_by_default\":false,"
         "\"export_screenshots_by_default\":false,"
         "\"export_design_files_by_default\":false,"
         "\"export_mouse_keyboard_events_by_default\":false,"
         "\"redacted_fields\":[\"authorization\",\"api_key\",\"token\",\"password\","
         "\"OTEL_EXPORTER_OTLP_HEADERS\",\"OPENAI_API_KEY\",\"ANTHROPIC_API_KEY\","
         "\"GEMINI_API_KEY\",\"GOOGLE_API_KEY\"],"
         "\"allowed_default_fields\":[\"span_name\",\"span_kind\",\"duration_ms\","
         "\"method\",\"tool_name\",\"exit_code\",\"diagnostic_count\",\"artifact_sha256\"],"
         "\"payload_policy\":\"metadata_and_counts_only_until_user_opt_in\"}";
}

std::string agentTraceExportDryRunJson() {
  const bool export_enabled = traceExportExplicitlyEnabled();
  const bool endpoint_present = envPresent("OTEL_EXPORTER_OTLP_ENDPOINT");
  const bool traces_endpoint_present = envPresent("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT");
  const bool headers_present = envPresent("OTEL_EXPORTER_OTLP_HEADERS");
  const bool service_name_present = envPresent("OTEL_SERVICE_NAME");
  const bool backend_present = envPresent("CCAD_TRACE_BACKEND");
  const bool configured = endpoint_present || traces_endpoint_present || backend_present;
  const bool would_export = export_enabled && (endpoint_present || traces_endpoint_present);

  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"dry_run_kind\":\"ccad_agent_trace_export_dry_run\","
      << "\"surface\":\"headless_cli\","
      << "\"network_probe_performed\":false,"
      << "\"network_export_performed\":false,"
      << "\"redaction_applied\":true,"
      << "\"secret_values_present\":false,"
      << "\"configured\":" << (configured ? "true" : "false") << ","
      << "\"export_enabled_env_present\":"
      << (envPresent("CCAD_TRACE_EXPORT_ENABLED") ? "true" : "false") << ","
      << "\"would_export\":" << (would_export ? "true" : "false") << ","
      << "\"endpoint_env\":\"OTEL_EXPORTER_OTLP_ENDPOINT\","
      << "\"endpoint_env_present\":" << (endpoint_present ? "true" : "false") << ","
      << "\"traces_endpoint_env_present\":" << (traces_endpoint_present ? "true" : "false")
      << ","
      << "\"headers_env\":\"OTEL_EXPORTER_OTLP_HEADERS\","
      << "\"headers_env_present\":" << (headers_present ? "true" : "false") << ","
      << "\"headers_value\":\"redacted\","
      << "\"service_name_env_present\":" << (service_name_present ? "true" : "false") << ","
      << "\"backend_env_present\":" << (backend_present ? "true" : "false") << ","
      << "\"next_method\":\"agent.trace_export_template\"}";
  return out.str();
}

std::string agentObservabilityConfigJson() {
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"config_kind\":\"ccad_agent_observability_config\","
      << "\"status\":\"disabled_until_user_configured\","
      << "\"protocol\":\"opentelemetry\","
      << "\"semantic_convention\":\"opentelemetry_gen_ai\","
      << "\"otel_backend_options\":[\"langfuse\",\"otlp_http\",\"otlp_grpc\",\"local_collector\"],"
      << "\"span_plan\":";
  appendStringArray(out, traceSpanPlan());
  out << ",\"schema_method\":\"agent.trace_export_schema\","
      << "\"template_method\":\"agent.trace_export_template\","
      << "\"redaction_policy_method\":\"agent.trace_redaction_policy\","
      << "\"dry_run_method\":\"agent.trace_export_dry_run\","
      << "\"redaction_policy\":{\"secret_values\":\"never\","
      << "\"export_design_files_by_default\":false,"
      << "\"export_screenshots_by_default\":false,"
      << "\"export_prompt_content_by_default\":false,"
      << "\"export_tool_payloads_by_default\":false}}";
  return out.str();
}

}  // namespace ccad_cli
