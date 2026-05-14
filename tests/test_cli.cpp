#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifndef CCAD_BINARY
#error "CCAD_BINARY must be defined"
#endif

namespace {

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

int run(const std::string& command) {
#ifdef _WIN32
  const std::string shell_command = "cmd /C \"" + command + "\"";
#else
  const std::string shell_command = command;
#endif
  return std::system(shell_command.c_str());
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

}  // namespace

int main() {
  const std::filesystem::path temp = std::filesystem::temp_directory_path() / "ccad_cli_test";
  std::filesystem::create_directories(temp);
  const std::filesystem::path project_path = temp / "demo.ccad.json";
  const std::filesystem::path diagnostics_path = temp / "diagnostics.json";

  const std::string init_command = quote(CCAD_BINARY) + " init --name demo --out " +
                                   quote(project_path);
  assert(run(init_command) == 0);
  assert(std::filesystem::exists(project_path));

  const std::string project_json = readFile(project_path);
  assert(project_json.find("\"name\": \"demo\"") != std::string::npos);

  const std::string validate_clean = quote(CCAD_BINARY) + " validate " + quote(project_path) +
                                     " > " + quote(diagnostics_path);
  assert(run(validate_clean) == 0);
  assert(readFile(diagnostics_path).find("\"diagnostics\": [") != std::string::npos);

  const std::filesystem::path invalid_path = temp / "invalid.ccad.json";
  std::ofstream invalid(invalid_path);
  invalid << "{\n"
          << "  \"schema_version\": 1,\n"
          << "  \"id\": \"bad\",\n"
          << "  \"name\": \"bad\",\n"
          << "  \"components\": [],\n"
          << "  \"constraints\": [],\n"
          << "  \"nets\": [\n"
          << "    {\"id\": \"N_BAD\", \"members\": ["
          << "{\"component_id\": \"U404\", \"pin_name\": \"VDD\"}]}\n"
          << "  ]\n"
          << "}\n";
  invalid.close();

  const std::string validate_invalid = quote(CCAD_BINARY) + " validate " + quote(invalid_path) +
                                       " > " + quote(diagnostics_path);
  assert(run(validate_invalid) != 0);
  const std::string invalid_output = readFile(diagnostics_path);
  assert(invalid_output.find("\"code\": \"UNKNOWN_COMPONENT\"") != std::string::npos);
}
