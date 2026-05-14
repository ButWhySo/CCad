#include "test_support.hpp"

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
  require(run(init_command) == 0, "init exits zero");
  require(std::filesystem::exists(project_path), "init writes project file");

  const std::string project_json = readFile(project_path);
  require(project_json.find("\"name\": \"demo\"") != std::string::npos, "init writes name");

  const std::filesystem::path board_project_path = temp / "board.ccad.json";
  const std::string board_init_command = quote(CCAD_BINARY) +
                                         " init --name board --width-mm 42 --height-mm 28 --out " +
                                         quote(board_project_path);
  require(run(board_init_command) == 0, "board init exits zero");
  const std::string board_json = readFile(board_project_path);
  require(board_json.find("\"board\"") != std::string::npos, "board init writes board");
  require(board_json.find("\"width_nm\": 42000000") != std::string::npos,
          "board init writes width");

  const std::string missing_height_command = quote(CCAD_BINARY) +
                                             " init --name bad --width-mm 42 --out " +
                                             quote(temp / "bad.ccad.json");
  require(run(missing_height_command) != 0, "board init rejects missing height");

  const std::string bad_width_command = quote(CCAD_BINARY) +
                                        " init --name bad --width-mm nope --height-mm 28 --out " +
                                        quote(temp / "bad-width.ccad.json");
  require(run(bad_width_command) != 0, "board init rejects invalid width");

  const std::string validate_clean = quote(CCAD_BINARY) + " validate " + quote(project_path) +
                                     " > " + quote(diagnostics_path);
  require(run(validate_clean) == 0, "clean validate exits zero");
  require(readFile(diagnostics_path).find("\"diagnostics\": [") != std::string::npos,
          "validate writes diagnostics json");

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
  require(run(validate_invalid) != 0, "invalid validate exits nonzero");
  const std::string invalid_output = readFile(diagnostics_path);
  require(invalid_output.find("\"code\": \"UNKNOWN_COMPONENT\"") != std::string::npos,
          "invalid validate reports unknown component");

  const std::filesystem::path inspect_path = temp / "inspect.json";
  const std::string inspect_command = quote(CCAD_BINARY) + " inspect " + quote(project_path) +
                                      " > " + quote(inspect_path);
  require(run(inspect_command) == 0, "inspect exits zero");
  const std::string inspect_output = readFile(inspect_path);
  require(inspect_output.find("\"project\"") != std::string::npos, "inspect has project object");
  require(inspect_output.find("\"components\": 0") != std::string::npos,
          "inspect has component count");
  require(inspect_output.find("\"status\": \"Warnings: 1\"") != std::string::npos,
          "inspect has review status");

  const std::filesystem::path board_inspect_path = temp / "board-inspect.json";
  const std::string board_inspect_command = quote(CCAD_BINARY) + " inspect " +
                                            quote(board_project_path) + " > " +
                                            quote(board_inspect_path);
  require(run(board_inspect_command) == 0, "board inspect exits zero");
  const std::string board_inspect_output = readFile(board_inspect_path);
  require(board_inspect_output.find("\"has_board\": true") != std::string::npos,
          "inspect reports board present");
  require(board_inspect_output.find("\"width_nm\": 42000000") != std::string::npos,
          "inspect reports board width");

  const std::filesystem::path diff_after_path = temp / "diff-after.ccad.json";
  std::ofstream diff_after(diff_after_path);
  diff_after << "{\n"
             << "  \"schema_version\": 1,\n"
             << "  \"id\": \"proj-diff\",\n"
             << "  \"name\": \"diff\",\n"
             << "  \"components\": [\n"
             << "    {\"id\": \"U1\", \"part\": \"MCU\", \"pins\": []}\n"
             << "  ],\n"
             << "  \"constraints\": [],\n"
             << "  \"nets\": []\n"
             << "}\n";
  diff_after.close();
  const std::filesystem::path diff_output_path = temp / "diff.json";
  const std::string diff_command = quote(CCAD_BINARY) + " diff " + quote(project_path) + " " +
                                   quote(diff_after_path) + " > " + quote(diff_output_path);
  require(run(diff_command) == 0, "diff exits zero");
  const std::string diff_output = readFile(diff_output_path);
  require(diff_output.find("\"added\": 1") != std::string::npos, "diff has added count");
  require(diff_output.find("\"object_type\": \"component\"") != std::string::npos,
          "diff has object type");
  require(diff_output.find("\"object_id\": \"U1\"") != std::string::npos, "diff has object id");

  const std::filesystem::path escaped_path = temp / "escaped.ccad.json";
  const std::string init_escaped = quote(CCAD_BINARY) +
                                   " init --name \"demo\tname\" --out " + quote(escaped_path);
  require(run(init_escaped) == 0, "init accepts escaped shell tab value");
  require(readFile(escaped_path).find("\\t") != std::string::npos,
          "init emits valid escaped JSON string");
}
