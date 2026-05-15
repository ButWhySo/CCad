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

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream output(path, std::ios::binary);
  output << content;
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

  const std::filesystem::path help_json_path = temp / "help.json";
  const std::string help_json_command = quote(CCAD_BINARY) + " help --format json > " +
                                        quote(help_json_path);
  require(run(help_json_command) == 0, "help json exits zero");
  const std::string help_json = readFile(help_json_path);
  require(help_json.find("\"commands\"") != std::string::npos, "help json has commands");
  require(help_json.find("\"name\": \"pcb place-footprint\"") != std::string::npos,
          "help json describes footprint placement");
  require(help_json.find("\"name\": \"pcb add-keepout\"") != std::string::npos,
          "help json describes keepout authoring");
  require(help_json.find("\"name\": \"lib catalog-info\"") != std::string::npos,
          "help json describes catalog info");
  require(help_json.find("\"name\": \"lib catalog-find\"") != std::string::npos,
          "help json describes catalog lookup");
  require(help_json.find("\"name\": \"lib catalog-search\"") != std::string::npos,
          "help json describes catalog search");
  require(help_json.find("--rotation-deg") != std::string::npos,
          "help json exposes rotation option");

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

  const std::string add_keepout_command =
      quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(board_project_path) +
      " --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3";
  require(run(add_keepout_command) == 0, "pcb add-keepout exits zero");
  const std::string keepout_json = readFile(board_project_path);
  require(keepout_json.find("\"keepouts\"") != std::string::npos,
          "pcb add-keepout writes keepouts");
  require(keepout_json.find("\"id\": \"K1\"") != std::string::npos,
          "pcb add-keepout writes id");
  require(keepout_json.find("\"kind\": \"placement\"") != std::string::npos,
          "pcb add-keepout writes kind");
  require(keepout_json.find("\"width_nm\": 4000000") != std::string::npos,
          "pcb add-keepout writes width");

  require(run(add_pad_command) != 0, "pcb add-pad rejects duplicate id");
  require(run(add_keepout_command) != 0, "pcb add-keepout rejects duplicate id");

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

  const std::string outside_keepout_command =
      quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(board_project_path) +
      " --id K_OUT --kind placement --x-mm 40 --y-mm 26 --width-mm 4 --height-mm 3";
  require(run(outside_keepout_command) != 0, "pcb add-keepout rejects area outside board");

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

  std::string board_with_net = readFile(board_project_path);
  const std::string empty_nets = "  \"nets\": [\n  ]";
  const std::string logical_n1 =
      "  \"nets\": [\n"
      "    {\n"
      "      \"id\": \"N1\",\n"
      "      \"members\": [\n"
      "      ]\n"
      "    }\n"
      "  ]";
  const std::size_t nets_position = board_with_net.find(empty_nets);
  require(nets_position != std::string::npos, "board fixture has empty nets before clean drc");
  board_with_net.replace(nets_position, empty_nets.size(), logical_n1);
  writeFile(board_project_path, board_with_net);

  const std::filesystem::path drc_output_path = temp / "drc.json";
  const std::string drc_command = quote(CCAD_BINARY) + " drc " + quote(board_project_path) +
                                  " > " + quote(drc_output_path);
  require(run(drc_command) == 0, "clean drc exits zero");
  require(readFile(drc_output_path).find("\"diagnostics\": [") != std::string::npos,
          "drc writes diagnostics json");

  const std::filesystem::path invalid_drc_path = temp / "invalid-drc.ccad.json";
  std::ofstream invalid_drc(invalid_drc_path);
  invalid_drc << "{\n"
              << "  \"schema_version\": 1,\n"
              << "  \"id\": \"proj-invalid-drc\",\n"
              << "  \"name\": \"invalid-drc\",\n"
              << "  \"board\": {\n"
              << "    \"outline\": {\n"
              << "      \"x_nm\": 0,\n"
              << "      \"y_nm\": 0,\n"
              << "      \"width_nm\": 42000000,\n"
              << "      \"height_nm\": 28000000\n"
              << "    },\n"
              << "    \"layers\": [\n"
              << "      {\"id\": \"F.Cu\", \"name\": \"Front copper\", \"kind\": \"copper\", \"visible\": true}\n"
              << "    ],\n"
              << "    \"pads\": [],\n"
              << "    \"vias\": [],\n"
              << "    \"tracks\": [\n"
              << "      {\"id\": \"T_BAD\", \"net_id\": \"N1\", \"layer_id\": \"Inner.Cu\", "
              << "\"start\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, "
              << "\"end\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"width_nm\": 250000}\n"
              << "    ]\n"
              << "  },\n"
              << "  \"components\": [],\n"
              << "  \"constraints\": [],\n"
              << "  \"nets\": []\n"
              << "}\n";
  invalid_drc.close();
  const std::string invalid_drc_command = quote(CCAD_BINARY) + " drc " +
                                          quote(invalid_drc_path) + " > " +
                                          quote(drc_output_path);
  require(run(invalid_drc_command) != 0, "invalid drc exits nonzero");
  require(readFile(drc_output_path).find("\"code\": \"UNKNOWN_TRACK_LAYER\"") !=
              std::string::npos,
          "drc reports unknown track layer");

  const std::filesystem::path footprint_in_path = temp / "R_0805_2012Metric.kicad_mod";
  std::ofstream footprint_in(footprint_in_path);
  footprint_in << "(footprint \"R_0805_2012Metric\"\n"
               << "  (version 20240101)\n"
               << "  (generator \"ccad-test\")\n"
               << "  (pad \"1\" smd roundrect (at -0.95 0 0) (size 1.0 1.45) "
               << "(layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
               << ")\n";
  footprint_in.close();
  const std::filesystem::path footprint_out_path = temp / "R_0805_2012Metric.ccad-footprint.json";
  const std::string import_footprint_command =
      quote(CCAD_BINARY) + " lib import-footprint --in " + quote(footprint_in_path) +
      " --out " + quote(footprint_out_path);
  require(run(import_footprint_command) == 0, "lib import-footprint exits zero");
  const std::string footprint_output = readFile(footprint_out_path);
  require(footprint_output.find("\"name\": \"R_0805_2012Metric\"") != std::string::npos,
          "lib import-footprint writes name");
  require(footprint_output.find("\"width_nm\": 1000000") != std::string::npos,
          "lib import-footprint writes pad width");

  const std::filesystem::path catalog_path = temp / "catalog.ccad-library.json";
  std::ofstream catalog(catalog_path);
  catalog << "{\n"
          << "  \"schema_version\": 1,\n"
          << "  \"name\": \"local-kicad-cache\",\n"
          << "  \"source\": {\n"
          << "    \"name\": \"kicad-official\",\n"
          << "    \"kind\": \"kicad\",\n"
          << "    \"url\": \"https://gitlab.com/kicad/libraries/kicad-footprints\",\n"
          << "    \"commit\": \"abc123\",\n"
          << "    \"mirror\": \"official\",\n"
          << "    \"fetched_at\": \"2026-05-15T00:00:00Z\"\n"
          << "  },\n"
          << "  \"items\": [\n"
          << "    {\n"
          << "      \"id\": \"footprint:Resistor_SMD:R_0603_1608Metric\",\n"
          << "      \"kind\": \"footprint\",\n"
          << "      \"name\": \"R_0603_1608Metric\",\n"
          << "      \"source_path\": \"Resistor_SMD.pretty/R_0603_1608Metric.kicad_mod\",\n"
          << "      \"native_path\": \"footprints/Resistor_SMD/R_0603_1608Metric.ccad-footprint.json\",\n"
          << "      \"sha256\": \"0123456789abcdef\",\n"
          << "      \"license\": \"CC-BY-SA-4.0 WITH KiCad-library-exception\",\n"
          << "      \"provenance\": \"kicad-official@abc123\",\n"
          << "      \"warnings\": [\n"
          << "      ]\n"
          << "    }\n"
          << "    ,\n"
          << "    {\n"
          << "      \"id\": \"footprint:Capacitor_SMD:C_0603_1608Metric\",\n"
          << "      \"kind\": \"footprint\",\n"
          << "      \"name\": \"C_0603_1608Metric\",\n"
          << "      \"source_path\": \"Capacitor_SMD.pretty/C_0603_1608Metric.kicad_mod\",\n"
          << "      \"native_path\": \"footprints/Capacitor_SMD/C_0603_1608Metric.ccad-footprint.json\",\n"
          << "      \"sha256\": \"fedcba9876543210\",\n"
          << "      \"license\": \"CC-BY-SA-4.0 WITH KiCad-library-exception\",\n"
          << "      \"provenance\": \"kicad-official@abc123\",\n"
          << "      \"warnings\": [\n"
          << "      ]\n"
          << "    }\n"
          << "  ]\n"
          << "}\n";
  catalog.close();

  const std::filesystem::path catalog_info_path = temp / "catalog-info.json";
  const std::string catalog_info_command =
      quote(CCAD_BINARY) + " lib catalog-info --catalog " + quote(catalog_path) + " > " +
      quote(catalog_info_path);
  require(run(catalog_info_command) == 0, "lib catalog-info exits zero");
  const std::string catalog_info_output = readFile(catalog_info_path);
  require(catalog_info_output.find("\"name\": \"local-kicad-cache\"") != std::string::npos,
          "lib catalog-info writes catalog name");
  require(catalog_info_output.find("\"item_count\": 2") != std::string::npos,
          "lib catalog-info writes item count");
  require(catalog_info_output.find("\"commit\": \"abc123\"") != std::string::npos,
          "lib catalog-info writes source commit");

  const std::filesystem::path catalog_find_path = temp / "catalog-find.json";
  const std::string catalog_find_command =
      quote(CCAD_BINARY) + " lib catalog-find --catalog " + quote(catalog_path) +
      " --id footprint:Resistor_SMD:R_0603_1608Metric > " + quote(catalog_find_path);
  require(run(catalog_find_command) == 0, "lib catalog-find exits zero");
  const std::string catalog_find_output = readFile(catalog_find_path);
  require(catalog_find_output.find("\"found\": true") != std::string::npos,
          "lib catalog-find reports found");
  require(catalog_find_output.find("\"native_path\": \"footprints/Resistor_SMD/R_0603_1608Metric.ccad-footprint.json\"") !=
              std::string::npos,
          "lib catalog-find writes native path");
  const std::string catalog_missing_command =
      quote(CCAD_BINARY) + " lib catalog-find --catalog " + quote(catalog_path) +
      " --id missing > " + quote(catalog_find_path);
  require(run(catalog_missing_command) != 0, "lib catalog-find missing item exits nonzero");
  require(readFile(catalog_find_path).find("\"found\": false") != std::string::npos,
          "lib catalog-find reports missing item");

  const std::filesystem::path catalog_search_path = temp / "catalog-search.json";
  const std::string catalog_search_command =
      quote(CCAD_BINARY) + " lib catalog-search --catalog " + quote(catalog_path) +
      " --query 0603 > " + quote(catalog_search_path);
  require(run(catalog_search_command) == 0, "lib catalog-search exits zero");
  const std::string catalog_search_output = readFile(catalog_search_path);
  require(catalog_search_output.find("\"count\": 2") != std::string::npos,
          "lib catalog-search writes match count");
  require(catalog_search_output.find("\"id\": \"footprint:Resistor_SMD:R_0603_1608Metric\"") !=
              std::string::npos,
          "lib catalog-search writes resistor match");
  require(catalog_search_output.find("\"id\": \"footprint:Capacitor_SMD:C_0603_1608Metric\"") !=
              std::string::npos,
          "lib catalog-search writes capacitor match");
  const std::string catalog_search_kind_command =
      quote(CCAD_BINARY) + " lib catalog-search --catalog " + quote(catalog_path) +
      " --query 0603 --kind symbol > " + quote(catalog_search_path);
  require(run(catalog_search_kind_command) == 0, "lib catalog-search kind filter exits zero");
  require(readFile(catalog_search_path).find("\"count\": 0") != std::string::npos,
          "lib catalog-search kind filter excludes mismatched kind");

  const std::filesystem::path catalog_validate_path = temp / "catalog-validate.json";
  const std::string catalog_validate_command =
      quote(CCAD_BINARY) + " lib catalog-validate --catalog " + quote(catalog_path) + " > " +
      quote(catalog_validate_path);
  require(run(catalog_validate_command) == 0, "lib catalog-validate clean catalog exits zero");
  require(readFile(catalog_validate_path).find("\"diagnostics\": [") != std::string::npos,
          "lib catalog-validate writes diagnostics array");

  const std::filesystem::path bad_catalog_path = temp / "bad-catalog.ccad-library.json";
  std::ofstream bad_catalog(bad_catalog_path);
  bad_catalog << "{\n"
              << "  \"schema_version\": 1,\n"
              << "  \"name\": \"bad-cache\",\n"
              << "  \"source\": {\n"
              << "    \"name\": \"kicad-official\",\n"
              << "    \"kind\": \"kicad\",\n"
              << "    \"url\": \"https://gitlab.com/kicad/libraries/kicad-footprints\",\n"
              << "    \"commit\": \"abc123\",\n"
              << "    \"mirror\": \"official\",\n"
              << "    \"fetched_at\": \"2026-05-15T00:00:00Z\"\n"
              << "  },\n"
              << "  \"items\": [\n"
              << "    {\n"
              << "      \"id\": \"footprint:dup\",\n"
              << "      \"kind\": \"footprint\",\n"
              << "      \"name\": \"dup-a\",\n"
              << "      \"source_path\": \"a.kicad_mod\",\n"
              << "      \"native_path\": \"a.ccad-footprint.json\",\n"
              << "      \"sha256\": \"aaa\",\n"
              << "      \"license\": \"CC-BY-SA-4.0 WITH KiCad-library-exception\",\n"
              << "      \"provenance\": \"kicad-official@abc123\",\n"
              << "      \"warnings\": []\n"
              << "    },\n"
              << "    {\n"
              << "      \"id\": \"footprint:dup\",\n"
              << "      \"kind\": \"footprint\",\n"
              << "      \"name\": \"dup-b\",\n"
              << "      \"source_path\": \"b.kicad_mod\",\n"
              << "      \"native_path\": \"b.ccad-footprint.json\",\n"
              << "      \"sha256\": \"\",\n"
              << "      \"license\": \"\",\n"
              << "      \"provenance\": \"\",\n"
              << "      \"warnings\": []\n"
              << "    }\n"
              << "  ]\n"
              << "}\n";
  bad_catalog.close();
  const std::string bad_catalog_validate_command =
      quote(CCAD_BINARY) + " lib catalog-validate --catalog " + quote(bad_catalog_path) +
      " > " + quote(catalog_validate_path);
  require(run(bad_catalog_validate_command) != 0,
          "lib catalog-validate invalid catalog exits nonzero");
  const std::string bad_catalog_validate_output = readFile(catalog_validate_path);
  require(bad_catalog_validate_output.find("\"code\": \"DUPLICATE_ITEM_ID\"") !=
              std::string::npos,
          "lib catalog-validate reports duplicate id");
  require(bad_catalog_validate_output.find("\"code\": \"MISSING_ITEM_SHA256\"") !=
              std::string::npos,
          "lib catalog-validate reports missing checksum");

  std::filesystem::create_directories(temp / "native" / "footprints");
  const std::filesystem::path native_artifact_path =
      temp / "native" / "footprints" / "demo.ccad-footprint.json";
  writeFile(native_artifact_path, "hello\n");
  const std::filesystem::path file_catalog_path = temp / "file-catalog.ccad-library.json";
  std::ofstream file_catalog(file_catalog_path);
  file_catalog << "{\n"
               << "  \"schema_version\": 1,\n"
               << "  \"name\": \"file-cache\",\n"
               << "  \"source\": {\n"
               << "    \"name\": \"kicad-official\",\n"
               << "    \"kind\": \"kicad\",\n"
               << "    \"url\": \"https://gitlab.com/kicad/libraries/kicad-footprints\",\n"
               << "    \"commit\": \"abc123\",\n"
               << "    \"mirror\": \"official\",\n"
               << "    \"fetched_at\": \"2026-05-15T00:00:00Z\"\n"
               << "  },\n"
               << "  \"items\": [\n"
               << "    {\n"
               << "      \"id\": \"footprint:demo\",\n"
               << "      \"kind\": \"footprint\",\n"
               << "      \"name\": \"demo\",\n"
               << "      \"source_path\": \"demo.kicad_mod\",\n"
               << "      \"native_path\": \"footprints/demo.ccad-footprint.json\",\n"
               << "      \"sha256\": \"5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03\",\n"
               << "      \"license\": \"CC-BY-SA-4.0 WITH KiCad-library-exception\",\n"
               << "      \"provenance\": \"kicad-official@abc123\",\n"
               << "      \"warnings\": []\n"
               << "    }\n"
               << "  ]\n"
               << "}\n";
  file_catalog.close();
  const std::string file_catalog_validate_command =
      quote(CCAD_BINARY) + " lib catalog-validate --catalog " + quote(file_catalog_path) +
      " --root " + quote(temp / "native") + " > " + quote(catalog_validate_path);
  require(run(file_catalog_validate_command) == 0,
          "lib catalog-validate root checksum exits zero");

  writeFile(native_artifact_path, "tampered\n");
  require(run(file_catalog_validate_command) != 0,
          "lib catalog-validate root checksum mismatch exits nonzero");
  require(readFile(catalog_validate_path).find("\"code\": \"ITEM_SHA256_MISMATCH\"") !=
              std::string::npos,
          "lib catalog-validate reports checksum mismatch");

  const std::string place_footprint_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(board_project_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component R1 --at-x-mm 10 --at-y-mm 12 --layer F.Cu";
  require(run(place_footprint_command) == 0, "pcb place-footprint exits zero");
  const std::string placed_footprint_project = readFile(board_project_path);
  require(placed_footprint_project.find("\"id\": \"R1.1\"") != std::string::npos,
          "pcb place-footprint writes first pad id");
  require(placed_footprint_project.find("\"component_id\": \"R1\"") != std::string::npos,
          "pcb place-footprint writes component id");
  require(placed_footprint_project.find("\"pin_name\": \"1\"") != std::string::npos,
          "pcb place-footprint writes pin name");
  require(placed_footprint_project.find("\"x_nm\": 9050000") != std::string::npos,
          "pcb place-footprint translates pad x");
  require(run(place_footprint_command) != 0, "pcb place-footprint rejects duplicate pad ids");

  const std::string bad_place_layer_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(board_project_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component R2 --at-x-mm 10 --at-y-mm 12 --layer Inner.Cu";
  require(run(bad_place_layer_command) != 0, "pcb place-footprint rejects unknown layer");

  const std::filesystem::path rotated_board_path = temp / "rotated-board.ccad.json";
  const std::string rotated_init_command =
      quote(CCAD_BINARY) +
      " init --name rotated-board --width-mm 42 --height-mm 28 --out " +
      quote(rotated_board_path);
  require(run(rotated_init_command) == 0, "rotated board init exits zero");
  const std::string rotated_place_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(rotated_board_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component R90 --at-x-mm 16 --at-y-mm 14 --layer F.Cu --rotation-deg 90";
  require(run(rotated_place_command) == 0, "pcb place-footprint rotation exits zero");
  const std::string rotated_project = readFile(rotated_board_path);
  require(rotated_project.find("\"id\": \"R90.1\"") != std::string::npos,
          "rotated placement writes first pad id");
  require(rotated_project.find("\"x_nm\": 16000000") != std::string::npos,
          "rotated placement transforms x");
  require(rotated_project.find("\"y_nm\": 13050000") != std::string::npos,
          "rotated placement transforms y");
  require(rotated_project.find("\"rotation_degrees\": 90") != std::string::npos,
          "rotated placement writes pad rotation");

  const std::filesystem::path mapped_board_path = temp / "mapped-board.ccad.json";
  std::ofstream mapped_board(mapped_board_path);
  mapped_board << "{\n"
               << "  \"schema_version\": 1,\n"
               << "  \"id\": \"proj-mapped-board\",\n"
               << "  \"name\": \"mapped-board\",\n"
               << "  \"board\": {\n"
               << "    \"outline\": {\n"
               << "      \"x_nm\": 0,\n"
               << "      \"y_nm\": 0,\n"
               << "      \"width_nm\": 42000000,\n"
               << "      \"height_nm\": 28000000\n"
               << "    },\n"
               << "    \"layers\": [\n"
               << "      {\"id\": \"F.Cu\", \"name\": \"Front copper\", \"kind\": \"copper\", \"visible\": true},\n"
               << "      {\"id\": \"B.Cu\", \"name\": \"Back copper\", \"kind\": \"copper\", \"visible\": true}\n"
               << "    ],\n"
               << "    \"pads\": [],\n"
               << "    \"vias\": [],\n"
               << "    \"tracks\": []\n"
               << "  },\n"
               << "  \"components\": [\n"
               << "    {\"id\": \"RMAP\", \"part\": \"R\", \"pins\": [\n"
               << "      {\"name\": \"1\", \"kind\": \"passive\"}\n"
               << "    ]}\n"
               << "  ],\n"
               << "  \"constraints\": [],\n"
               << "  \"nets\": [\n"
               << "    {\"id\": \"N_SIGNAL\", \"members\": [\n"
               << "      {\"component_id\": \"RMAP\", \"pin_name\": \"1\"}\n"
               << "    ]}\n"
               << "  ]\n"
               << "}\n";
  mapped_board.close();
  const std::string mapped_place_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(mapped_board_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component RMAP --at-x-mm 16 --at-y-mm 14 --layer F.Cu";
  require(run(mapped_place_command) == 0, "pcb place-footprint mapped net exits zero");
  const std::string mapped_project = readFile(mapped_board_path);
  require(mapped_project.find("\"id\": \"RMAP.1\"") != std::string::npos,
          "mapped placement writes first pad id");
  require(mapped_project.find("\"net_id\": \"N_SIGNAL\"") != std::string::npos,
          "mapped placement assigns logical net id");

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
