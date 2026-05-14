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

  const std::string add_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(board_project_path) +
      " --id P1 --component U1 --pin 1 --net N1 --layer F.Cu"
      " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0";
  require(run(add_pad_command) == 0, "pcb add-pad exits zero");
  const std::string pad_json = readFile(board_project_path);
  require(pad_json.find("\"pads\"") != std::string::npos, "pcb add-pad writes pads");
  require(pad_json.find("\"id\": \"P1\"") != std::string::npos, "pcb add-pad writes id");
  require(pad_json.find("\"width_nm\": 1500000") != std::string::npos,
          "pcb add-pad writes width");

  const std::string add_via_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
      " --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
  require(run(add_via_command) == 0, "pcb add-via exits zero");
  const std::string via_json = readFile(board_project_path);
  require(via_json.find("\"vias\"") != std::string::npos, "pcb add-via writes vias");
  require(via_json.find("\"drill_nm\": 400000") != std::string::npos,
          "pcb add-via writes drill");

  const std::string add_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T1 --net N1 --layer F.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
  require(run(add_track_command) == 0, "pcb add-track exits zero");
  const std::string track_json = readFile(board_project_path);
  require(track_json.find("\"tracks\"") != std::string::npos, "pcb add-track writes tracks");
  require(track_json.find("\"width_nm\": 250000") != std::string::npos,
          "pcb add-track writes width");

  require(run(add_pad_command) != 0, "pcb add-pad rejects duplicate id");

  const std::string bad_layer_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T_BAD --net N1 --layer Inner.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
  require(run(bad_layer_command) != 0, "pcb add-track rejects unknown layer");

  const std::string bad_via_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
      " --id V_BAD --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.4 --drill-mm 0.8";
  require(run(bad_via_command) != 0, "pcb add-via rejects drill larger than diameter");

  const std::string outside_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T_OUT --net N1 --layer F.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 99 --end-y-mm 9 --width-mm 0.25";
  require(run(outside_track_command) != 0, "pcb add-track rejects endpoint outside board");

  const std::string missing_board_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(project_path) +
      " --id V_NO_BOARD --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
  require(run(missing_board_command) != 0, "pcb add-via rejects missing board");

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
