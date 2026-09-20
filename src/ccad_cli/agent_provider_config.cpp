#include "agent_provider_config.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#include <conio.h>
#endif

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
      {"cerebras",
       "Cerebras API",
       "cerebras_api",
       {{"CEREBRAS_API_KEY", "Cerebras API key"},
        {"CCAD_CEREBRAS_MODEL", "preferred Cerebras model override"}},
       {"Use the official Cerebras OpenAI-compatible API endpoint.",
        "Keep keys outside project files and source control."}},
      {"openrouter",
       "OpenRouter API",
       "openrouter_api",
       {{"OPENROUTER_API_KEY", "OpenRouter API key"},
        {"CCAD_OPENROUTER_MODEL", "preferred OpenRouter model ID"}},
       {"Use the official OpenRouter API endpoint.",
        "Review routed provider data and model terms before use."}},
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
      {"ollama",
       "Ollama (local)",
       "ollama_local_api",
       {{"CCAD_OLLAMA_BASE_URL", "Ollama OpenAI-compatible base URL"},
        {"CCAD_OLLAMA_MODEL", "preferred installed Ollama model"}},
       {"Default endpoint is localhost and does not require an API key.",
        "Refresh installed models only on explicit user request."}},
      {"local_model",
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

bool vaultPresent(const std::string& provider) {
#ifdef _WIN32
  const std::string target = "CCad/provider/" + provider;
  const std::wstring target_w(target.begin(), target.end());
  PCREDENTIALW credential = nullptr;
  const BOOL found = CredReadW(target_w.c_str(), CRED_TYPE_GENERIC, 0, &credential);
  if (credential != nullptr) CredFree(credential);
  return found != FALSE;
#else
  (void)provider;
  return false;
#endif
}

bool knownProvider(const std::string& id) {
  for (const ProviderSpec& spec : providerSpecs()) {
    if (spec.id == id) return true;
  }
  return false;
}

std::string credentialTarget(const std::string& provider) {
  return "CCad/provider/" + provider;
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
  bool configured = vaultPresent(provider.id);
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
  out << "],\"vault_present\":" << (vaultPresent(provider.id) ? "true" : "false")
      << ",\"configured\":" << (configured ? "true" : "false") << "}";
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

int agentProviderCredentialCommand(const std::vector<std::string>& args) {
  if (args.size() != 2 || (args[0] != "status" && args[0] != "set" && args[0] != "remove") ||
      !knownProvider(args[1])) {
    std::cerr << "Usage: ccad agent credential <status|set|remove> <provider>\n";
    return 2;
  }
#ifndef _WIN32
  std::cerr << "Credential Manager integration is currently supported on Windows only.\n";
  return 3;
#else
  const std::string provider = args[1];
  const std::string target = credentialTarget(provider);
  const std::wstring target_w(target.begin(), target.end());
  if (args[0] == "status") {
    PCREDENTIALW credential = nullptr;
    const BOOL found = CredReadW(target_w.c_str(), CRED_TYPE_GENERIC, 0, &credential);
    if (credential != nullptr) CredFree(credential);
    std::cout << "{\"provider\":\"" << provider << "\",\"stored\":"
              << (found ? "true" : "false") << ",\"secret_value_visible\":false}\n";
    return found || GetLastError() == ERROR_NOT_FOUND ? 0 : 4;
  }
  if (args[0] == "remove") {
    const BOOL removed = CredDeleteW(target_w.c_str(), CRED_TYPE_GENERIC, 0);
    const DWORD error = removed ? ERROR_SUCCESS : GetLastError();
    if (!removed && error != ERROR_NOT_FOUND) return 4;
    std::cout << "{\"provider\":\"" << provider
              << "\",\"removed\":true,\"secret_value_visible\":false}\n";
    return 0;
  }
  std::cerr << "Enter " << provider << " API key (input hidden): ";
  std::string secret;
  for (int ch = _getwch(); ch != '\r' && ch != '\n'; ch = _getwch()) {
    if (ch == '\b') { if (!secret.empty()) secret.pop_back(); }
    else if (ch >= 32 && ch <= 126) secret.push_back(static_cast<char>(ch));
  }
  std::cerr << "\n";
  if (secret.empty() || secret.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE) return 2;
  CREDENTIALW credential{};
  credential.Type = CRED_TYPE_GENERIC;
  credential.TargetName = const_cast<wchar_t*>(target_w.c_str());
  credential.CredentialBlobSize = static_cast<DWORD>(secret.size());
  credential.CredentialBlob = reinterpret_cast<LPBYTE>(secret.data());
  credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
  if (!CredWriteW(&credential, 0)) return 4;
  std::cout << "{\"provider\":\"" << provider
            << "\",\"stored\":true,\"secret_value_visible\":false}\n";
  return 0;
#endif
}

}  // namespace ccad_cli
