#include "agent_provider_config.hpp"

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace ccad_cli {
namespace {

struct EnvVarSpec {
  std::string name;
  std::string purpose;
};

struct ProviderSpec {
  std::string id;
  std::string label;
  std::string access_path;
  std::vector<EnvVarSpec> env_vars;
  std::vector<std::string> security_notes;
};

const std::vector<ProviderSpec>& providerSpecs() {
  static const std::vector<ProviderSpec> specs = {
      {"openai",
       "OpenAI API",
       "official_api",
       {{"OPENAI_API_KEY", "bearer API key"},
        {"OPENAI_ORG_ID", "optional organization header"},
        {"OPENAI_PROJECT_ID", "optional project header"},
        {"CCAD_OPENAI_MODEL", "preferred model override"}},
       {"Use Authorization bearer credentials.",
        "Keep keys outside project files and source control."}},
      {"openai_compatible",
       "OpenAI-compatible API",
       "openai_compatible_api",
       {{"CCAD_OPENAI_COMPATIBLE_API_KEY", "OpenAI-compatible API key"},
        {"CCAD_OPENAI_COMPATIBLE_BASE_URL", "OpenAI-compatible base URL"},
        {"CCAD_OPENAI_COMPATIBLE_MODEL", "preferred model override"}},
       {"Use only provider-approved compatible endpoints.",
        "Never infer terms of service compatibility from URL shape alone."}},
      {"anthropic",
       "Anthropic Claude API",
       "anthropic_api",
       {{"ANTHROPIC_API_KEY", "Claude API key"},
        {"ANTHROPIC_BASE_URL", "optional Claude-compatible base URL"},
        {"CCAD_ANTHROPIC_MODEL", "preferred model override"}},
       {"Use official Claude API or approved partner APIs.",
        "Do not automate consumer Claude web sessions as a clean integration path."}},
      {"google_gemini",
       "Google Gemini API",
       "google_gemini_api",
       {{"GEMINI_API_KEY", "Gemini API key"},
        {"GOOGLE_API_KEY", "Gemini API key alias"},
        {"CCAD_GEMINI_MODEL", "preferred model override"}},
       {"Restrict keys to the Gemini API where possible.",
        "Unrestricted Gemini traffic keys are being discontinued on 2026-06-19."}},
      {"local_model_server",
       "Local model server",
       "local_model_server",
       {{"CCAD_LOCAL_MODEL_BASE_URL", "local model server base URL"},
        {"CCAD_LOCAL_MODEL_NAME", "preferred local model name"},
        {"CCAD_LOCAL_MODEL_API_KEY", "optional local server key"}},
       {"Treat local servers as user-controlled tools.",
        "Keep localhost access explicit and auditable."}}};
  return specs;
}

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

void appendEnvVarSchema(std::ostringstream& out, const std::vector<EnvVarSpec>& vars) {
  out << '[';
  for (std::size_t i = 0; i < vars.size(); ++i) {
    if (i > 0) out << ',';
    out << "{\"name\":";
    appendEscaped(out, vars[i].name);
    out << ",\"purpose\":";
    appendEscaped(out, vars[i].purpose);
    out << "}";
  }
  out << ']';
}

void appendEnvVarNames(std::ostringstream& out, const std::vector<EnvVarSpec>& vars) {
  out << '[';
  for (std::size_t i = 0; i < vars.size(); ++i) {
    if (i > 0) out << ',';
    appendEscaped(out, vars[i].name);
  }
  out << ']';
}

bool appendProviderStatus(std::ostringstream& out, const ProviderSpec& provider) {
  bool configured = false;
  out << "{\"id\":";
  appendEscaped(out, provider.id);
  out << ",\"label\":";
  appendEscaped(out, provider.label);
  out << ",\"access_path\":";
  appendEscaped(out, provider.access_path);
  out << ",\"env_vars\":[";
  for (std::size_t i = 0; i < provider.env_vars.size(); ++i) {
    if (i > 0) out << ',';
    const bool present = envPresent(provider.env_vars[i].name);
    configured = configured || present;
    out << "{\"name\":";
    appendEscaped(out, provider.env_vars[i].name);
    out << ",\"present\":" << (present ? "true" : "false")
        << ",\"value\":\"redacted\"}";
  }
  out << "],\"configured\":" << (configured ? "true" : "false") << "}";
  return configured;
}

}  // namespace

std::string agentProviderConfigSchemaJson() {
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"schema_kind\":\"ccad_agent_provider_config_schema\","
      << "\"surface\":\"headless_cli\","
      << "\"provider_execution\":false,"
      << "\"secret_value_policy\":\"never_emit_secret_values\","
      << "\"secret_storage\":\"environment_or_os_credential_store_only\","
      << "\"project_file_secret_storage\":false,"
      << "\"status_method\":\"agent.provider_status\","
      << "\"template_method\":\"agent.provider_config_template\","
      << "\"providers\":[";
  const auto& specs = providerSpecs();
  for (std::size_t i = 0; i < specs.size(); ++i) {
    if (i > 0) out << ',';
    out << "{\"id\":";
    appendEscaped(out, specs[i].id);
    out << ",\"label\":";
    appendEscaped(out, specs[i].label);
    out << ",\"access_path\":";
    appendEscaped(out, specs[i].access_path);
    out << ",\"env_vars\":";
    appendEnvVarSchema(out, specs[i].env_vars);
    out << ",\"security_notes\":";
    appendStringArray(out, specs[i].security_notes);
    if (specs[i].id == "google_gemini") {
      out << ",\"key_restriction_required\":true,"
          << "\"unrestricted_key_cutoff\":\"2026-06-19\"";
    }
    out << "}";
  }
  out << "],\"disallowed_paths\":[\"consumer_web_ui_automation\","
      << "\"subscription_browser_session_reuse\",\"gmail_account_automation\"],"
      << "\"config_fields\":[\"enabled\",\"provider_id\",\"api_key_env\","
      << "\"base_url_env\",\"model_env\",\"local_only\",\"data_policy\"],"
      << "\"secret_fields_policy\":\"store_env_var_names_only\"}";
  return out.str();
}

std::string agentProviderConfigTemplateJson() {
  std::ostringstream out;
  out << "{\"schema_version\":1,"
      << "\"template_kind\":\"ccad_agent_provider_config_template\","
      << "\"surface\":\"headless_cli\","
      << "\"provider_execution\":false,"
      << "\"secret_values_present\":false,"
      << "\"project_file_secret_storage\":false,"
      << "\"storage_policy\":\"store_env_var_names_or_os_credential_references_only\","
      << "\"providers\":[";
  const auto& specs = providerSpecs();
  for (std::size_t i = 0; i < specs.size(); ++i) {
    if (i > 0) out << ',';
    out << "{\"id\":";
    appendEscaped(out, specs[i].id);
    out << ",\"enabled\":false,\"access_path\":";
    appendEscaped(out, specs[i].access_path);
    out << ",\"env_vars\":";
    appendEnvVarNames(out, specs[i].env_vars);
    out << ",\"secret_value_field_absent\":true}";
  }
  out << "],\"default_mode\":\"local_off_until_user_configured\","
      << "\"approval_required_before_remote_model_upload\":true,"
      << "\"notes\":[\"Set environment variables outside project files\","
      << "\"Run agent.provider_status to check presence without printing values\","
      << "\"Provider execution remains disabled in Sprint 197\"]}";
  return out.str();
}

std::string agentProviderStatusJson() {
  std::ostringstream out;
  bool any_configured = false;
  out << "{\"schema_version\":1,"
      << "\"status_kind\":\"ccad_agent_provider_status\","
      << "\"surface\":\"headless_cli\","
      << "\"provider_execution\":false,"
      << "\"secret_values_present\":false,"
      << "\"env_value_redaction\":\"presence_only\","
      << "\"providers\":[";
  const auto& specs = providerSpecs();
  for (std::size_t i = 0; i < specs.size(); ++i) {
    if (i > 0) out << ',';
    const bool configured = appendProviderStatus(out, specs[i]);
    any_configured = any_configured || configured;
  }
  out << "],\"provider_configured\":" << (any_configured ? "true" : "false")
      << ",\"project_file_secret_storage\":false,"
      << "\"network_probe_performed\":false,"
      << "\"next_method\":\"agent.provider_config_template\"}";
  return out.str();
}

}  // namespace ccad_cli
