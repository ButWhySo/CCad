#include "test_support.hpp"

#include <cstdio>
#include <cstdlib>
#include <chrono>
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

std::string shellLiteral(const std::string& value) {
#ifdef _WIN32
  return value;
#else
  std::string escaped;
  escaped.reserve(value.size() + 8);
  for (const char character : value) {
    if (character == '$') escaped.push_back('\\');
    escaped.push_back(character);
  }
  return escaped;
#endif
}

std::string shellArgument(const std::string& value) {
#ifdef _WIN32
  return "\"" + value + "\"";
#else
  std::string escaped = "'";
  for (const char character : value) {
    escaped += character == '\'' ? "'\\''" : std::string(1, character);
  }
  return escaped + "'";
#endif
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
  const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path temp =
      std::filesystem::temp_directory_path() / ("ccad_cli_test_" + std::to_string(nonce));
  std::filesystem::remove_all(temp);
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
  require(help_json.find("\"name\": \"pcb autoplace-footprint\"") != std::string::npos,
          "help json describes footprint autoplacement");
  require(help_json.find("\"name\": \"pcb spread-footprints\"") != std::string::npos,
          "help json describes footprint spreading");
  require(help_json.find("\"name\": \"sch place-symbol\"") != std::string::npos,
          "help json describes schematic symbol placement");
  require(help_json.find("\"name\": \"pcb add-layer\"") != std::string::npos,
          "help json describes layer authoring");
  require(help_json.find("\"name\": \"pcb add-standard-layers\"") != std::string::npos,
          "help json describes KiCad standard layer authoring");
  require(help_json.find("--max-track-width-mm") != std::string::npos,
          "help documents maximum track width rule");
  require(help_json.find("--max-via-diameter-mm") != std::string::npos,
          "help documents maximum via diameter rule");
  require(help_json.find("\"name\": \"pcb set-layer\"") != std::string::npos,
          "help json describes layer metadata editing");
  require(help_json.find("\"name\": \"pcb get-object\"") != std::string::npos,
          "help json describes pcb object lookup");
  require(help_json.find("\"name\": \"pcb load-state\"") != std::string::npos,
          "help json describes KiCad-style board loader state");
  require(help_json.find("\"name\": \"pcb drill-statistics\"") != std::string::npos,
          "help json describes KiCad-style board drill statistics");
  require(help_json.find("\"name\": \"pcb board-statistics\"") != std::string::npos,
          "help json describes KiCad-style board statistics report");
  require(help_json.find("\"name\": \"pcb export-board-bom\"") != std::string::npos,
          "help json describes KiCad-style board BOM export");
  require(help_json.find("\"name\": \"pcb cleanup-actions\"") != std::string::npos,
          "help json describes KiCad-style cleanup action catalog");
  require(help_json.find("\"name\": \"pcb collect-items\"") != std::string::npos,
          "help json describes KiCad-style board item collector");
  require(help_json.find("\"name\": \"pcb list-objects\"") != std::string::npos,
          "help json describes pcb object listing");
  require(help_json.find("\"name\": \"pcb list-nets\"") != std::string::npos,
          "help json describes pcb net listing");
  require(help_json.find("\"name\": \"pcb list-by-net\"") != std::string::npos,
          "help json describes KiCad-style PCB items-by-net listing");
  require(help_json.find("\"name\": \"pcb list-connected\"") != std::string::npos,
          "help json describes KiCad-style connected item listing");
  require(help_json.find("\"name\": \"pcb list-route-requests\"") != std::string::npos,
          "help json describes route request listing");
  require(help_json.find("\"name\": \"pcb route-status\"") != std::string::npos,
          "help json describes route status reporting");
  require(help_json.find("\"name\": \"pcb export-route-job\"") != std::string::npos,
          "help json describes route job export");
  require(help_json.find("\"name\": \"pcb remove-layer\"") != std::string::npos,
          "help json describes layer removal");
  require(help_json.find("\"name\": \"pcb set-layer-visibility\"") != std::string::npos,
          "help json describes layer visibility authoring");
  require(help_json.find("\"name\": \"pcb list-enabled-layers\"") != std::string::npos,
          "help json describes enabled layer listing");
  require(help_json.find("\"name\": \"pcb list-visible-layers\"") != std::string::npos,
          "help json describes visible layer listing");
  require(help_json.find("\"name\": \"pcb get-layer-name\"") != std::string::npos,
          "help json describes layer name lookup");
  require(help_json.find("\"name\": \"pcb get-board-stackup\"") != std::string::npos,
          "help json describes board stackup lookup");
  require(help_json.find("\"name\": \"pcb get-rules\"") != std::string::npos,
          "help json describes DRC rule lookup");
  require(help_json.find("\"name\": \"pcb set-rules\"") != std::string::npos,
          "help json describes drc rule authoring");
  require(help_json.find("\"name\": \"pcb get-outline\"") != std::string::npos,
          "help json describes outline lookup");
  require(help_json.find("\"name\": \"pcb outline-polygon\"") != std::string::npos,
          "help json describes Edge.Cuts outline polygon reporting");
  require(help_json.find("\"name\": \"pcb cross-probe\"") != std::string::npos,
          "help json describes KiCad-style cross-probe packet reporting");
  require(help_json.find("\"name\": \"pcb set-outline\"") != std::string::npos,
          "help json describes outline authoring");
  require(help_json.find("\"name\": \"pcb set-track\"") != std::string::npos,
          "help json describes track editing");
  require(help_json.find("\"name\": \"pcb add-graphic-line\"") != std::string::npos,
          "help json describes board graphic line authoring");
  require(help_json.find("\"name\": \"pcb add-text\"") != std::string::npos,
          "help json describes board text authoring");
  require(help_json.find("\"name\": \"pcb expand-text-variables\"") != std::string::npos,
          "help json describes board text variable expansion");
  require(help_json.find("\"name\": \"project set-text-variable\"") != std::string::npos,
          "help json describes project text variable mutation");
  require(help_json.find("\"name\": \"project list-text-variables\"") != std::string::npos,
          "help json describes project text variable listing");
  require(help_json.find("\"name\": \"pcb add-route-request\"") != std::string::npos,
          "help json describes route request authoring");
  require(help_json.find("\"name\": \"pcb set-route-request\"") != std::string::npos,
          "help json describes route request editing");
  require(help_json.find("\"name\": \"pcb remove-route-request\"") != std::string::npos,
          "help json describes route request removal");
  require(help_json.find("\"name\": \"pcb apply-route-segment\"") != std::string::npos,
          "help json describes route segment application");
  require(help_json.find("\"name\": \"pcb apply-route-polyline\"") != std::string::npos,
          "help json describes route polyline application");
  require(help_json.find("\"name\": \"pcb add-keepout\"") != std::string::npos,
          "help json describes keepout authoring");
  require(help_json.find("\"name\": \"pcb set-pad\"") != std::string::npos,
          "help json describes pad editing");
  require(help_json.find("\"name\": \"pcb set-via\"") != std::string::npos,
          "help json describes via editing");
  require(help_json.find("\"name\": \"pcb add-placement-region\"") != std::string::npos,
          "help json describes placement region authoring");
  require(help_json.find("\"name\": \"pcb set-region-kind\"") != std::string::npos,
          "help json describes region kind editing");
  require(help_json.find("\"name\": \"pcb remove-object\"") != std::string::npos,
          "help json describes physical object removal");
  require(help_json.find("\"name\": \"pcb move-object\"") != std::string::npos,
          "help json describes physical object movement");
  require(help_json.find("\"name\": \"pcb resize-object\"") != std::string::npos,
          "help json describes physical object resizing");
  require(help_json.find("\"name\": \"lib catalog-info\"") != std::string::npos,
          "help json describes catalog info");
  require(help_json.find("\"name\": \"lib catalog-find\"") != std::string::npos,
          "help json describes catalog lookup");
  require(help_json.find("\"name\": \"lib catalog-search\"") != std::string::npos,
          "help json describes catalog search");
  require(help_json.find("--rotation-deg") != std::string::npos,
          "help json exposes rotation option");

  const std::filesystem::path schematic_project_path = temp / "schematic.ccad.json";
  require(run(quote(CCAD_BINARY) + " init --name schematic --out " +
              quote(schematic_project_path)) == 0,
          "schematic project init exits zero");
  const std::filesystem::path diode_symbol_path = temp / "diode.ccad-symbol.json";
  writeFile(diode_symbol_path,
            "{\n"
            "  \"name\": \"D\",\n"
            "  \"extends\": \"\",\n"
            "  \"pins\": [\n"
            "    {\n"
            "      \"name\": \"A\",\n"
            "      \"number\": \"1\",\n"
            "      \"electrical_type\": \"passive\",\n"
            "      \"graphical_style\": \"line\",\n"
            "      \"x_nm\": -3540000,\n"
            "      \"y_nm\": 0,\n"
            "      \"rotation_degrees\": 0,\n"
            "      \"length_nm\": 2540000\n"
            "    }\n"
            "  ],\n"
            "  \"properties\": [],\n"
            "  \"rectangles\": [\n"
            "    {\"start_x\": -1000000, \"start_y\": -1000000, \"end_x\": 1000000, "
            "\"end_y\": 1000000, \"stroke_width\": 120000, \"fill_type\": \"none\"}\n"
            "  ],\n"
            "  \"lines\": [\n"
            "    {\"start_x\": -1000000, \"start_y\": 0, \"end_x\": 1000000, "
            "\"end_y\": 0, \"stroke_width\": 120000}\n"
            "  ],\n"
            "  \"arcs\": [],\n"
            "  \"circles\": [],\n"
            "  \"polylines\": [],\n"
            "  \"texts\": []\n"
            "}\n");
  const std::string place_symbol_command =
      quote(CCAD_BINARY) + " sch place-symbol --file " + quote(schematic_project_path) +
      " --symbol " + quote(diode_symbol_path) +
      " --component D1 --at-x-mm 20 --at-y-mm 15 --rotation-deg 90";
  require(run(place_symbol_command) == 0, "sch place-symbol exits zero");
  const std::string placed_symbol_project = readFile(schematic_project_path);
  require(placed_symbol_project.find("\"id\": \"D1\"") != std::string::npos,
          "sch place-symbol writes component id");
  require(placed_symbol_project.find("\"symbol\"") != std::string::npos,
          "sch place-symbol writes symbol snapshot");
  require(placed_symbol_project.find("\"length_nm\": 2540000") != std::string::npos,
          "sch place-symbol preserves pin lead length");
  require(run(place_symbol_command) != 0, "sch place-symbol rejects duplicate component id");

  require(run(quote(CCAD_BINARY) + " sch add-wire --file " + quote(schematic_project_path) +
              " --id W1 --start-x-mm 1 --start-y-mm 2 --end-x-mm 3 --end-y-mm 4 --net N1") == 0,
          "sch add-wire exits zero");
  require(run(quote(CCAD_BINARY) + " sch add-bus --file " + quote(schematic_project_path) +
              " --id B1 --start-x-mm 2 --start-y-mm 3 --end-x-mm 4 --end-y-mm 5 --bus BUS1") == 0,
          "sch add-bus exits zero");
  require(run(quote(CCAD_BINARY) + " sch add-label --file " + quote(schematic_project_path) +
              " --id L1 --text DATA --at-x-mm 5 --at-y-mm 6 --net N1") == 0,
          "sch add-label exits zero");
  require(run(quote(CCAD_BINARY) + " sch add-power --file " + quote(schematic_project_path) +
              " --id PWR1 --value VCC --at-x-mm 7 --at-y-mm 8 --net VCC") == 0,
          "sch add-power exits zero");
  const std::string schematic_mutations = readFile(schematic_project_path);
  require(schematic_mutations.find("\"id\": \"W1\"") != std::string::npos,
          "sch add-wire writes wire id");
  require(schematic_mutations.find("\"id\": \"B1\"") != std::string::npos,
          "sch add-bus writes bus id");
  require(schematic_mutations.find("\"id\": \"L1\"") != std::string::npos,
          "sch add-label writes label id");
  require(schematic_mutations.find("\"id\": \"PWR1\"") != std::string::npos,
          "sch add-power writes power symbol id");
  require(run(quote(CCAD_BINARY) + " sch annotate --file " + quote(schematic_project_path) +
              " --algo sequential --order y --start 1") == 0,
          "sch annotate exits zero");
  require(run(quote(CCAD_BINARY) + " sch autoplace --file " + quote(schematic_project_path)) == 0,
          "sch autoplace exits zero");
  require(run(quote(CCAD_BINARY) + " sch fix-junctions --file " + quote(schematic_project_path)) == 0,
          "sch fix-junctions exits zero");

  const std::filesystem::path board_project_path = temp / "board.ccad.json";
  const std::string board_init_command = quote(CCAD_BINARY) +
                                         " init --name board --width-mm 42 --height-mm 28 --out " +
                                         quote(board_project_path);
  require(run(board_init_command) == 0, "board init exits zero");
  const std::string board_json = readFile(board_project_path);
  require(board_json.find("\"board\"") != std::string::npos, "board init writes board");
  require(board_json.find("\"width_nm\": 42000000") != std::string::npos,
          "board init writes width");
  const std::filesystem::path board_load_state_path = temp / "board-load-state.json";
  require(run(quote(CCAD_BINARY) + " pcb load-state --file " + quote(board_project_path) +
              " > " + quote(board_load_state_path)) == 0,
          "pcb load-state exits zero");
  const std::string board_load_state_json = readFile(board_load_state_path);
  require(board_load_state_json.find("\"kicad_class\": \"BOARD_LOADER\"") !=
              std::string::npos,
          "pcb load-state reports KiCad board loader class");
  require(board_load_state_json.find("\"source_format\": \"CCAD_JSON\"") != std::string::npos,
          "pcb load-state reports CCad JSON source format");
  require(board_load_state_json.find("\"board_attached\": true") != std::string::npos,
          "pcb load-state reports initialized board attachment");
  require(board_load_state_json.find("\"drc_ready\": true") != std::string::npos,
          "pcb load-state reports DRC readiness");
  require(board_load_state_json.find("\"connectivity_ready\": true") != std::string::npos,
          "pcb load-state reports connectivity readiness");
  require(board_load_state_json.find("\"pending_kicad_loader_steps\"") != std::string::npos,
          "pcb load-state reports remaining KiCad loader parity steps");
  const std::filesystem::path raw_board_load_state_path = temp / "board-load-state-raw.json";
  require(run(quote(CCAD_BINARY) + " pcb load-state --file " + quote(board_project_path) +
              " --initialize false > " + quote(raw_board_load_state_path)) == 0,
          "pcb load-state raw mode exits zero");
  const std::string raw_board_load_state_json = readFile(raw_board_load_state_path);
  require(raw_board_load_state_json.find("\"initialize_after_load\": false") !=
              std::string::npos,
          "pcb load-state raw mode reports disabled initialization");
  require(raw_board_load_state_json.find("\"board_attached\": false") != std::string::npos,
          "pcb load-state raw mode reports board not attached");

  const std::string add_layer_command =
      quote(CCAD_BINARY) + " pcb add-layer --file " + quote(board_project_path) +
      " --id In1.Cu --name Inner1 --kind copper --visible false";
  require(run(add_layer_command) == 0, "pcb add-layer exits zero");
  const std::string layer_json = readFile(board_project_path);
  require(layer_json.find("\"id\": \"In1.Cu\"") != std::string::npos,
          "pcb add-layer writes id");
  require(layer_json.find("\"name\": \"Inner1\"") != std::string::npos,
          "pcb add-layer writes name");
  require(layer_json.find("\"kind\": \"copper\"") != std::string::npos,
          "pcb add-layer writes kind");
  require(layer_json.find("\"visible\": false") != std::string::npos,
          "pcb add-layer writes visibility");
  require(run(add_layer_command) != 0, "pcb add-layer rejects duplicate id");

  const std::filesystem::path standard_layers_path = temp / "standard-layers.ccad.json";
  const std::string standard_layers_init_command =
      quote(CCAD_BINARY) +
      " init --name standard-layers --width-mm 42 --height-mm 28 --out " +
      quote(standard_layers_path);
  require(run(standard_layers_init_command) == 0, "standard layer board init exits zero");
  const std::string add_standard_layers_command =
      quote(CCAD_BINARY) + " pcb add-standard-layers --file " + quote(standard_layers_path);
  require(run(add_standard_layers_command) == 0, "pcb add-standard-layers exits zero");
  const std::string standard_layers_json = readFile(standard_layers_path);
  require(standard_layers_json.find("\"id\": \"In30.Cu\"") != std::string::npos,
          "pcb add-standard-layers writes final inner copper layer");
  require(standard_layers_json.find("\"id\": \"F.Paste\"") != std::string::npos,
          "pcb add-standard-layers writes front paste layer");
  require(standard_layers_json.find("\"kind\": \"board_edge\"") != std::string::npos,
          "pcb add-standard-layers writes board edge kind");
  require(standard_layers_json.find("\"id\": \"User.9\"") != std::string::npos,
          "pcb add-standard-layers writes final user layer");
  require(run(add_standard_layers_command) == 0,
          "pcb add-standard-layers is idempotent for already-complete boards");

  const std::string add_graphic_line_command =
      quote(CCAD_BINARY) + " pcb add-graphic-line --file " + quote(standard_layers_path) +
      " --id G1 --layer Dwgs.User --start-x-mm 3 --start-y-mm 4"
      " --end-x-mm 16 --end-y-mm 4 --width-mm 0.15";
  require(run(add_graphic_line_command) == 0, "pcb add-graphic-line exits zero");
  const std::string add_board_text_command =
      quote(CCAD_BINARY) + " pcb add-text --file " + quote(standard_layers_path) +
      " --id BT1 --layer F.SilkS --text \"Bridge rectifier\" --x-mm 8 --y-mm 22"
      " --size-x-mm 1.5 --size-y-mm 1.5 --rotation-deg 90";
  require(run(add_board_text_command) == 0, "pcb add-text exits zero");
  const std::string graphic_text_json = readFile(standard_layers_path);
  require(graphic_text_json.find("\"graphics\"") != std::string::npos,
          "pcb add-graphic-line writes graphics array");
  require(graphic_text_json.find("\"id\": \"G1\"") != std::string::npos,
          "pcb add-graphic-line writes graphic id");
  require(graphic_text_json.find("\"layer_id\": \"Dwgs.User\"") != std::string::npos,
          "pcb add-graphic-line writes graphic layer");
  require(graphic_text_json.find("\"texts\"") != std::string::npos,
          "pcb add-text writes texts array");
  require(graphic_text_json.find("\"text\": \"Bridge rectifier\"") != std::string::npos,
          "pcb add-text writes text value");
  require(graphic_text_json.find("\"rotation_degrees\": 90") != std::string::npos,
          "pcb add-text writes rotation");
  const std::filesystem::path graphic_lookup_path = temp / "graphic-lookup.json";
  require(run(quote(CCAD_BINARY) + " pcb get-object --file " + quote(standard_layers_path) +
              " --id G1 > " + quote(graphic_lookup_path)) == 0,
          "pcb get-object finds board graphic line");
  const std::string graphic_lookup_json = readFile(graphic_lookup_path);
  require(graphic_lookup_json.find("\"type\": \"graphic\"") != std::string::npos,
          "pcb get-object writes graphic type");
  require(graphic_lookup_json.find("\"kind\": \"line\"") != std::string::npos,
          "pcb get-object writes graphic kind");
  require(graphic_lookup_json.find("\"layer_id\": \"Dwgs.User\"") != std::string::npos,
          "pcb get-object writes graphic layer");
  require(graphic_lookup_json.find("\"width_nm\": 150000") != std::string::npos,
          "pcb get-object writes graphic width");
  const std::filesystem::path board_text_lookup_path = temp / "board-text-lookup.json";
  require(run(quote(CCAD_BINARY) + " pcb get-object --file " + quote(standard_layers_path) +
              " --id BT1 > " + quote(board_text_lookup_path)) == 0,
          "pcb get-object finds board text");
  const std::string board_text_lookup_json = readFile(board_text_lookup_path);
  require(board_text_lookup_json.find("\"type\": \"text\"") != std::string::npos,
          "pcb get-object writes text type");
  require(board_text_lookup_json.find("\"text\": \"Bridge rectifier\"") != std::string::npos,
          "pcb get-object writes text payload");
  require(board_text_lookup_json.find("\"layer_id\": \"F.SilkS\"") != std::string::npos,
          "pcb get-object writes text layer");
  require(board_text_lookup_json.find("\"rotation_degrees\": 90") != std::string::npos,
          "pcb get-object writes text rotation");
  const std::filesystem::path graphic_list_path = temp / "graphic-list.json";
  require(run(quote(CCAD_BINARY) + " pcb list-objects --file " + quote(standard_layers_path) +
              " --type graphic > " + quote(graphic_list_path)) == 0,
          "pcb list-objects filters board graphics");
  const std::string graphic_list_json = readFile(graphic_list_path);
  require(graphic_list_json.find("\"total\": 1") != std::string::npos,
          "pcb list-objects graphic summary reports one row");
  require(graphic_list_json.find("\"type\": \"graphic\"") != std::string::npos,
          "pcb list-objects writes graphic row");
  require(graphic_list_json.find("\"kind\": \"line\"") != std::string::npos,
          "pcb list-objects writes graphic kind");
  require(graphic_list_json.find("\"width_nm\": 150000") != std::string::npos,
          "pcb list-objects writes graphic width");
  const std::filesystem::path board_text_list_path = temp / "board-text-list.json";
  require(run(quote(CCAD_BINARY) + " pcb list-objects --file " + quote(standard_layers_path) +
              " --type text > " + quote(board_text_list_path)) == 0,
          "pcb list-objects filters board texts");
  const std::string board_text_list_json = readFile(board_text_list_path);
  require(board_text_list_json.find("\"total\": 1") != std::string::npos,
          "pcb list-objects text summary reports one row");
  require(board_text_list_json.find("\"type\": \"text\"") != std::string::npos,
          "pcb list-objects writes text row");
  require(board_text_list_json.find("\"text\": \"Bridge rectifier\"") != std::string::npos,
          "pcb list-objects writes text payload");
  require(board_text_list_json.find("\"rotation_degrees\": 90") != std::string::npos,
          "pcb list-objects writes text rotation");
  require(run(add_graphic_line_command) != 0, "pcb add-graphic-line rejects duplicate id");
  require(run(add_board_text_command) != 0, "pcb add-text rejects duplicate id");

  require(run(quote(CCAD_BINARY) + " project set-text-variable --file " +
              quote(standard_layers_path) + " --key REV --value A1") == 0,
          "project set-text-variable exits zero");
  require(run(quote(CCAD_BINARY) + " project set-text-variable --file " +
              quote(standard_layers_path) + " --key COMPANY --value CCad") == 0,
          "project set-text-variable stores second variable");
  const std::string board_text_arg = "Rev_${REV}_${UNKNOWN}";
  require(run(quote(CCAD_BINARY) + " pcb add-text --file " + quote(standard_layers_path) +
              " --id BT2 --layer F.SilkS --text " + shellLiteral(board_text_arg) +
              " --x-mm 10 --y-mm 22 --size-x-mm 1.5 --size-y-mm 1.5 --rotation-deg 0") == 0,
          "pcb add-text accepts variable-bearing board text");

  const std::string variable_project_json = readFile(standard_layers_path);
  require(variable_project_json.find("\"text_variables\"") != std::string::npos,
          "project set-text-variable writes text_variables object");
  require(variable_project_json.find("\"REV\": \"A1\"") != std::string::npos,
          "project set-text-variable writes REV value");

  const std::filesystem::path text_variables_path = temp / "text-variables.json";
  require(run(quote(CCAD_BINARY) + " project list-text-variables --file " +
              quote(standard_layers_path) + " > " + quote(text_variables_path)) == 0,
          "project list-text-variables exits zero");
  const std::string text_variables_json = readFile(text_variables_path);
  require(text_variables_json.find("\"kicad_handler\": \"GetTextVariables\"") !=
              std::string::npos,
          "project list-text-variables reports KiCad handler");
  require(text_variables_json.find("\"REV\": \"A1\"") != std::string::npos,
          "project list-text-variables lists REV");
  require(text_variables_json.find("\"COMPANY\": \"CCad\"") != std::string::npos,
          "project list-text-variables lists COMPANY");

  const std::filesystem::path expand_text_path = temp / "expand-text.json";
  const std::string text_arg = "Release_${REV}_${UNKNOWN}";
  require(run(quote(CCAD_BINARY) + " pcb expand-text-variables --file " +
              quote(standard_layers_path) + " --text " + shellLiteral(text_arg) + " > " +
              quote(expand_text_path)) == 0,
          "pcb expand-text-variables exits zero for explicit text");
  const std::string expand_text_json = readFile(expand_text_path);
  require(expand_text_json.find("\"kicad_handler\": \"ExpandTextVariables\"") !=
              std::string::npos,
          "pcb expand-text-variables reports KiCad handler");
  require(expand_text_json.find("\"expanded_text\": \"Release_A1_${UNKNOWN}\"") !=
              std::string::npos,
          "pcb expand-text-variables expands known explicit variables");
  require(expand_text_json.find("\"name\": \"UNKNOWN\"") != std::string::npos,
          "pcb expand-text-variables reports unknown token name");
  require(expand_text_json.find("\"resolved\": false") != std::string::npos,
          "pcb expand-text-variables marks unknown token unresolved");

  const std::filesystem::path expand_board_text_path = temp / "expand-board-text.json";
  require(run(quote(CCAD_BINARY) + " pcb expand-text-variables --file " +
              quote(standard_layers_path) + " > " + quote(expand_board_text_path)) == 0,
          "pcb expand-text-variables exits zero for board texts");
  const std::string expand_board_text_json = readFile(expand_board_text_path);
  require(expand_board_text_json.find("\"id\": \"BT2\"") != std::string::npos,
          "pcb expand-text-variables includes board text id");
  require(expand_board_text_json.find("\"expanded_text\": \"Rev_A1_${UNKNOWN}\"") !=
              std::string::npos,
          "pcb expand-text-variables expands board text variables");

  const std::filesystem::path advanced_pad_path = temp / "advanced-pad-authoring.ccad.json";
  require(run(quote(CCAD_BINARY) +
              " init --name advanced-pad-authoring --width-mm 42 --height-mm 28 --out " +
              quote(advanced_pad_path)) == 0,
          "advanced pad board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(advanced_pad_path) +
              " --id P2 --component U1 --pin 2 --net N1 --layers F.Cu"
              " --type smd --shape roundrect --roundrect-rratio 0.25"
              " --x-mm 12 --y-mm 6 --width-mm 1.2 --height-mm 1.0") == 0,
          "pcb add-pad accepts KiCad roundrect pad metadata");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(advanced_pad_path) +
              " --id P3 --component U1 --pin 3 --net N1 --layers F.Cu"
              " --type smd --shape chamfered_rect --chamfer-ratio 0.20"
              " --x-mm 14 --y-mm 6 --width-mm 1.2 --height-mm 1.0") == 0,
          "pcb add-pad accepts KiCad chamfered pad metadata");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(advanced_pad_path) +
              " --id P4 --component U1 --pin 4 --net N1 --layers *.Cu,*.Mask"
              " --type thru_hole --shape circle --drill-mm 0.7"
              " --x-mm 16 --y-mm 6 --width-mm 1.5 --height-mm 1.5") == 0,
          "pcb add-pad accepts KiCad through-hole pad metadata");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(advanced_pad_path) +
              " --id P5 --component U1 --pin 5 --net N1 --layers *.Cu,*.Mask"
              " --type thru_hole --shape circle --drill-mm 0.7"
              " --x-mm 18 --y-mm 6 --width-mm 1.5 --height-mm 1.5") == 0,
          "pcb add-pad accepts second identical KiCad through-hole pad for statistics");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(advanced_pad_path) +
              " --id MH1 --component MH1 --pin MH --net N_MH --layers *.Mask"
              " --type np_thru_hole --shape circle --drill-mm 1.1"
              " --x-mm 22 --y-mm 6 --width-mm 2.0 --height-mm 2.0") == 0,
          "pcb add-pad accepts non-plated mounting hole metadata for statistics");
  require(run(quote(CCAD_BINARY) + " pcb add-via --file " + quote(advanced_pad_path) +
              " --id V_STAT --net N1 --x-mm 24 --y-mm 6 --diameter-mm 0.8 --drill-mm 0.4") ==
              0,
          "pcb add-via accepts KiCad drill statistics fixture via");
  const std::string advanced_pad_json = readFile(advanced_pad_path);
  require(advanced_pad_json.find("\"shape\": \"roundrect\"") != std::string::npos,
          "pcb add-pad writes roundrect shape");
  require(advanced_pad_json.find("\"roundrect_rratio\": 0.25") != std::string::npos,
          "pcb add-pad writes roundrect ratio");
  require(advanced_pad_json.find("\"shape\": \"chamfered_rect\"") != std::string::npos,
          "pcb add-pad writes chamfered shape");
  require(advanced_pad_json.find("\"chamfer_ratio\": 0.2") != std::string::npos,
          "pcb add-pad writes chamfer ratio");
  require(advanced_pad_json.find("\"type\": \"thru_hole\"") != std::string::npos,
          "pcb add-pad writes through-hole type");
  require(advanced_pad_json.find("\"width_nm\": 700000") != std::string::npos,
          "pcb add-pad writes through-hole drill");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(advanced_pad_path) +
              " --id P_BAD --component U1 --pin 9 --net N1 --layers F.Cu"
              " --shape roundrect --roundrect-rratio 0.75"
              " --x-mm 18 --y-mm 6 --width-mm 1.0 --height-mm 1.0") != 0,
          "pcb add-pad rejects invalid roundrect ratio");

  const std::filesystem::path advanced_pad_lookup_path = temp / "advanced-pad-lookup.json";
  require(run(quote(CCAD_BINARY) + " pcb get-object --file " + quote(advanced_pad_path) +
              " --id P4 > " + quote(advanced_pad_lookup_path)) == 0,
          "pcb get-object finds KiCad through-hole pad metadata");
  const std::string advanced_pad_lookup_json = readFile(advanced_pad_lookup_path);
  require(advanced_pad_lookup_json.find("\"pad_type\": \"thru_hole\"") != std::string::npos,
          "pcb get-object writes KiCad pad type");
  require(advanced_pad_lookup_json.find("\"shape\": \"circle\"") != std::string::npos,
          "pcb get-object writes KiCad pad shape");
  require(advanced_pad_lookup_json.find("\"drill_nm\": 700000") != std::string::npos,
          "pcb get-object writes KiCad pad drill");
  require(advanced_pad_lookup_json.find("\"*.Cu\"") != std::string::npos,
          "pcb get-object writes KiCad wildcard pad layer");

  const std::filesystem::path advanced_pad_list_path = temp / "advanced-pad-list.json";
  require(run(quote(CCAD_BINARY) + " pcb list-objects --file " + quote(advanced_pad_path) +
              " --type pad > " + quote(advanced_pad_list_path)) == 0,
          "pcb list-objects lists KiCad pad metadata");
  const std::string advanced_pad_list_json = readFile(advanced_pad_list_path);
  require(advanced_pad_list_json.find("\"pad_type\": \"smd\"") != std::string::npos,
          "pcb list-objects writes compact pad type");
  require(advanced_pad_list_json.find("\"shape\": \"roundrect\"") != std::string::npos,
          "pcb list-objects writes compact roundrect shape");
  require(advanced_pad_list_json.find("\"roundrect_rratio\": 0.25") != std::string::npos,
          "pcb list-objects writes compact roundrect ratio");
  require(advanced_pad_list_json.find("\"shape\": \"chamfered_rect\"") != std::string::npos,
          "pcb list-objects writes compact chamfered shape");
  require(advanced_pad_list_json.find("\"chamfer_ratio\": 0.2") != std::string::npos,
          "pcb list-objects writes compact chamfer ratio");
  require(advanced_pad_list_json.find("\"drill_nm\": 700000") != std::string::npos,
          "pcb list-objects writes compact drill size");
  require(advanced_pad_list_json.find("\"resolved_layers\": [\"F.Cu\", \"B.Cu\", \"*.Mask\"]") !=
              std::string::npos,
          "pcb list-objects resolves KiCad wildcard pad layer set in board context");
  require(advanced_pad_list_json.find("\"kicad_layer_numbers\": [0, 31]") !=
              std::string::npos,
          "pcb list-objects writes KiCad layer numbers for resolved pad layer set");

  const std::filesystem::path drill_statistics_path = temp / "drill-statistics.json";
  require(run(quote(CCAD_BINARY) + " pcb drill-statistics --file " +
              quote(advanced_pad_path) + " > " + quote(drill_statistics_path)) == 0,
          "pcb drill-statistics exits zero");
  const std::string drill_statistics_json = readFile(drill_statistics_path);
  require(drill_statistics_json.find("\"kicad_reference\": \"board_statistics\"") !=
              std::string::npos,
          "pcb drill-statistics reports KiCad board statistics reference");
  require(drill_statistics_json.find("\"unique_drill_rows\": 3") != std::string::npos,
          "pcb drill-statistics reports unique drill row count");
  require(drill_statistics_json.find("\"total_drill_count\": 4") != std::string::npos,
          "pcb drill-statistics reports total drill count");
  require(drill_statistics_json.find("\"count\": 2") != std::string::npos,
          "pcb drill-statistics aggregates identical through-hole pads");
  require(drill_statistics_json.find("\"x_size_nm\": 700000") != std::string::npos,
          "pcb drill-statistics writes through-hole drill X size");
  require(drill_statistics_json.find("\"shape\": \"Round\"") != std::string::npos,
          "pcb drill-statistics writes KiCad drill shape label");
  require(drill_statistics_json.find("\"plated\": true") != std::string::npos,
          "pcb drill-statistics writes plated status");
  require(drill_statistics_json.find("\"source\": \"Pad\"") != std::string::npos,
          "pcb drill-statistics writes pad source");
  require(drill_statistics_json.find("\"start_layer\": \"F.Cu\"") != std::string::npos,
          "pcb drill-statistics writes top copper start layer");
  require(drill_statistics_json.find("\"stop_layer\": \"B.Cu\"") != std::string::npos,
          "pcb drill-statistics writes bottom copper stop layer");
  require(drill_statistics_json.find("\"plated\": false") != std::string::npos,
          "pcb drill-statistics writes non-plated hole status");
  require(drill_statistics_json.find("\"start_layer\": null") != std::string::npos,
          "pcb drill-statistics writes null layer span for non-copper NPTH hole");
  require(drill_statistics_json.find("\"source\": \"Via\"") != std::string::npos,
          "pcb drill-statistics writes via source");

  require(run(quote(CCAD_BINARY) + " pcb add-track --file " + quote(advanced_pad_path) +
              " --id STAT_T1 --net N1 --layer F.Cu --start-x-mm 12 --start-y-mm 8 "
              "--end-x-mm 24 --end-y-mm 8 --width-mm 0.18") == 0,
          "board statistics fixture add track exits zero");
  const std::filesystem::path board_statistics_path = temp / "board-statistics.json";
  require(run(quote(CCAD_BINARY) + " pcb board-statistics --file " +
              quote(advanced_pad_path) + " > " + quote(board_statistics_path)) == 0,
          "pcb board-statistics exits zero");
  const std::string board_statistics_json = readFile(board_statistics_path);
  require(board_statistics_json.find("\"kicad_reference\": \"board_statistics_report\"") !=
              std::string::npos,
          "pcb board-statistics reports KiCad report reference");
  require(board_statistics_json.find("\"parity_scope\": \"summary_report_first_slice\"") !=
              std::string::npos,
          "pcb board-statistics declares first-slice report scope");
  require(board_statistics_json.find("\"board_area_square_mm\": 1176") != std::string::npos,
          "pcb board-statistics reports rectangular board area");
  require(board_statistics_json.find("\"pad_count\": 5") != std::string::npos,
          "pcb board-statistics reports pad count");
  require(board_statistics_json.find("\"smd_pad_count\": 2") != std::string::npos,
          "pcb board-statistics reports SMD pad count");
  require(board_statistics_json.find("\"through_hole_pad_count\": 2") != std::string::npos,
          "pcb board-statistics reports through-hole pad count");
  require(board_statistics_json.find("\"npth_pad_count\": 1") != std::string::npos,
          "pcb board-statistics reports non-plated pad count");
  require(board_statistics_json.find("\"via_count\": 1") != std::string::npos,
          "pcb board-statistics reports via count");
  require(board_statistics_json.find("\"track_count\": 1") != std::string::npos,
          "pcb board-statistics reports track count");
  require(board_statistics_json.find("\"min_track_width_nm\": 180000") != std::string::npos,
          "pcb board-statistics reports minimum track width");
  require(board_statistics_json.find("\"min_drill_diameter_nm\": 400000") !=
              std::string::npos,
          "pcb board-statistics reports minimum drill diameter");
  require(board_statistics_json.find("\"drill_holes\"") != std::string::npos,
          "pcb board-statistics includes drill table rows");

  const std::filesystem::path advanced_pad_route_job_path =
      temp / "advanced-pad-route-job.json";
  require(run(quote(CCAD_BINARY) + " pcb export-route-job --file " +
              quote(advanced_pad_path) + " > " + quote(advanced_pad_route_job_path)) == 0,
          "pcb export-route-job exports KiCad pad metadata fixture");
  const std::string advanced_pad_route_job_json = readFile(advanced_pad_route_job_path);
  require(advanced_pad_route_job_json.find("\"pad_type\": \"smd\"") != std::string::npos,
          "pcb export-route-job writes pad type metadata");
  require(advanced_pad_route_job_json.find("\"shape\": \"roundrect\"") != std::string::npos,
          "pcb export-route-job writes roundrect pad shape");
  require(advanced_pad_route_job_json.find("\"roundrect_rratio\": 0.25") !=
              std::string::npos,
          "pcb export-route-job writes roundrect ratio");
  require(advanced_pad_route_job_json.find("\"shape\": \"chamfered_rect\"") !=
              std::string::npos,
          "pcb export-route-job writes chamfered pad shape");
  require(advanced_pad_route_job_json.find("\"chamfer_ratio\": 0.2") != std::string::npos,
          "pcb export-route-job writes chamfer ratio");
  require(advanced_pad_route_job_json.find("\"pad_type\": \"thru_hole\"") !=
              std::string::npos,
          "pcb export-route-job writes through-hole pad type");
  require(advanced_pad_route_job_json.find("\"drill_nm\": 700000") != std::string::npos,
          "pcb export-route-job writes through-hole pad drill");
  require(advanced_pad_route_job_json.find(
              "\"resolved_layers\": [\"F.Cu\", \"B.Cu\", \"*.Mask\"]") != std::string::npos,
          "pcb export-route-job resolves KiCad wildcard pad layer set in board context");
  require(advanced_pad_route_job_json.find("\"kicad_layer_numbers\": [0, 31]") !=
              std::string::npos,
          "pcb export-route-job writes KiCad layer numbers for resolved pad layer set");

  const std::string set_layer_visibility_command =
      quote(CCAD_BINARY) + " pcb set-layer-visibility --file " + quote(board_project_path) +
      " --id In1.Cu --visible true";
  require(run(set_layer_visibility_command) == 0, "pcb set-layer-visibility exits zero");
  const std::string visible_layer_json = readFile(board_project_path);
  require(visible_layer_json.find("\"id\": \"In1.Cu\"") != std::string::npos,
          "pcb set-layer-visibility preserves layer id");
  require(visible_layer_json.find("\"visible\": true") != std::string::npos,
          "pcb set-layer-visibility writes true visibility");
  const std::string set_layer_command =
      quote(CCAD_BINARY) + " pcb set-layer --file " + quote(board_project_path) +
      " --id In1.Cu --name InnerSignal --kind copper --visible false";
  require(run(set_layer_command) == 0, "pcb set-layer exits zero");
  const std::string edited_layer_json = readFile(board_project_path);
  require(edited_layer_json.find("\"id\": \"In1.Cu\"") != std::string::npos,
          "pcb set-layer preserves layer id");
  require(edited_layer_json.find("\"name\": \"InnerSignal\"") != std::string::npos,
          "pcb set-layer writes name");
  require(edited_layer_json.find("\"kind\": \"copper\"") != std::string::npos,
          "pcb set-layer writes kind");
  require(edited_layer_json.find("\"visible\": false") != std::string::npos,
          "pcb set-layer writes visibility");

  const std::filesystem::path enabled_layers_path = temp / "enabled-layers.json";
  require(run(quote(CCAD_BINARY) + " pcb list-enabled-layers --file " +
              quote(board_project_path) + " > " + quote(enabled_layers_path)) == 0,
          "pcb list-enabled-layers exits zero");
  const std::string enabled_layers_json = readFile(enabled_layers_path);
  require(enabled_layers_json.find("\"query_kind\": \"enabled_layers\"") != std::string::npos,
          "pcb list-enabled-layers reports query kind");
  require(enabled_layers_json.find("\"total\": 3") != std::string::npos,
          "pcb list-enabled-layers reports all board layers");
  require(enabled_layers_json.find("\"copper_count\": 3") != std::string::npos,
          "pcb list-enabled-layers counts copper layers");
  require(enabled_layers_json.find("\"visible_count\": 2") != std::string::npos,
          "pcb list-enabled-layers counts visible layers");
  require(enabled_layers_json.find("\"hidden_count\": 1") != std::string::npos,
          "pcb list-enabled-layers counts hidden layers");
  require(enabled_layers_json.find("\"id\": \"In1.Cu\"") != std::string::npos,
          "pcb list-enabled-layers includes hidden enabled layer");
  require(enabled_layers_json.find("\"kicad_layer_number\": 1") != std::string::npos,
          "pcb list-enabled-layers includes KiCad layer numbers");

  const std::filesystem::path visible_layers_path = temp / "visible-layers.json";
  require(run(quote(CCAD_BINARY) + " pcb list-visible-layers --file " +
              quote(board_project_path) + " > " + quote(visible_layers_path)) == 0,
          "pcb list-visible-layers exits zero");
  const std::string visible_layers_json = readFile(visible_layers_path);
  require(visible_layers_json.find("\"query_kind\": \"visible_layers\"") != std::string::npos,
          "pcb list-visible-layers reports query kind");
  require(visible_layers_json.find("\"total\": 2") != std::string::npos,
          "pcb list-visible-layers reports visible layers only");
  require(visible_layers_json.find("\"id\": \"F.Cu\"") != std::string::npos,
          "pcb list-visible-layers includes front copper");
  require(visible_layers_json.find("\"id\": \"B.Cu\"") != std::string::npos,
          "pcb list-visible-layers includes back copper");
  require(visible_layers_json.find("\"id\": \"In1.Cu\"") == std::string::npos,
          "pcb list-visible-layers excludes hidden layer rows");

  const std::filesystem::path layer_name_path = temp / "layer-name.json";
  require(run(quote(CCAD_BINARY) + " pcb get-layer-name --file " + quote(board_project_path) +
              " --id In1.Cu > " + quote(layer_name_path)) == 0,
          "pcb get-layer-name exits zero");
  const std::string layer_name_json = readFile(layer_name_path);
  require(layer_name_json.find("\"query_kind\": \"layer_name\"") != std::string::npos,
          "pcb get-layer-name reports query kind");
  require(layer_name_json.find("\"id\": \"In1.Cu\"") != std::string::npos,
          "pcb get-layer-name reports layer id");
  require(layer_name_json.find("\"name\": \"InnerSignal\"") != std::string::npos,
          "pcb get-layer-name reports edited layer name");
  require(layer_name_json.find("\"visible\": false") != std::string::npos,
          "pcb get-layer-name reports visibility");
  require(run(quote(CCAD_BINARY) + " pcb get-layer-name --file " + quote(board_project_path) +
              " --id Missing.Cu") != 0,
          "pcb get-layer-name rejects missing layer");

  const std::filesystem::path stackup_path = temp / "stackup.json";
  require(run(quote(CCAD_BINARY) + " pcb get-board-stackup --file " +
              quote(board_project_path) + " > " + quote(stackup_path)) == 0,
          "pcb get-board-stackup exits zero");
  const std::string stackup_json = readFile(stackup_path);
  require(stackup_json.find("\"stackup_kind\": \"ccad_board_stackup\"") != std::string::npos,
          "pcb get-board-stackup reports stackup kind");
  require(stackup_json.find("\"kicad_class\": \"BOARD_STACKUP\"") != std::string::npos,
          "pcb get-board-stackup reports KiCad stackup class");
  require(stackup_json.find("\"kicad_parity_scope\": \"default_stackup_first_slice\"") !=
              std::string::npos,
          "pcb get-board-stackup declares its current parity scope");
  require(stackup_json.find("\"copper_layer_count\": 3") != std::string::npos,
          "pcb get-board-stackup counts copper layers");
  require(stackup_json.find("\"stackup_item_count\": 5") != std::string::npos,
          "pcb get-board-stackup reports physical stackup item count");
  require(stackup_json.find("\"computed_stackup_thickness_nm\": 1600000") !=
              std::string::npos,
          "pcb get-board-stackup derives stackup thickness");
  require(stackup_json.find("\"type\": \"dielectric\"") != std::string::npos,
          "pcb get-board-stackup includes dielectric stackup rows");
  require(stackup_json.find("\"thickness_nm\": 747500") != std::string::npos,
          "pcb get-board-stackup distributes dielectric thickness");
  require(stackup_json.find("\"material\": \"FR4\"") != std::string::npos,
          "pcb get-board-stackup reports default dielectric material");
  require(stackup_json.find("\"from_layer\": \"F.Cu\"") != std::string::npos,
          "pcb get-board-stackup reports copper layer distances");
  require(stackup_json.find("\"to_layer\": \"B.Cu\"") != std::string::npos,
          "pcb get-board-stackup reports full copper layer span");
  require(stackup_json.find("\"distance_nm\": 1600000") != std::string::npos,
          "pcb get-board-stackup reports KiCad-style layer distance");
  require(stackup_json.find("\"id\": \"In1.Cu\"") != std::string::npos,
          "pcb get-board-stackup includes inner layer");

  const std::string bad_set_layer_command =
      quote(CCAD_BINARY) + " pcb set-layer --file " + quote(board_project_path) +
      " --id Missing.Cu --name Missing --kind copper --visible true";
  require(run(bad_set_layer_command) != 0, "pcb set-layer rejects missing layer");
  const std::string bad_set_layer_visibility_value_command =
      quote(CCAD_BINARY) + " pcb set-layer --file " + quote(board_project_path) +
      " --id In1.Cu --name InnerSignal --kind copper --visible maybe";
  require(run(bad_set_layer_visibility_value_command) != 0,
          "pcb set-layer rejects invalid visibility");
  const std::string bad_set_layer_visibility_command =
      quote(CCAD_BINARY) + " pcb set-layer-visibility --file " + quote(board_project_path) +
      " --id Missing.Cu --visible true";
  require(run(bad_set_layer_visibility_command) != 0,
          "pcb set-layer-visibility rejects missing layer");
  const std::string bad_layer_visibility_value_command =
      quote(CCAD_BINARY) + " pcb set-layer-visibility --file " + quote(board_project_path) +
      " --id In1.Cu --visible maybe";
  require(run(bad_layer_visibility_value_command) != 0,
          "pcb set-layer-visibility rejects invalid visibility");

  const std::filesystem::path remove_layer_board_path = temp / "remove-layer-board.ccad.json";
  const std::string remove_layer_init_command =
      quote(CCAD_BINARY) +
      " init --name remove-layer --width-mm 42 --height-mm 28 --out " +
      quote(remove_layer_board_path);
  require(run(remove_layer_init_command) == 0, "remove layer board init exits zero");
  const std::string remove_layer_add_back_command =
      quote(CCAD_BINARY) + " pcb add-layer --file " + quote(remove_layer_board_path) +
      " --id In1.Cu --name Inner1 --kind copper";
  require(run(remove_layer_add_back_command) == 0, "remove layer fixture add layer exits zero");
  const std::string remove_back_layer_command =
      quote(CCAD_BINARY) + " pcb remove-layer --file " + quote(remove_layer_board_path) +
      " --id In1.Cu";
  require(run(remove_back_layer_command) == 0, "pcb remove-layer removes unused layer");
  require(readFile(remove_layer_board_path).find("\"id\": \"In1.Cu\"") == std::string::npos,
          "pcb remove-layer deletes layer id");
  require(run(remove_back_layer_command) != 0, "pcb remove-layer rejects missing layer");
  require(run(remove_layer_add_back_command) == 0,
          "remove layer fixture re-adds layer after removal");
  const std::string remove_layer_add_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(remove_layer_board_path) +
      " --id LP1 --component U1 --pin 1 --net N1 --layers In1.Cu"
      " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0";
  require(run(remove_layer_add_pad_command) == 0, "remove layer fixture add pad exits zero");
  require(run(remove_back_layer_command) != 0, "pcb remove-layer rejects referenced layer");
  const std::string set_referenced_layer_kind_command =
      quote(CCAD_BINARY) + " pcb set-layer --file " + quote(remove_layer_board_path) +
      " --id In1.Cu --name InnerSilkscreen --kind silkscreen --visible true";
  require(run(set_referenced_layer_kind_command) != 0,
          "pcb set-layer rejects referenced non-copper layer kind");

  const std::string add_silkscreen_layer_command =
      quote(CCAD_BINARY) + " pcb add-layer --file " + quote(board_project_path) +
      " --id F.SilkS --name FrontSilkscreen --kind silkscreen";
  require(run(add_silkscreen_layer_command) == 0, "pcb add-layer accepts non-copper metadata");

  const std::string set_rules_command =
      quote(CCAD_BINARY) + " pcb set-rules --file " + quote(board_project_path) +
      " --copper-clearance-mm 0.15 --min-track-width-mm 0.12"
      " --min-via-annular-ring-mm 0.08 --min-connection-mm 0.01"
      " --min-via-diameter-mm 0.60 --min-through-hole-drill-mm 0.35"
      " --max-track-width-mm 0.40 --max-via-diameter-mm 1.20"
      " --min-microvia-diameter-mm 0.20 --min-microvia-drill-mm 0.10"
      " --min-hole-to-hole-mm 0.25 --hole-clearance-mm 0.25"
      " --copper-edge-clearance-mm 0.50 --silk-clearance-mm 0.02"
      " --min-groove-width-mm 0.10 --solder-mask-expansion-mm 0.03"
      " --solder-mask-min-width-mm 0.10 --solder-mask-to-copper-clearance-mm 0.02"
      " --solder-paste-margin-mm -0.01 --solder-paste-margin-ratio -0.05"
      " --board-thickness-mm 1.60 --use-height-for-length-calcs false"
      " --tent-vias-front true --tent-vias-back false --cover-vias-front true"
      " --cover-vias-back false --plug-vias-front true --plug-vias-back false"
      " --cap-vias true --fill-vias false";
  require(run(set_rules_command) == 0, "pcb set-rules exits zero");
  const std::string rules_json = readFile(board_project_path);
  require(rules_json.find("\"design_rules\"") != std::string::npos,
          "pcb set-rules writes design rules");
  require(rules_json.find("\"copper_clearance_nm\": 150000") != std::string::npos,
          "pcb set-rules writes copper clearance");
  require(rules_json.find("\"min_track_width_nm\": 120000") != std::string::npos,
          "pcb set-rules writes minimum track width");
  require(rules_json.find("\"max_track_width_nm\": 400000") != std::string::npos,
          "pcb set-rules writes maximum track width");
  require(rules_json.find("\"min_via_annular_ring_nm\": 80000") != std::string::npos,
          "pcb set-rules writes minimum via annular ring");
  require(rules_json.find("\"min_via_diameter_nm\": 600000") != std::string::npos,
          "pcb set-rules writes minimum via diameter");
  require(rules_json.find("\"max_via_diameter_nm\": 1200000") != std::string::npos,
          "pcb set-rules writes maximum via diameter");
  require(rules_json.find("\"min_through_hole_drill_nm\": 350000") != std::string::npos,
          "pcb set-rules writes minimum through hole drill");
  require(rules_json.find("\"solder_mask_expansion_nm\": 30000") != std::string::npos,
          "pcb set-rules writes solder mask expansion");
  require(rules_json.find("\"solder_paste_margin_ratio\": -0.05") != std::string::npos,
          "pcb set-rules writes solder paste margin ratio");
  require(rules_json.find("\"board_thickness_nm\": 1600000") != std::string::npos,
          "pcb set-rules writes board thickness");
  const std::filesystem::path get_rules_path = temp / "get-rules.json";
  require(run(quote(CCAD_BINARY) + " pcb get-rules --file " + quote(board_project_path) +
              " > " + quote(get_rules_path)) == 0,
          "pcb get-rules exits zero");
  const std::string get_rules_json = readFile(get_rules_path);
  require(get_rules_json.find("\"rules_kind\": \"ccad_board_design_rules\"") !=
              std::string::npos,
          "pcb get-rules reports rules kind");
  require(get_rules_json.find("\"kicad_handler\": \"GetBoardDesignRules\"") !=
              std::string::npos,
          "pcb get-rules records KiCad handler reference");
  require(get_rules_json.find("\"kicad_class\": \"BOARD_DESIGN_SETTINGS\"") !=
              std::string::npos,
          "pcb get-rules records KiCad board design settings class");
  require(get_rules_json.find("\"kicad_schema_version\": 2") != std::string::npos,
          "pcb get-rules reports KiCad board settings schema version");
  require(get_rules_json.find("\"copper_clearance_nm\": 150000") != std::string::npos,
          "pcb get-rules reports copper clearance");
  require(get_rules_json.find("\"min_track_width_nm\": 120000") != std::string::npos,
          "pcb get-rules reports minimum track width");
  require(get_rules_json.find("\"min_via_annular_ring_nm\": 80000") != std::string::npos,
          "pcb get-rules reports minimum via annular ring");
  require(get_rules_json.find("\"min_via_diameter_nm\": 600000") != std::string::npos,
          "pcb get-rules reports minimum via diameter");
  require(get_rules_json.find("\"min_through_hole_drill_nm\": 350000") !=
              std::string::npos,
          "pcb get-rules reports minimum through hole drill");
  require(get_rules_json.find("\"solder_mask\"") != std::string::npos,
          "pcb get-rules reports solder mask rule group");
  require(get_rules_json.find("\"solder_paste\"") != std::string::npos,
          "pcb get-rules reports solder paste rule group");
  require(get_rules_json.find("\"via_finishing\"") != std::string::npos,
          "pcb get-rules reports via finishing flags");
  const std::filesystem::path inspect_rules_path = temp / "inspect-rules.json";
  const std::string inspect_rules_command =
      quote(CCAD_BINARY) + " inspect " + quote(board_project_path) + " > " +
      quote(inspect_rules_path);
  require(run(inspect_rules_command) == 0, "inspect after set-rules exits zero");
  const std::string inspect_rules_json = readFile(inspect_rules_path);
  require(inspect_rules_json.find("\"design_rules\"") != std::string::npos,
          "inspect writes design rules");
  require(inspect_rules_json.find("\"layer_summary\"") != std::string::npos,
          "inspect writes layer summary");
  require(inspect_rules_json.find("\"copper\": 3") != std::string::npos,
          "inspect writes copper layer count");
  require(inspect_rules_json.find("\"non_copper\": 1") != std::string::npos,
          "inspect writes non-copper layer count");
  require(inspect_rules_json.find("\"visible\": 3") != std::string::npos,
          "inspect writes visible layer count");
  require(inspect_rules_json.find("\"hidden\": 1") != std::string::npos,
          "inspect writes hidden layer count");
  require(inspect_rules_json.find("\"copper_clearance_nm\": 150000") != std::string::npos,
          "inspect writes copper clearance rule");
  require(inspect_rules_json.find("\"min_track_width_nm\": 120000") != std::string::npos,
          "inspect writes minimum track width rule");
  require(inspect_rules_json.find("\"min_via_annular_ring_nm\": 80000") != std::string::npos,
          "inspect writes minimum via annular ring rule");
  require(inspect_rules_json.find("\"min_via_diameter_nm\": 600000") != std::string::npos,
          "inspect writes minimum via diameter rule");
  require(inspect_rules_json.find("\"board_thickness_nm\": 1600000") != std::string::npos,
          "inspect writes board thickness rule");

  const std::filesystem::path pcb_api_schema_path = temp / "pcb-api-schema.json";
  require(run(quote(CCAD_BINARY) + " agent pcb-api-schema > " + quote(pcb_api_schema_path)) == 0,
          "agent pcb-api-schema exits zero");
  const std::string pcb_api_schema = readFile(pcb_api_schema_path);
  require(pcb_api_schema.find("\"ledger_status\":\"api_handler_pcb_cpp_registered_handlers_read\"") !=
              std::string::npos,
          "pcb api schema records full KiCad handler ledger status");
  require(pcb_api_schema.find("\"kicad_handler\":\"RunAction\"") != std::string::npos,
          "pcb api schema includes RunAction handler");
  require(pcb_api_schema.find(
              "\"kicad_handler\":\"GetBoardStackup\",\"ccad_command\":\"pcb get-board-stackup\",\"status\":\"first_slice\",\"scope\":\"default_stackup_first_slice\"") !=
              std::string::npos,
          "pcb api schema maps GetBoardStackup to physical stackup first slice");
  require(pcb_api_schema.find("\"kicad_handler\":\"GetSelection\"") != std::string::npos,
          "pcb api schema includes selection handlers");
  require(pcb_api_schema.find("\"kicad_handler\":\"SetBoardDesignRules\"") !=
              std::string::npos,
          "pcb api schema includes board rules mutation handler");
  require(pcb_api_schema.find("\"kicad_handler\":\"GetPadShapeAsPolygon\"") !=
              std::string::npos,
          "pcb api schema includes pad polygon handler gap");
  require(pcb_api_schema.find(
              "\"kicad_handler\":\"ExpandTextVariables\",\"ccad_command\":\"pcb expand-text-variables\",\"status\":\"first_slice\",\"scope\":\"project_text_variable_expansion_first_slice\"") !=
              std::string::npos,
          "pcb api schema maps ExpandTextVariables to text variable expansion first slice");
  require(pcb_api_schema.find("\"kicad_handler\":\"RunBoardJobExportGerbers\"") !=
              std::string::npos,
          "pcb api schema includes Gerber export job handler");
  require(pcb_api_schema.find(
              "\"ccad_command\":\"agent kicad-evidence-plan --kind pcb-export-gerbers\"") !=
              std::string::npos,
          "pcb api schema maps Gerber job to current evidence planner");
  require(pcb_api_schema.find("\"kicad_handler\":\"GetPageSettings\"") !=
              std::string::npos,
          "pcb api schema includes page settings gap");
  require(pcb_api_schema.find("\"next_file\":\"F:/kicad_src/pcbnew/api/api_pcb_enums.cpp\"") !=
              std::string::npos,
          "pcb api schema records next KiCad source file in parity walk");
  require(pcb_api_schema.find("\"enum_ledger_status\":\"api_pcb_enums_cpp_read\"") !=
              std::string::npos,
          "pcb api schema records KiCad enum ledger status");
  require(pcb_api_schema.find("\"source_enum_reference\":\"F:/kicad_src/pcbnew/api/api_pcb_enums.cpp\"") !=
              std::string::npos,
          "pcb api schema records KiCad enum source reference");
  require(pcb_api_schema.find("\"api_enum\":\"types::PadType\"") != std::string::npos,
          "pcb api schema includes pad type enum group");
  require(pcb_api_schema.find("\"PT_EDGE_CONNECTOR\"") != std::string::npos,
          "pcb api schema includes edge connector pad type");
  require(pcb_api_schema.find("\"api_enum\":\"types::PadStackShape\"") != std::string::npos,
          "pcb api schema includes pad stack shape enum group");
  require(pcb_api_schema.find("\"PSS_CHAMFEREDRECT\"") != std::string::npos,
          "pcb api schema includes chamfered pad shape");
  require(pcb_api_schema.find("\"api_enum\":\"types::ViaType\"") != std::string::npos,
          "pcb api schema includes via type enum group");
  require(pcb_api_schema.find("\"VT_MICRO\"") != std::string::npos,
          "pcb api schema includes microvia type");
  require(pcb_api_schema.find("\"api_enum\":\"types::ZoneConnectionStyle\"") !=
              std::string::npos,
          "pcb api schema includes zone connection style enum group");
  require(pcb_api_schema.find("\"ZCS_PTH_THERMAL\"") != std::string::npos,
          "pcb api schema includes through-hole thermal zone connection");
  require(pcb_api_schema.find("\"api_enum\":\"CustomRuleConstraintType\"") !=
              std::string::npos,
          "pcb api schema includes DRC constraint enum group");
  require(pcb_api_schema.find("\"CRCT_DIFF_PAIR_GAP\"") != std::string::npos,
          "pcb api schema includes differential-pair gap constraint");
  require(pcb_api_schema.find("\"api_enum\":\"DrcErrorType\"") != std::string::npos,
          "pcb api schema includes DRC error enum group");
  require(pcb_api_schema.find("\"DRCET_TRACK_WIDTH\"") != std::string::npos,
          "pcb api schema includes track-width DRC error");
  require(pcb_api_schema.find("\"api_enum\":\"DrillFormat\"") != std::string::npos,
          "pcb api schema includes drill export enum group");
  require(pcb_api_schema.find("\"DF_EXCELLON\"") != std::string::npos,
          "pcb api schema includes Excellon drill format");
  require(pcb_api_schema.find("\"next_file\":\"F:/kicad_src/pcbnew/api/board_context.cpp\"") !=
              std::string::npos,
          "pcb api schema advances next KiCad source file after enum ledger");
  require(pcb_api_schema.find("\"context_ledger_status\":\"board_context_h_cpp_read\"") !=
              std::string::npos,
          "pcb api schema records KiCad board context ledger status");
  require(pcb_api_schema.find("\"source_context_reference\":\"F:/kicad_src/pcbnew/api/board_context.h\"") !=
              std::string::npos,
          "pcb api schema records KiCad board context source header");
  require(pcb_api_schema.find("\"GetBoard\"") != std::string::npos,
          "pcb api schema records KiCad GetBoard context method");
  require(pcb_api_schema.find("\"CanAcceptApiCommands\"") != std::string::npos,
          "pcb api schema records KiCad command acceptance context method");
  require(pcb_api_schema.find("\"SavePcbCopy\"") != std::string::npos,
          "pcb api schema records KiCad save-copy context method");
  require(pcb_api_schema.find("\"ccad_analogue\":\"headless_cli_project_file_context_and_qt_review_window_context\"") !=
              std::string::npos,
          "pcb api schema records current CCad board context analogue");
  require(pcb_api_schema.find("\"context_status\":\"split_context_gap\"") !=
              std::string::npos,
          "pcb api schema marks split context gap");
  require(
      pcb_api_schema.find("\"next_file\":\"F:/kicad_src/pcbnew/api/headless_board_context.cpp\"") !=
          std::string::npos,
      "pcb api schema advances next KiCad source file after board context");
  require(pcb_api_schema.find(
              "\"headless_context_ledger_status\":\"headless_board_context_h_cpp_read\"") !=
              std::string::npos,
          "pcb api schema records KiCad headless context ledger status");
  require(pcb_api_schema.find(
              "\"source_headless_context_reference\":\"F:/kicad_src/pcbnew/api/headless_board_context.h\"") !=
              std::string::npos,
          "pcb api schema records KiCad headless context source header");
  require(pcb_api_schema.find("\"kicad_class\":\"HEADLESS_BOARD_CONTEXT\"") !=
              std::string::npos,
          "pcb api schema records KiCad headless context class");
  require(pcb_api_schema.find("\"owned_board\"") != std::string::npos,
          "pcb api schema records headless context board ownership");
  require(pcb_api_schema.find("\"project_linkage_teardown\"") != std::string::npos,
          "pcb api schema records headless context project teardown behavior");
  require(pcb_api_schema.find("\"command_acceptance\":\"always_true_headless\"") !=
              std::string::npos,
          "pcb api schema records headless command acceptance behavior");
  require(pcb_api_schema.find("\"api_folder_status\":\"complete_first_audit\"") !=
              std::string::npos,
          "pcb api schema marks pcbnew api folder audit complete");

  const std::string add_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(board_project_path) +
      " --id P1 --component U1 --pin 1 --net N1 --layers F.Cu"
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

  const std::string duplicate_cross_type_id_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
      " --id P1 --net N1 --x-mm 10 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
  require(run(duplicate_cross_type_id_command) != 0,
          "pcb add-via rejects duplicate physical object id across types");

  const std::string add_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T1 --net N1 --layer F.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
  require(run(add_track_command) == 0, "pcb add-track exits zero");
  const std::string track_json = readFile(board_project_path);
  require(track_json.find("\"tracks\"") != std::string::npos, "pcb add-track writes tracks");
  require(track_json.find("\"width_nm\": 250000") != std::string::npos,
          "pcb add-track writes width");

  const std::string add_route_request_command =
      quote(CCAD_BINARY) + " pcb add-route-request --file " + quote(board_project_path) +
      " --id RR1 --net N1 --from P1 --to V1 --preferred-layer F.Cu"
      " --policy shortest_safe --width-mm 0.25";
  require(run(add_route_request_command) == 0, "pcb add-route-request exits zero");
  const std::string route_request_json = readFile(board_project_path);
  require(route_request_json.find("\"route_requests\"") != std::string::npos,
          "pcb add-route-request writes route requests");
  require(route_request_json.find("\"from_object_id\": \"P1\"") != std::string::npos,
          "pcb add-route-request writes source object");
  require(route_request_json.find("\"to_object_id\": \"V1\"") != std::string::npos,
          "pcb add-route-request writes target object");
  require(route_request_json.find("\"policy\": \"shortest_safe\"") != std::string::npos,
          "pcb add-route-request writes policy");
  require(run(add_route_request_command) != 0, "pcb add-route-request rejects duplicate id");
  require(run(quote(CCAD_BINARY) + " pcb add-route-request --file " + quote(board_project_path) +
              " --id RR_BAD --net N1 --from P1 --to NO_OBJECT --preferred-layer F.Cu"
              " --policy shortest_safe --width-mm 0.25") != 0,
          "pcb add-route-request rejects missing endpoint");
  require(run(quote(CCAD_BINARY) + " pcb add-route-request --file " + quote(board_project_path) +
              " --id RR_SILK --net N1 --from P1 --to V1 --preferred-layer F.SilkS"
              " --policy shortest_safe --width-mm 0.25") != 0,
          "pcb add-route-request rejects non-copper preferred layer");
  const std::string set_route_request_command =
      quote(CCAD_BINARY) + " pcb set-route-request --file " + quote(board_project_path) +
      " --id RR1 --net N1 --from V1 --to T1 --preferred-layer B.Cu"
      " --policy prefer_back --width-mm 0.30";
  require(run(set_route_request_command) == 0, "pcb set-route-request exits zero");
  const std::string set_route_request_json = readFile(board_project_path);
  require(set_route_request_json.find("\"net_id\": \"N1\"") != std::string::npos,
          "pcb set-route-request writes net");
  require(set_route_request_json.find("\"from_object_id\": \"V1\"") != std::string::npos,
          "pcb set-route-request writes source object");
  require(set_route_request_json.find("\"to_object_id\": \"T1\"") != std::string::npos,
          "pcb set-route-request writes target object");
  require(set_route_request_json.find("\"preferred_layer_id\": \"B.Cu\"") != std::string::npos,
          "pcb set-route-request writes preferred layer");
  require(set_route_request_json.find("\"policy\": \"prefer_back\"") != std::string::npos,
          "pcb set-route-request writes policy");
  require(set_route_request_json.find("\"width_nm\": 300000") != std::string::npos,
          "pcb set-route-request writes width");
  require(run(quote(CCAD_BINARY) + " pcb set-route-request --file " + quote(board_project_path) +
              " --id RR_MISSING --net N1 --from V1 --to T1 --preferred-layer B.Cu"
              " --policy prefer_back --width-mm 0.30") != 0,
          "pcb set-route-request rejects missing request");
  require(run(quote(CCAD_BINARY) + " pcb set-route-request --file " + quote(board_project_path) +
              " --id RR1 --net N1 --from V1 --to NO_OBJECT --preferred-layer B.Cu"
              " --policy prefer_back --width-mm 0.30") != 0,
          "pcb set-route-request rejects missing endpoint");
  require(run(quote(CCAD_BINARY) + " pcb set-route-request --file " + quote(board_project_path) +
              " --id RR1 --net N1 --from V1 --to T1 --preferred-layer F.SilkS"
              " --policy prefer_back --width-mm 0.30") != 0,
          "pcb set-route-request rejects non-copper preferred layer");

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

  const std::string add_zone_command =
      quote(CCAD_BINARY) + " pcb add-zone --file " + quote(board_project_path) +
      " --id Z1 --name \"GND copper\" --net N1 --layers F.Cu"
      " --x-mm 2 --y-mm 2 --width-mm 20 --height-mm 12"
      " --priority 1 --clearance-mm 0.2 --min-thickness-mm 0.25"
      " --pad-connection thermal";
  require(run(add_zone_command) == 0, "pcb add-zone exits zero");
  const std::string zone_json = readFile(board_project_path);
  require(zone_json.find("\"zones\"") != std::string::npos, "pcb add-zone writes zones");
  require(zone_json.find("\"id\": \"Z1\"") != std::string::npos, "pcb add-zone writes id");
  require(zone_json.find("\"name\": \"GND copper\"") != std::string::npos,
          "pcb add-zone writes name");
  require(zone_json.find("\"net_id\": \"N1\"") != std::string::npos,
          "pcb add-zone writes net");
  require(zone_json.find("\"pad_connection\": \"thermal\"") != std::string::npos,
          "pcb add-zone writes pad connection");

  const std::filesystem::path refill_zones_path = temp / "refill-zones.json";
  require(run(quote(CCAD_BINARY) + " pcb refill-zones --file " + quote(board_project_path) +
              " --zone-id Z1 > " + quote(refill_zones_path)) == 0,
          "pcb refill-zones exits zero");
  const std::string refill_zones_json = readFile(refill_zones_path);
  require(refill_zones_json.find("\"id\": \"Z1\"") != std::string::npos,
          "pcb refill-zones reports requested zone");
  require(refill_zones_json.find("\"filled\": true") != std::string::npos,
          "pcb refill-zones reports filled state");
  require(refill_zones_json.find("\"contour_count\": 1") != std::string::npos,
          "pcb refill-zones reports contour count");
  require(refill_zones_json.find("\"thermal_spoke_count\": ") != std::string::npos,
          "pcb refill-zones reports thermal spoke count");
  require(run(quote(CCAD_BINARY) + " pcb refill-zones --file " + quote(board_project_path) +
              " --zone-id Z1 --apply true") == 0,
          "pcb refill-zones apply exits zero");
  const std::string refilled_project_json = readFile(board_project_path);
  require(refilled_project_json.find("\"filled_contours\"") != std::string::npos,
          "pcb refill-zones persists filled contours");
  require(refilled_project_json.find("\"filled_thermal_spokes\"") != std::string::npos,
          "pcb refill-zones persists thermal spokes");

  const std::string add_placement_region_command =
      quote(CCAD_BINARY) + " pcb add-placement-region --file " + quote(board_project_path) +
      " --id PR1 --kind component --x-mm 2 --y-mm 3 --width-mm 10 --height-mm 6";
  require(run(add_placement_region_command) == 0, "pcb add-placement-region exits zero");
  const std::string placement_region_json = readFile(board_project_path);
  require(placement_region_json.find("\"placement_regions\"") != std::string::npos,
          "pcb add-placement-region writes placement regions");
  require(placement_region_json.find("\"id\": \"PR1\"") != std::string::npos,
          "pcb add-placement-region writes id");
  require(placement_region_json.find("\"kind\": \"component\"") != std::string::npos,
          "pcb add-placement-region writes kind");
  require(placement_region_json.find("\"width_nm\": 10000000") != std::string::npos,
          "pcb add-placement-region writes width");

  const std::filesystem::path layer_lookup_path = temp / "layer-lookup.json";
  const std::string layer_lookup_command =
      quote(CCAD_BINARY) + " pcb get-object --file " + quote(board_project_path) +
      " --id F.Cu > " + quote(layer_lookup_path);
  require(run(layer_lookup_command) == 0, "pcb get-object finds layer");
  const std::string layer_lookup_json = readFile(layer_lookup_path);
  require(layer_lookup_json.find("\"type\": \"layer\"") != std::string::npos,
          "pcb get-object writes layer type");
  require(layer_lookup_json.find("\"id\": \"F.Cu\"") != std::string::npos,
          "pcb get-object writes layer id");
  require(layer_lookup_json.find("\"kind\": \"copper\"") != std::string::npos,
          "pcb get-object writes layer kind");

  const std::filesystem::path pad_lookup_path = temp / "pad-lookup.json";
  const std::string pad_lookup_command =
      quote(CCAD_BINARY) + " pcb get-object --file " + quote(board_project_path) +
      " --id P1 > " + quote(pad_lookup_path);
  require(run(pad_lookup_command) == 0, "pcb get-object finds pad");
  const std::string pad_lookup_json = readFile(pad_lookup_path);
  require(pad_lookup_json.find("\"type\": \"pad\"") != std::string::npos,
          "pcb get-object writes pad type");
  require(pad_lookup_json.find("\"component_id\": \"U1\"") != std::string::npos,
          "pcb get-object writes pad component");
  require(pad_lookup_json.find("\"width_nm\": 1500000") != std::string::npos,
          "pcb get-object writes pad width");
  require(pad_lookup_json.find("\"kicad_base_class\": \"BOARD_ITEM\"") != std::string::npos,
          "pcb get-object exposes KiCad board item base class");
  require(pad_lookup_json.find("\"kicad_groupable\": true") != std::string::npos,
          "pcb get-object exposes KiCad board item groupable status");
  require(pad_lookup_json.find("\"primary_layer_id\": \"F.Cu\"") != std::string::npos,
          "pcb get-object exposes board item primary layer");
  require(pad_lookup_json.find("\"layer_mask_description\": \"F.Cu\"") != std::string::npos,
          "pcb get-object exposes KiCad-style layer mask description");
  require(pad_lookup_json.find("\"is_on_copper_layer\": true") != std::string::npos,
          "pcb get-object exposes board item copper-layer status");
  require(pad_lookup_json.find("\"has_hole\": false") != std::string::npos,
          "pcb get-object exposes board item hole status");
  require(pad_lookup_json.find("\"locked\": false") != std::string::npos,
          "pcb get-object exposes board item locked status");
  require(pad_lookup_json.find("\"view_layer_ids\": [\"F.Cu\"]") != std::string::npos,
          "pcb get-object exposes board item view layers");

  const std::filesystem::path via_lookup_path = temp / "via-lookup.json";
  const std::string via_lookup_command =
      quote(CCAD_BINARY) + " pcb get-object --file " + quote(board_project_path) +
      " --id V1 > " + quote(via_lookup_path);
  require(run(via_lookup_command) == 0, "pcb get-object finds via");
  const std::string via_lookup_json = readFile(via_lookup_path);
  require(via_lookup_json.find("\"via_type\": \"through\"") != std::string::npos,
          "pcb get-object exposes via type");
  require(via_lookup_json.find("\"start_layer_id\": \"\"") != std::string::npos,
          "pcb get-object exposes via start layer");

  const std::filesystem::path zone_lookup_path = temp / "zone-lookup.json";
  const std::string zone_lookup_command =
      quote(CCAD_BINARY) + " pcb get-object --file " + quote(board_project_path) +
      " --id Z1 > " + quote(zone_lookup_path);
  require(run(zone_lookup_command) == 0, "pcb get-object finds zone");
  const std::string zone_lookup_json = readFile(zone_lookup_path);
  require(zone_lookup_json.find("\"type\": \"zone\"") != std::string::npos,
          "pcb get-object writes zone type");
  require(zone_lookup_json.find("\"name\": \"GND copper\"") != std::string::npos,
          "pcb get-object writes zone name");
  require(zone_lookup_json.find("\"layer_ids\"") != std::string::npos,
          "pcb get-object writes zone layer set");

  const std::string missing_lookup_command =
      quote(CCAD_BINARY) + " pcb get-object --file " + quote(board_project_path) +
      " --id MISSING";
  require(run(missing_lookup_command) != 0, "pcb get-object rejects missing id");

  const std::filesystem::path list_objects_path = temp / "list-objects.json";
  const std::string list_objects_command =
      quote(CCAD_BINARY) + " pcb list-objects --file " + quote(board_project_path) +
      " > " + quote(list_objects_path);
  require(run(list_objects_command) == 0, "pcb list-objects exits zero");
  const std::string list_objects_json = readFile(list_objects_path);
  require(list_objects_json.find("\"summary\": {") != std::string::npos,
          "pcb list-objects writes summary");
  require(list_objects_json.find("\"total\": 10") != std::string::npos,
          "pcb list-objects reports total");
  require(list_objects_json.find("\"type\": \"layer\"") != std::string::npos,
          "pcb list-objects includes layers");
  require(list_objects_json.find("\"kicad_layer_number\": 0") != std::string::npos,
          "pcb list-objects includes canonical layer number for F.Cu");
  require(list_objects_json.find("\"kicad_layer_number\": 1") != std::string::npos,
          "pcb list-objects includes canonical layer number for In1.Cu");
  require(list_objects_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb list-objects includes pad id");
  require(list_objects_json.find("\"net_id\": \"N1\"") != std::string::npos,
          "pcb list-objects includes net metadata");
  require(list_objects_json.find("\"kicad_connected_class\": \"BOARD_CONNECTED_ITEM\"") !=
              std::string::npos,
          "pcb list-objects exposes KiCad connected item class");
  require(list_objects_json.find("\"connected_item\": true") != std::string::npos,
          "pcb list-objects marks connected items");
  require(list_objects_json.find("\"net_name\": \"N1\"") != std::string::npos,
          "pcb list-objects exposes connected item net name");
  require(list_objects_json.find("\"net_name_message\": \"[N1]\"") != std::string::npos,
          "pcb list-objects exposes KiCad-style net name message");
  require(list_objects_json.find("\"net_class_name\": \"Default\"") != std::string::npos,
          "pcb list-objects exposes default connected item net class");
  require(list_objects_json.find("\"local_ratsnest_visible\": true") != std::string::npos,
          "pcb list-objects exposes local ratsnest visibility");
  require(list_objects_json.find("\"kicad_base_class\": \"BOARD_ITEM\"") != std::string::npos,
          "pcb list-objects exposes KiCad board item base class");
  require(list_objects_json.find("\"kicad_groupable\": true") != std::string::npos,
          "pcb list-objects exposes KiCad board item groupable status");
  require(list_objects_json.find("\"primary_layer_id\": \"F.Cu\"") != std::string::npos,
          "pcb list-objects exposes board item primary layer");
  require(list_objects_json.find("\"layer_mask_description\": \"F.Cu\"") != std::string::npos,
          "pcb list-objects exposes KiCad-style layer mask description");
  require(list_objects_json.find("\"is_on_copper_layer\": true") != std::string::npos,
          "pcb list-objects exposes board item copper-layer status");
  require(list_objects_json.find("\"has_hole\": false") != std::string::npos,
          "pcb list-objects exposes board item hole status");
  require(list_objects_json.find("\"locked\": false") != std::string::npos,
          "pcb list-objects exposes board item locked status");
  require(list_objects_json.find("\"view_layer_ids\": [\"F.Cu\"]") != std::string::npos,
          "pcb list-objects exposes board item view layers");

  const std::filesystem::path list_tracks_path = temp / "list-tracks.json";
  const std::string list_tracks_command =
      quote(CCAD_BINARY) + " pcb list-objects --file " + quote(board_project_path) +
      " --type track > " + quote(list_tracks_path);
  require(run(list_tracks_command) == 0, "pcb list-objects filters by type");
  const std::string list_tracks_json = readFile(list_tracks_path);
  require(list_tracks_json.find("\"total\": 1") != std::string::npos,
          "pcb list-objects filtered summary reports one track");
  require(list_tracks_json.find("\"type\": \"track\"") != std::string::npos,
          "pcb list-objects filtered output includes track");
  require(list_tracks_json.find("\"type\": \"pad\"") == std::string::npos,
          "pcb list-objects filtered output excludes pad");

  const std::filesystem::path list_zones_path = temp / "list-zones.json";
  const std::string list_zones_command =
      quote(CCAD_BINARY) + " pcb list-objects --file " + quote(board_project_path) +
      " --type zone > " + quote(list_zones_path);
  require(run(list_zones_command) == 0, "pcb list-objects filters zones by type");
  const std::string list_zones_json = readFile(list_zones_path);
  require(list_zones_json.find("\"total\": 1") != std::string::npos,
          "pcb list-objects zone summary reports one zone");
  require(list_zones_json.find("\"type\": \"zone\"") != std::string::npos,
          "pcb list-objects zone output includes zone");
  require(list_zones_json.find("\"id\": \"Z1\"") != std::string::npos,
          "pcb list-objects zone output includes zone id");

  const std::filesystem::path collect_items_path = temp / "collect-items.json";
  const std::string collect_items_command =
      quote(CCAD_BINARY) + " pcb collect-items --file " + quote(board_project_path) +
      " --scan-set pads_or_tracks --preferred-layer F.Cu --visible-layers F.Cu,B.Cu > " +
      quote(collect_items_path);
  require(run(collect_items_command) == 0, "pcb collect-items exits zero");
  const std::string collect_items_json = readFile(collect_items_path);
  require(collect_items_json.find("\"kicad_collector\": \"GENERAL_COLLECTOR\"") !=
              std::string::npos,
          "pcb collect-items reports KiCad collector class");
  require(collect_items_json.find("\"scan_set\": \"pads_or_tracks\"") != std::string::npos,
          "pcb collect-items reports scan set");
  require(collect_items_json.find("\"PCB_ARC_T\"") != std::string::npos,
          "pcb collect-items reports unsupported arc parity gap");
  require(collect_items_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb collect-items includes pad row");
  require(collect_items_json.find("\"kicad_type\": \"PCB_PAD_T\"") != std::string::npos,
          "pcb collect-items maps pad to KiCad type");
  require(collect_items_json.find("\"collection_bucket\": \"primary\"") != std::string::npos,
          "pcb collect-items reports primary bucket");
  require(collect_items_json.find("\"id\": \"V1\"") != std::string::npos,
          "pcb collect-items includes via row");
  require(collect_items_json.find("\"id\": \"T1\"") != std::string::npos,
          "pcb collect-items includes track row");
  require(collect_items_json.find("\"primary_count\": 3") != std::string::npos,
          "pcb collect-items counts preferred-layer collector rows");
  require(collect_items_json.find("\"locked\": false") != std::string::npos,
          "pcb collect-items exposes item lock state");

  const std::filesystem::path collect_no_tracks_path = temp / "collect-items-no-tracks.json";
  const std::string collect_no_tracks_command =
      quote(CCAD_BINARY) + " pcb collect-items --file " + quote(board_project_path) +
      " --scan-set pads_or_tracks --preferred-layer F.Cu --visible-layers F.Cu,B.Cu "
      "--ignore-tracks true > " +
      quote(collect_no_tracks_path);
  require(run(collect_no_tracks_command) == 0,
          "pcb collect-items supports KiCad ignore-tracks guide flag");
  const std::string collect_no_tracks_json = readFile(collect_no_tracks_path);
  require(collect_no_tracks_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb collect-items ignore-tracks keeps pads");
  require(collect_no_tracks_json.find("\"id\": \"T1\"") == std::string::npos,
          "pcb collect-items ignore-tracks removes tracks");

  const std::filesystem::path collect_locked_board_path =
      temp / "collect-items-locked-board.ccad.json";
  std::string collect_locked_board = readFile(board_project_path);
  const std::string pad_id_line = "        \"id\": \"P1\",\n";
  const std::size_t pad_id_position = collect_locked_board.find(pad_id_line);
  require(pad_id_position != std::string::npos, "collector locked fixture finds pad id line");
  collect_locked_board.insert(pad_id_position + pad_id_line.size(), "        \"locked\": true,\n");
  const std::string track_id_line = "        \"id\": \"T1\",\n";
  const std::size_t track_id_position = collect_locked_board.find(track_id_line);
  require(track_id_position != std::string::npos, "collector locked fixture finds track id line");
  collect_locked_board.insert(track_id_position + track_id_line.size(),
                              "        \"locked\": true,\n");
  writeFile(collect_locked_board_path, collect_locked_board);

  const std::filesystem::path collect_locked_visible_path =
      temp / "collect-items-locked-visible.json";
  const std::string collect_locked_visible_command =
      quote(CCAD_BINARY) + " pcb collect-items --file " + quote(collect_locked_board_path) +
      " --scan-set pads_or_tracks --preferred-layer F.Cu --visible-layers F.Cu,B.Cu > " +
      quote(collect_locked_visible_path);
  require(run(collect_locked_visible_command) == 0,
          "pcb collect-items reads locked board items");
  const std::string collect_locked_visible_json = readFile(collect_locked_visible_path);
  require(collect_locked_visible_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb collect-items keeps locked pads unless guide ignores locks");
  require(collect_locked_visible_json.find("\"id\": \"T1\"") != std::string::npos,
          "pcb collect-items keeps locked tracks unless guide ignores locks");
  require(collect_locked_visible_json.find("\"locked\": true") != std::string::npos,
          "pcb collect-items reports locked candidate state");

  const std::filesystem::path collect_ignore_locked_path =
      temp / "collect-items-ignore-locked.json";
  const std::string collect_ignore_locked_command =
      quote(CCAD_BINARY) + " pcb collect-items --file " + quote(collect_locked_board_path) +
      " --scan-set pads_or_tracks --preferred-layer F.Cu --visible-layers F.Cu,B.Cu "
      "--ignore-locked true > " +
      quote(collect_ignore_locked_path);
  require(run(collect_ignore_locked_command) == 0,
          "pcb collect-items supports KiCad ignore-locked guide flag");
  const std::string collect_ignore_locked_json = readFile(collect_ignore_locked_path);
  require(collect_ignore_locked_json.find("\"id\": \"P1\"") == std::string::npos,
          "pcb collect-items ignore-locked removes locked pads");
  require(collect_ignore_locked_json.find("\"id\": \"T1\"") == std::string::npos,
          "pcb collect-items ignore-locked removes locked tracks");
  require(collect_ignore_locked_json.find("\"id\": \"V1\"") != std::string::npos,
          "pcb collect-items ignore-locked keeps unlocked vias");
  require(collect_ignore_locked_json.find("\"primary_count\": 1") != std::string::npos,
          "pcb collect-items ignore-locked recounts remaining primary rows");

  const std::string bad_list_objects_command =
      quote(CCAD_BINARY) + " pcb list-objects --file " + quote(board_project_path) +
      " --type nonsense";
  require(run(bad_list_objects_command) != 0, "pcb list-objects rejects unknown type");

  const std::filesystem::path list_nets_path = temp / "list-nets.json";
  const std::string list_nets_command =
      quote(CCAD_BINARY) + " pcb list-nets --file " + quote(board_project_path) +
      " > " + quote(list_nets_path);
  require(run(list_nets_command) == 0, "pcb list-nets exits zero");
  const std::string list_nets_json = readFile(list_nets_path);
  require(list_nets_json.find("\"summary\": {") != std::string::npos,
          "pcb list-nets writes summary");
  require(list_nets_json.find("\"total\": 1") != std::string::npos,
          "pcb list-nets reports one physical net");
  require(list_nets_json.find("\"id\": \"N1\"") != std::string::npos,
          "pcb list-nets includes net id");
  require(list_nets_json.find("\"pad_count\": 1") != std::string::npos,
          "pcb list-nets counts pads");
  require(list_nets_json.find("\"via_count\": 1") != std::string::npos,
          "pcb list-nets counts vias");
  require(list_nets_json.find("\"track_count\": 1") != std::string::npos,
          "pcb list-nets counts tracks");

  const std::filesystem::path list_by_net_path = temp / "list-by-net.json";
  const std::string list_by_net_command =
      quote(CCAD_BINARY) + " pcb list-by-net --file " + quote(board_project_path) +
      " --net N1 > " + quote(list_by_net_path);
  require(run(list_by_net_command) == 0, "pcb list-by-net exits zero");
  const std::string list_by_net_json = readFile(list_by_net_path);
  require(list_by_net_json.find("\"query_kind\": \"items_by_net\"") != std::string::npos,
          "pcb list-by-net reports its query kind");
  require(list_by_net_json.find("\"connectivity_scope\": \"net_equivalent_first_slice\"") !=
              std::string::npos,
          "pcb list-by-net declares the first-slice connectivity scope");
  require(list_by_net_json.find("\"net_id\": \"N1\"") != std::string::npos,
          "pcb list-by-net reports queried net");
  require(list_by_net_json.find("\"total\": 4") != std::string::npos,
          "pcb list-by-net returns pad, via, track, and zone for the net");
  require(list_by_net_json.find("\"pad_count\": 1") != std::string::npos,
          "pcb list-by-net counts pads");
  require(list_by_net_json.find("\"via_count\": 1") != std::string::npos,
          "pcb list-by-net counts vias");
  require(list_by_net_json.find("\"track_count\": 1") != std::string::npos,
          "pcb list-by-net counts tracks");
  require(list_by_net_json.find("\"zone_count\": 1") != std::string::npos,
          "pcb list-by-net counts zones");
  require(list_by_net_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb list-by-net includes pad id");
  require(list_by_net_json.find("\"resolved_layers\": [\"F.Cu\"]") != std::string::npos,
          "pcb list-by-net writes resolved pad layer set");
  require(list_by_net_json.find("\"kicad_layer_numbers\": [0]") != std::string::npos,
          "pcb list-by-net writes KiCad layer numbers for pads");
  require(list_by_net_json.find("\"id\": \"V1\"") != std::string::npos,
          "pcb list-by-net includes via id");
  require(list_by_net_json.find("\"id\": \"T1\"") != std::string::npos,
          "pcb list-by-net includes track id");
  require(list_by_net_json.find("\"id\": \"Z1\"") != std::string::npos,
          "pcb list-by-net includes zone id");
  require(list_by_net_json.find("\"kicad_connected_class\": \"BOARD_CONNECTED_ITEM\"") !=
              std::string::npos,
          "pcb list-by-net exposes KiCad connected item class");
  require(list_by_net_json.find("\"net_name_message\": \"[N1]\"") != std::string::npos,
          "pcb list-by-net exposes KiCad-style net name message");
  require(list_by_net_json.find("\"net_class_scope\": \"default_netclass_until_model_exists\"") !=
              std::string::npos,
          "pcb list-by-net exposes explicit netclass scope");

  const std::filesystem::path list_by_net_tracks_path = temp / "list-by-net-tracks.json";
  const std::string list_by_net_tracks_command =
      quote(CCAD_BINARY) + " pcb list-by-net --file " + quote(board_project_path) +
      " --net N1 --type track > " + quote(list_by_net_tracks_path);
  require(run(list_by_net_tracks_command) == 0, "pcb list-by-net filters by connectable type");
  const std::string list_by_net_tracks_json = readFile(list_by_net_tracks_path);
  require(list_by_net_tracks_json.find("\"total\": 1") != std::string::npos,
          "pcb list-by-net type filter reports one track");
  require(list_by_net_tracks_json.find("\"id\": \"T1\"") != std::string::npos,
          "pcb list-by-net type filter keeps the track");
  require(list_by_net_tracks_json.find("\"id\": \"P1\"") == std::string::npos,
          "pcb list-by-net type filter excludes pads");

  const std::filesystem::path list_connected_path = temp / "list-connected.json";
  const std::string list_connected_command =
      quote(CCAD_BINARY) + " pcb list-connected --file " + quote(board_project_path) +
      " --id P1 > " + quote(list_connected_path);
  require(run(list_connected_command) == 0, "pcb list-connected exits zero");
  const std::string list_connected_json = readFile(list_connected_path);
  require(list_connected_json.find("\"query_kind\": \"connected_items\"") != std::string::npos,
          "pcb list-connected reports its query kind");
  require(list_connected_json.find("\"source_id\": \"P1\"") != std::string::npos,
          "pcb list-connected reports source id");
  require(list_connected_json.find("\"source_type\": \"pad\"") != std::string::npos,
          "pcb list-connected reports source type");
  require(list_connected_json.find("\"source_found\": true") != std::string::npos,
          "pcb list-connected reports source discovery");
  require(list_connected_json.find("\"net_id\": \"N1\"") != std::string::npos,
          "pcb list-connected reports source net");
  require(list_connected_json.find("\"total\": 4") != std::string::npos,
          "pcb list-connected returns same-net connectable objects");
  require(list_connected_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb list-connected includes source pad");
  require(list_connected_json.find("\"resolved_layers\": [\"F.Cu\"]") != std::string::npos,
          "pcb list-connected writes resolved source-pad layer set");
  require(list_connected_json.find("\"kicad_layer_numbers\": [0]") != std::string::npos,
          "pcb list-connected writes KiCad layer numbers for pads");
  require(list_connected_json.find("\"id\": \"V1\"") != std::string::npos,
          "pcb list-connected includes via");
  require(list_connected_json.find("\"id\": \"T1\"") != std::string::npos,
          "pcb list-connected includes track");
  require(list_connected_json.find("\"id\": \"Z1\"") != std::string::npos,
          "pcb list-connected includes zone");

  const std::filesystem::path list_connected_vias_path = temp / "list-connected-vias.json";
  const std::string list_connected_vias_command =
      quote(CCAD_BINARY) + " pcb list-connected --file " + quote(board_project_path) +
      " --id P1 --type via > " + quote(list_connected_vias_path);
  require(run(list_connected_vias_command) == 0,
          "pcb list-connected filters by connectable type");
  const std::string list_connected_vias_json = readFile(list_connected_vias_path);
  require(list_connected_vias_json.find("\"total\": 1") != std::string::npos,
          "pcb list-connected type filter reports one via");
  require(list_connected_vias_json.find("\"id\": \"V1\"") != std::string::npos,
          "pcb list-connected type filter keeps via");
  require(list_connected_vias_json.find("\"id\": \"P1\"") == std::string::npos,
          "pcb list-connected type filter excludes pad rows");

  const std::string bad_list_by_net_command =
      quote(CCAD_BINARY) + " pcb list-by-net --file " + quote(board_project_path) +
      " --net N1 --type graphic";
  require(run(bad_list_by_net_command) != 0,
          "pcb list-by-net rejects non-connectable object type");
  const std::string bad_list_connected_command =
      quote(CCAD_BINARY) + " pcb list-connected --file " + quote(board_project_path) +
      " --id G1";
  require(run(bad_list_connected_command) != 0,
          "pcb list-connected rejects non-connectable source object");

  const std::filesystem::path list_route_requests_path = temp / "list-route-requests.json";
  const std::string list_route_requests_command =
      quote(CCAD_BINARY) + " pcb list-route-requests --file " + quote(board_project_path) +
      " > " + quote(list_route_requests_path);
  require(run(list_route_requests_command) == 0, "pcb list-route-requests exits zero");
  const std::string list_route_requests_json = readFile(list_route_requests_path);
  require(list_route_requests_json.find("\"summary\": {") != std::string::npos,
          "pcb list-route-requests writes summary");
  require(list_route_requests_json.find("\"total\": 1") != std::string::npos,
          "pcb list-route-requests reports one request");
  require(list_route_requests_json.find("\"id\": \"RR1\"") != std::string::npos,
          "pcb list-route-requests includes request id");
  require(list_route_requests_json.find("\"from_object_id\": \"V1\"") != std::string::npos,
          "pcb list-route-requests includes source object");
  require(list_route_requests_json.find("\"to_object_id\": \"T1\"") != std::string::npos,
          "pcb list-route-requests includes target object");
  const std::filesystem::path route_status_path = temp / "route-status.json";
  const std::string route_status_command =
      quote(CCAD_BINARY) + " pcb route-status --file " + quote(board_project_path) +
      " > " + quote(route_status_path);
  require(run(route_status_command) == 0, "pcb route-status exits zero");
  const std::string route_status_json = readFile(route_status_path);
  require(route_status_json.find("\"open\": 1") != std::string::npos,
          "pcb route-status reports open request");
  require(route_status_json.find("\"partial\": 0") != std::string::npos,
          "pcb route-status reports no partial requests");
  require(route_status_json.find("\"completed\": 0") != std::string::npos,
          "pcb route-status reports no completed requests");
  require(route_status_json.find("\"status\": \"open\"") != std::string::npos,
          "pcb route-status includes open status row");
  const std::filesystem::path route_job_path = temp / "route-job.json";
  const std::string export_route_job_command =
      quote(CCAD_BINARY) + " pcb export-route-job --file " + quote(board_project_path) +
      " > " + quote(route_job_path);
  require(run(export_route_job_command) == 0, "pcb export-route-job exits zero");
  const std::string route_job_json = readFile(route_job_path);
  require(route_job_json.find("\"route_job\"") != std::string::npos,
          "pcb export-route-job writes route job root");
  require(route_job_json.find("\"schema_version\": 1") != std::string::npos,
          "pcb export-route-job writes schema version");
  require(route_job_json.find("\"length_unit\": \"nanometer\"") != std::string::npos,
          "pcb export-route-job writes length unit");
  require(route_job_json.find("\"angle_unit\": \"degree\"") != std::string::npos,
          "pcb export-route-job writes angle unit");
  require(route_job_json.find("\"route_request_count\": 1") != std::string::npos,
          "pcb export-route-job reports request count");
  require(route_job_json.find("\"keepout_count\": 1") != std::string::npos,
          "pcb export-route-job reports keepout count");
  require(route_job_json.find("\"zone_count\": 1") != std::string::npos,
          "pcb export-route-job reports zone count");
  require(route_job_json.find("\"placement_region_count\": 1") != std::string::npos,
          "pcb export-route-job reports placement region count");
  require(route_job_json.find("\"board_outline\"") != std::string::npos,
          "pcb export-route-job includes board outline");
  require(route_job_json.find("\"physical_objects\"") != std::string::npos,
          "pcb export-route-job includes physical objects");
  require(route_job_json.find("\"keepouts\"") != std::string::npos,
          "pcb export-route-job includes keepouts");
  require(route_job_json.find("\"zones\"") != std::string::npos,
          "pcb export-route-job includes zones");
  require(route_job_json.find("\"placement_regions\"") != std::string::npos,
          "pcb export-route-job includes placement regions");
  require(route_job_json.find("\"route_requests\"") != std::string::npos,
          "pcb export-route-job includes route requests");
  require(route_job_json.find("\"id\": \"RR1\"") != std::string::npos,
          "pcb export-route-job includes route request id");
  const std::filesystem::path filtered_route_job_path = temp / "filtered-route-job.json";
  const std::string filtered_route_job_command =
      quote(CCAD_BINARY) + " pcb export-route-job --file " + quote(board_project_path) +
      " --request-id RR1 > " + quote(filtered_route_job_path);
  require(run(filtered_route_job_command) == 0,
          "pcb export-route-job filters by request id");
  const std::string filtered_route_job_json = readFile(filtered_route_job_path);
  require(filtered_route_job_json.find("\"route_request_count\": 1") != std::string::npos,
          "pcb export-route-job filtered summary reports one request");
  require(filtered_route_job_json.find("\"id\": \"RR1\"") != std::string::npos,
          "pcb export-route-job filtered output includes requested id");
  require(run(quote(CCAD_BINARY) + " pcb export-route-job --file " + quote(board_project_path) +
              " --request-id RR_MISSING") != 0,
          "pcb export-route-job rejects missing request id");
  const std::string remove_route_request_command =
      quote(CCAD_BINARY) + " pcb remove-route-request --file " + quote(board_project_path) +
      " --id RR1";
  require(run(remove_route_request_command) == 0, "pcb remove-route-request exits zero");
  const std::string remove_route_request_json = readFile(board_project_path);
  require(remove_route_request_json.find("\"id\": \"RR1\"") == std::string::npos,
          "pcb remove-route-request removes request id");
  require(run(remove_route_request_command) != 0,
          "pcb remove-route-request rejects missing request");

  require(run(add_pad_command) != 0, "pcb add-pad rejects duplicate id");
  require(run(add_keepout_command) != 0, "pcb add-keepout rejects duplicate id");
  require(run(add_zone_command) != 0, "pcb add-zone rejects duplicate id");
  require(run(add_placement_region_command) != 0,
          "pcb add-placement-region rejects duplicate id");

  const std::filesystem::path apply_route_board_path = temp / "apply-route-board.ccad.json";
  const std::string apply_route_init_command =
      quote(CCAD_BINARY) +
      " init --name apply-route-board --width-mm 42 --height-mm 28 --out " +
      quote(apply_route_board_path);
  require(run(apply_route_init_command) == 0, "apply route board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(apply_route_board_path) +
              " --id ARP1 --component U1 --pin 1 --net N1 --layers F.Cu"
              " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0") == 0,
          "apply route fixture add pad exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-via --file " + quote(apply_route_board_path) +
              " --id ARV1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4") == 0,
          "apply route fixture add via exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-route-request --file " +
              quote(apply_route_board_path) +
              " --id ARR1 --net N1 --from ARP1 --to ARV1 --preferred-layer F.Cu"
              " --policy straight --width-mm 0.25") == 0,
          "apply route fixture add request exits zero");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR1 --track-id ART1 --layer F.Cu"
              " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9") == 0,
          "pcb apply-route-segment exits zero");
  const std::string applied_route_json = readFile(apply_route_board_path);
  require(applied_route_json.find("\"id\": \"ART1\"") != std::string::npos,
          "pcb apply-route-segment writes track id");
  require(applied_route_json.find("\"width_nm\": 250000") != std::string::npos,
          "pcb apply-route-segment uses request width");
  require(applied_route_json.find("\"source_route_request_id\": \"ARR1\"") !=
              std::string::npos,
          "pcb apply-route-segment records route request provenance");
  const std::filesystem::path applied_track_lookup_path = temp / "applied-track-lookup.json";
  require(run(quote(CCAD_BINARY) + " pcb get-object --file " + quote(apply_route_board_path) +
              " --id ART1 > " + quote(applied_track_lookup_path)) == 0,
          "pcb get-object finds applied route track");
  require(readFile(applied_track_lookup_path)
              .find("\"source_route_request_id\": \"ARR1\"") != std::string::npos,
          "pcb get-object reports route track provenance");
  const std::filesystem::path applied_track_list_path = temp / "applied-track-list.json";
  require(run(quote(CCAD_BINARY) + " pcb list-objects --file " + quote(apply_route_board_path) +
              " --type track > " + quote(applied_track_list_path)) == 0,
          "pcb list-objects lists applied route tracks");
  require(readFile(applied_track_list_path)
              .find("\"source_route_request_id\": \"ARR1\"") != std::string::npos,
          "pcb list-objects reports route track provenance");
  const std::filesystem::path completed_route_status_path = temp / "completed-route-status.json";
  require(run(quote(CCAD_BINARY) + " pcb route-status --file " + quote(apply_route_board_path) +
              " > " + quote(completed_route_status_path)) == 0,
          "pcb route-status exits after completed route");
  require(readFile(completed_route_status_path).find("\"completed\": 1") != std::string::npos,
          "pcb route-status counts completed request");
  require(readFile(completed_route_status_path).find("\"status\": \"completed\"") !=
              std::string::npos,
          "pcb route-status reports completed status row");
  require(applied_route_json.find("\"id\": \"ARR1\"") == std::string::npos,
          "pcb apply-route-segment removes satisfied request");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR1 --track-id ART2 --layer F.Cu"
              " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9") != 0,
          "pcb apply-route-segment rejects missing request");
  require(run(quote(CCAD_BINARY) + " pcb add-route-request --file " +
              quote(apply_route_board_path) +
              " --id ARR2 --net N1 --from ARP1 --to ARV1 --preferred-layer F.Cu"
              " --policy straight --width-mm 0.25") == 0,
          "apply route fixture re-adds request exits zero");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR2 --track-id ART2 --layer B.Cu"
              " --start-x-mm 5 --start-y-mm 6 --end-x-mm 60 --end-y-mm 9") != 0,
          "pcb apply-route-segment rejects outside board");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR2 --track-id ART2 --layer F.Cu"
              " --start-x-mm 5 --start-y-mm 6 --end-x-mm 6 --end-y-mm 7"
              " --complete false") == 0,
          "pcb apply-route-segment can keep request open");
  const std::string partial_route_json = readFile(apply_route_board_path);
  require(partial_route_json.find("\"id\": \"ART2\"") != std::string::npos,
          "pcb apply-route-segment writes partial track id");
  require(partial_route_json.find("\"id\": \"ARR2\"") != std::string::npos,
          "pcb apply-route-segment keeps incomplete request");
  const std::filesystem::path partial_route_status_path = temp / "partial-route-status.json";
  require(run(quote(CCAD_BINARY) + " pcb route-status --file " + quote(apply_route_board_path) +
              " > " + quote(partial_route_status_path)) == 0,
          "pcb route-status exits after partial route");
  require(readFile(partial_route_status_path).find("\"partial\": 1") != std::string::npos,
          "pcb route-status counts partial request");
  require(readFile(partial_route_status_path).find("\"status\": \"partial\"") !=
              std::string::npos,
          "pcb route-status reports partial status row");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR2 --track-id ART3 --layer F.Cu"
              " --start-x-mm 6 --start-y-mm 7 --end-x-mm 8 --end-y-mm 9"
              " --complete maybe") != 0,
          "pcb apply-route-segment rejects invalid completion flag");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR2 --track-id ART3 --layer F.Cu"
              " --start-x-mm 6 --start-y-mm 7 --end-x-mm 8 --end-y-mm 9"
              " --complete true") == 0,
          "pcb apply-route-segment completes open request");
  const std::string complete_route_json = readFile(apply_route_board_path);
  require(complete_route_json.find("\"id\": \"ART3\"") != std::string::npos,
          "pcb apply-route-segment writes final track id");
  require(complete_route_json.find("\"id\": \"ARR2\"") == std::string::npos,
          "pcb apply-route-segment removes completed request");
  require(run(quote(CCAD_BINARY) + " pcb add-route-request --file " +
              quote(apply_route_board_path) +
              " --id ARR3 --net N1 --from ARP1 --to ARV1 --preferred-layer B.Cu"
              " --policy straight --width-mm 0.25") == 0,
          "apply route fixture adds default-layer request");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-segment --file " +
              quote(apply_route_board_path) +
              " --request-id ARR3 --track-id ART4"
              " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9") == 0,
          "pcb apply-route-segment defaults to request preferred layer");
  const std::string default_layer_route_json = readFile(apply_route_board_path);
  require(default_layer_route_json.find("\"id\": \"ART4\"") != std::string::npos,
          "pcb apply-route-segment writes default-layer track id");
  require(default_layer_route_json.find("\"layer_id\": \"B.Cu\"") != std::string::npos,
          "pcb apply-route-segment writes request preferred layer");
  require(run(quote(CCAD_BINARY) + " pcb add-route-request --file " +
              quote(apply_route_board_path) +
              " --id ARR4 --net N1 --from ARP1 --to ARV1 --preferred-layer F.Cu"
              " --policy polyline --width-mm 0.25") == 0,
          "apply route fixture adds polyline request");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-polyline --file " +
              quote(apply_route_board_path) +
              " --request-id ARR4 --track-prefix ARP --points-mm \"5,6;6,7;8,9\""
              " --complete false") == 0,
          "pcb apply-route-polyline writes multiple route segments");
  const std::string polyline_route_json = readFile(apply_route_board_path);
  require(polyline_route_json.find("\"id\": \"ARP.1\"") != std::string::npos,
          "pcb apply-route-polyline writes first generated track id");
  require(polyline_route_json.find("\"id\": \"ARP.2\"") != std::string::npos,
          "pcb apply-route-polyline writes second generated track id");
  require(polyline_route_json.find("\"source_route_request_id\": \"ARR4\"") !=
              std::string::npos,
          "pcb apply-route-polyline records route request provenance");
  require(polyline_route_json.find("\"id\": \"ARR4\"") != std::string::npos,
          "pcb apply-route-polyline can leave request open");
  const std::filesystem::path polyline_status_path = temp / "polyline-route-status.json";
  require(run(quote(CCAD_BINARY) + " pcb route-status --file " + quote(apply_route_board_path) +
              " > " + quote(polyline_status_path)) == 0,
          "pcb route-status exits after polyline route");
  require(readFile(polyline_status_path).find("\"routed_segment_count\": 2") !=
              std::string::npos,
          "pcb route-status counts polyline route segments");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-polyline --file " +
              quote(apply_route_board_path) +
              " --request-id ARR4 --track-prefix ARP --points-mm \"5,6;6,7\"") != 0,
          "pcb apply-route-polyline rejects duplicate generated track ids");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-polyline --file " +
              quote(apply_route_board_path) +
              " --request-id ARR4 --track-prefix ARZ --points-mm \"5,6;5,6\"") != 0,
          "pcb apply-route-polyline rejects zero-length segment");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-polyline --file " +
              quote(apply_route_board_path) +
              " --request-id ARR4 --track-prefix ARO --points-mm \"5,6;60,7\"") != 0,
          "pcb apply-route-polyline rejects out-of-board point");
  require(run(quote(CCAD_BINARY) + " pcb apply-route-polyline --file " +
              quote(apply_route_board_path) +
              " --request-id ARR4 --track-prefix ARC --points-mm \"8,9;9,10\""
              " --complete true") == 0,
          "pcb apply-route-polyline can complete open request");
  const std::string complete_polyline_json = readFile(apply_route_board_path);
  require(complete_polyline_json.find("\"id\": \"ARC.1\"") != std::string::npos,
          "pcb apply-route-polyline writes completion segment");
  require(complete_polyline_json.find("\"id\": \"ARR4\"") == std::string::npos,
          "pcb apply-route-polyline removes completed request");

  const std::filesystem::path remove_board_path = temp / "remove-board.ccad.json";
  const std::string remove_board_init_command =
      quote(CCAD_BINARY) +
      " init --name remove-board --width-mm 42 --height-mm 28 --out " +
      quote(remove_board_path);
  require(run(remove_board_init_command) == 0, "remove board init exits zero");
  const std::string remove_add_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(remove_board_path) +
      " --id RP1 --component U1 --pin 1 --net N1 --layers F.Cu"
      " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0";
  const std::string remove_add_via_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(remove_board_path) +
      " --id RV1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
  const std::string remove_add_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(remove_board_path) +
      " --id RT1 --net N1 --layer F.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
  const std::string remove_add_keepout_command =
      quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(remove_board_path) +
      " --id RK1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3";
  const std::string remove_add_zone_command =
      quote(CCAD_BINARY) + " pcb add-zone --file " + quote(remove_board_path) +
      " --id RZ1 --name \"Remove zone\" --net N1 --layers F.Cu"
      " --x-mm 3 --y-mm 3 --width-mm 12 --height-mm 8"
      " --priority 1 --clearance-mm 0.2 --min-thickness-mm 0.25"
      " --pad-connection thermal";
  const std::string remove_add_region_command =
      quote(CCAD_BINARY) + " pcb add-placement-region --file " + quote(remove_board_path) +
      " --id RPR1 --kind component --x-mm 2 --y-mm 3 --width-mm 10 --height-mm 6";
  require(run(remove_add_pad_command) == 0, "remove fixture add pad exits zero");
  require(run(remove_add_via_command) == 0, "remove fixture add via exits zero");
  require(run(remove_add_track_command) == 0, "remove fixture add track exits zero");
  require(run(remove_add_keepout_command) == 0, "remove fixture add keepout exits zero");
  require(run(remove_add_zone_command) == 0, "remove fixture add zone exits zero");
  require(run(remove_add_region_command) == 0, "remove fixture add placement region exits zero");

  const std::filesystem::path remove_pad_result_path = temp / "remove-pad-result.json";
  const std::filesystem::path remove_via_result_path = temp / "remove-via-result.json";
  const std::filesystem::path remove_track_result_path = temp / "remove-track-result.json";
  const std::filesystem::path remove_keepout_result_path = temp / "remove-keepout-result.json";
  const std::filesystem::path remove_zone_result_path = temp / "remove-zone-result.json";
  const std::filesystem::path remove_region_result_path = temp / "remove-region-result.json";
  const std::string remove_pad_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RP1 > " + quote(remove_pad_result_path);
  const std::string remove_via_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RV1 > " + quote(remove_via_result_path);
  const std::string remove_track_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RT1 --mode bulk > " + quote(remove_track_result_path);
  const std::string remove_keepout_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RK1 > " + quote(remove_keepout_result_path);
  const std::string remove_zone_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RZ1 > " + quote(remove_zone_result_path);
  const std::string remove_region_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RPR1 > " + quote(remove_region_result_path);
  require(run(remove_pad_command) == 0, "pcb remove-object removes pad");
  require(run(remove_via_command) == 0, "pcb remove-object removes via");
  require(run(remove_track_command) == 0, "pcb remove-object removes track");
  require(run(remove_keepout_command) == 0, "pcb remove-object removes keepout");
  require(run(remove_zone_command) == 0, "pcb remove-object removes zone");
  require(run(remove_region_command) == 0, "pcb remove-object removes placement region");
  const std::string remove_pad_result = readFile(remove_pad_result_path);
  require(remove_pad_result.find("\"kicad_container_class\":\"BOARD_ITEM_CONTAINER\"") !=
              std::string::npos,
          "pcb remove-object reports KiCad board item container class");
  require(remove_pad_result.find("\"kicad_method\":\"Delete\"") != std::string::npos,
          "pcb remove-object reports KiCad delete semantics");
  require(remove_pad_result.find("\"remove_mode\":\"normal\"") != std::string::npos,
          "pcb remove-object reports default remove mode");
  require(remove_pad_result.find("\"kind\":\"pad\"") != std::string::npos,
          "pcb remove-object reports removed pad kind");
  require(remove_pad_result.find("\"kicad_delete_semantics\":true") != std::string::npos,
          "pcb remove-object reports board-item delete semantics");
  const std::string remove_track_result = readFile(remove_track_result_path);
  require(remove_track_result.find("\"remove_mode\":\"bulk\"") != std::string::npos,
          "pcb remove-object accepts KiCad bulk remove mode");
  require(remove_track_result.find("\"kind\":\"track\"") != std::string::npos,
          "pcb remove-object reports removed track kind");
  const std::string removed_objects_json = readFile(remove_board_path);
  require(removed_objects_json.find("\"id\": \"RP1\"") == std::string::npos,
          "pcb remove-object deletes pad id");
  require(removed_objects_json.find("\"id\": \"RV1\"") == std::string::npos,
          "pcb remove-object deletes via id");
  require(removed_objects_json.find("\"id\": \"RT1\"") == std::string::npos,
          "pcb remove-object deletes track id");
  require(removed_objects_json.find("\"id\": \"RK1\"") == std::string::npos,
          "pcb remove-object deletes keepout id");
  require(removed_objects_json.find("\"id\": \"RZ1\"") == std::string::npos,
          "pcb remove-object deletes zone id");
  require(removed_objects_json.find("\"id\": \"RPR1\"") == std::string::npos,
          "pcb remove-object deletes placement region id");
  require(run(remove_pad_command) != 0, "pcb remove-object rejects missing object");

  const std::filesystem::path move_board_path = temp / "move-board.ccad.json";
  const std::string move_board_init_command =
      quote(CCAD_BINARY) +
      " init --name move-board --width-mm 42 --height-mm 28 --out " + quote(move_board_path);
  require(run(move_board_init_command) == 0, "move board init exits zero");
  const std::string move_add_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(move_board_path) +
      " --id MP1 --component U1 --pin 1 --net N1 --layers F.Cu"
      " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0";
  const std::string move_add_via_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(move_board_path) +
      " --id MV1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
  const std::string move_add_keepout_command =
      quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(move_board_path) +
      " --id MK1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3";
  const std::string move_add_region_command =
      quote(CCAD_BINARY) + " pcb add-placement-region --file " + quote(move_board_path) +
      " --id MPR1 --kind component --x-mm 2 --y-mm 3 --width-mm 10 --height-mm 6";
  require(run(move_add_pad_command) == 0, "move fixture add pad exits zero");
  require(run(move_add_via_command) == 0, "move fixture add via exits zero");
  require(run(move_add_keepout_command) == 0, "move fixture add keepout exits zero");
  require(run(move_add_region_command) == 0, "move fixture add placement region exits zero");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(move_board_path) +
              " --id MP1 --x-mm 7 --y-mm 8") == 0,
          "pcb move-object moves pad");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(move_board_path) +
              " --id MV1 --x-mm 9 --y-mm 10") == 0,
          "pcb move-object moves via");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(move_board_path) +
              " --id MK1 --x-mm 21 --y-mm 11") == 0,
          "pcb move-object moves keepout");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(move_board_path) +
              " --id MPR1 --x-mm 3 --y-mm 4") == 0,
          "pcb move-object moves placement region");
  const std::string moved_objects_json = readFile(move_board_path);
  require(moved_objects_json.find("\"x_nm\": 7000000") != std::string::npos,
          "pcb move-object writes moved pad x");
  require(moved_objects_json.find("\"x_nm\": 9000000") != std::string::npos,
          "pcb move-object writes moved via x");
  require(moved_objects_json.find("\"x_nm\": 21000000") != std::string::npos,
          "pcb move-object writes moved keepout x");
  require(moved_objects_json.find("\"x_nm\": 3000000") != std::string::npos,
          "pcb move-object writes moved placement region x");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(move_board_path) +
              " --id MP1 --x-mm 0.2 --y-mm 8") != 0,
          "pcb move-object rejects pad outside board");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(move_board_path) +
              " --id MISSING --x-mm 3 --y-mm 4") != 0,
          "pcb move-object rejects missing object");

  const std::filesystem::path resize_board_path = temp / "resize-board.ccad.json";
  const std::string resize_board_init_command =
      quote(CCAD_BINARY) +
      " init --name resize-board --width-mm 42 --height-mm 28 --out " +
      quote(resize_board_path);
  require(run(resize_board_init_command) == 0, "resize board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(resize_board_path) +
              " --id SP1 --component U1 --pin 1 --net N1 --layers F.Cu"
              " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0") == 0,
          "resize fixture add pad exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(resize_board_path) +
              " --id SK1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3") ==
              0,
          "resize fixture add keepout exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-placement-region --file " +
              quote(resize_board_path) +
              " --id SPR1 --kind component --x-mm 2 --y-mm 3 --width-mm 10 --height-mm 6") ==
              0,
          "resize fixture add placement region exits zero");
  require(run(quote(CCAD_BINARY) + " pcb resize-object --file " + quote(resize_board_path) +
              " --id SP1 --width-mm 2 --height-mm 1.2") == 0,
          "pcb resize-object resizes pad");
  require(run(quote(CCAD_BINARY) + " pcb resize-object --file " + quote(resize_board_path) +
              " --id SK1 --width-mm 5 --height-mm 4") == 0,
          "pcb resize-object resizes keepout");
  require(run(quote(CCAD_BINARY) + " pcb resize-object --file " + quote(resize_board_path) +
              " --id SPR1 --width-mm 11 --height-mm 7") == 0,
          "pcb resize-object resizes placement region");
  const std::string resized_objects_json = readFile(resize_board_path);
  require(resized_objects_json.find("\"width_nm\": 2000000") != std::string::npos,
          "pcb resize-object writes resized pad width");
  require(resized_objects_json.find("\"width_nm\": 5000000") != std::string::npos,
          "pcb resize-object writes resized keepout width");
  require(resized_objects_json.find("\"width_nm\": 11000000") != std::string::npos,
          "pcb resize-object writes resized placement region width");
  require(run(quote(CCAD_BINARY) + " pcb resize-object --file " + quote(resize_board_path) +
              " --id SP1 --width-mm 20 --height-mm 20") != 0,
          "pcb resize-object rejects pad outside board");
  require(run(quote(CCAD_BINARY) + " pcb resize-object --file " + quote(resize_board_path) +
              " --id MISSING --width-mm 2 --height-mm 2") != 0,
          "pcb resize-object rejects missing object");

  const std::filesystem::path set_track_board_path = temp / "set-track-board.ccad.json";
  const std::string set_track_board_init_command =
      quote(CCAD_BINARY) +
      " init --name set-track-board --width-mm 42 --height-mm 28 --out " +
      quote(set_track_board_path);
  require(run(set_track_board_init_command) == 0, "set track board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-track --file " + quote(set_track_board_path) +
              " --id ST1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6"
              " --end-x-mm 8 --end-y-mm 9 --width-mm 0.25") == 0,
          "set track fixture add track exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-layer --file " + quote(set_track_board_path) +
              " --id F.SilkS --name FrontSilkscreen --kind silkscreen") == 0,
          "set track fixture add silkscreen layer exits zero");
  require(run(quote(CCAD_BINARY) + " pcb set-track --file " + quote(set_track_board_path) +
              " --id ST1 --start-x-mm 6 --start-y-mm 7 --end-x-mm 10 --end-y-mm 11"
              " --width-mm 0.35 --net N2 --layer B.Cu") == 0,
          "pcb set-track updates track geometry and metadata");
  const std::string set_track_json = readFile(set_track_board_path);
  require(set_track_json.find("\"net_id\": \"N2\"") != std::string::npos,
          "pcb set-track writes net");
  require(set_track_json.find("\"layer_id\": \"B.Cu\"") != std::string::npos,
          "pcb set-track writes layer");
  require(set_track_json.find("\"x_nm\": 6000000") != std::string::npos,
          "pcb set-track writes start x");
  require(set_track_json.find("\"x_nm\": 10000000") != std::string::npos,
          "pcb set-track writes end x");
  require(set_track_json.find("\"width_nm\": 350000") != std::string::npos,
          "pcb set-track writes width");
  require(run(quote(CCAD_BINARY) + " pcb set-track --file " + quote(set_track_board_path) +
              " --id ST1 --start-x-mm 0.05 --start-y-mm 7 --end-x-mm 10 --end-y-mm 11"
              " --width-mm 0.35") != 0,
          "pcb set-track rejects copper outside board");
  require(run(quote(CCAD_BINARY) + " pcb set-track --file " + quote(set_track_board_path) +
              " --id MISSING --start-x-mm 6 --start-y-mm 7 --end-x-mm 10 --end-y-mm 11"
              " --width-mm 0.35") != 0,
          "pcb set-track rejects missing track");
  require(run(quote(CCAD_BINARY) + " pcb set-track --file " + quote(set_track_board_path) +
              " --id ST1 --start-x-mm 6 --start-y-mm 7 --end-x-mm 10 --end-y-mm 11"
              " --width-mm 0.35 --layer F.SilkS") != 0,
          "pcb set-track rejects non-copper layer metadata");

  const std::filesystem::path set_via_board_path = temp / "set-via-board.ccad.json";
  const std::string set_via_board_init_command =
      quote(CCAD_BINARY) +
      " init --name set-via-board --width-mm 42 --height-mm 28 --out " +
      quote(set_via_board_path);
  require(run(set_via_board_init_command) == 0, "set via board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-via --file " + quote(set_via_board_path) +
              " --id SV1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4") ==
              0,
          "set via fixture add via exits zero");
  require(run(quote(CCAD_BINARY) + " pcb set-via --file " + quote(set_via_board_path) +
              " --id SV1 --diameter-mm 1.0 --drill-mm 0.5 --net N2") == 0,
          "pcb set-via updates via geometry and metadata");
  const std::string set_via_json = readFile(set_via_board_path);
  require(set_via_json.find("\"net_id\": \"N2\"") != std::string::npos,
          "pcb set-via writes net");
  require(set_via_json.find("\"diameter_nm\": 1000000") != std::string::npos,
          "pcb set-via writes diameter");
  require(set_via_json.find("\"drill_nm\": 500000") != std::string::npos,
          "pcb set-via writes drill");
  require(run(quote(CCAD_BINARY) + " pcb set-via --file " + quote(set_via_board_path) +
              " --id SV1 --diameter-mm 0.4 --drill-mm 0.8") != 0,
          "pcb set-via rejects drill larger than diameter");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(set_via_board_path) +
              " --id SV1 --x-mm 0.5 --y-mm 9") == 0,
          "set via fixture moves via near edge");
  require(run(quote(CCAD_BINARY) + " pcb set-via --file " + quote(set_via_board_path) +
              " --id SV1 --diameter-mm 2.0 --drill-mm 0.5") != 0,
          "pcb set-via rejects geometry outside board");
  require(run(quote(CCAD_BINARY) + " pcb set-via --file " + quote(set_via_board_path) +
              " --id MISSING --diameter-mm 1.0 --drill-mm 0.5") != 0,
          "pcb set-via rejects missing via");

  const std::filesystem::path set_pad_board_path = temp / "set-pad-board.ccad.json";
  const std::string set_pad_board_init_command =
      quote(CCAD_BINARY) +
      " init --name set-pad-board --width-mm 42 --height-mm 28 --out " +
      quote(set_pad_board_path);
  require(run(set_pad_board_init_command) == 0, "set pad board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-layer --file " + quote(set_pad_board_path) +
              " --id F.SilkS --name FrontSilkscreen --kind silkscreen") == 0,
          "set pad fixture add silkscreen layer exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U1 --pin 1 --net N1 --layers F.Cu"
              " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0") == 0,
          "set pad fixture add pad exits zero");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layers B.Cu"
              " --rotation-deg 90") == 0,
          "pcb set-pad updates pad metadata");
  const std::string set_pad_json = readFile(set_pad_board_path);
  require(set_pad_json.find("\"component_id\": \"U2\"") != std::string::npos,
          "pcb set-pad writes component");
  require(set_pad_json.find("\"pin_name\": \"2\"") != std::string::npos,
          "pcb set-pad writes pin");
  require(set_pad_json.find("\"net_id\": \"N2\"") != std::string::npos,
          "pcb set-pad writes net");
  require(set_pad_json.find("\"B.Cu\"") != std::string::npos,
          "pcb set-pad writes layer");
  require(set_pad_json.find("\"rotation_degrees\": 90") != std::string::npos,
          "pcb set-pad writes rotation");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layers B.Cu"
              " --type smd --shape roundrect --roundrect-rratio 0.15"
              " --rotation-deg 90") == 0,
          "pcb set-pad updates pad shape metadata");
  const std::string set_pad_shape_json = readFile(set_pad_board_path);
  require(set_pad_shape_json.find("\"shape\": \"roundrect\"") != std::string::npos,
          "pcb set-pad writes shape");
  require(set_pad_shape_json.find("\"roundrect_rratio\": 0.15") != std::string::npos,
          "pcb set-pad writes roundrect ratio");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layers B.Cu"
              " --roundrect-rratio 0.75 --rotation-deg 90") != 0,
          "pcb set-pad rejects invalid roundrect ratio");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layers F.SilkS"
              " --rotation-deg 90") != 0,
          "pcb set-pad rejects non-copper layer");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(set_pad_board_path) +
              " --id SPD1 --x-mm 0.6 --y-mm 6") == 0,
          "set pad fixture moves pad near edge");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layers F.Cu"
              " --rotation-deg 0") != 0,
          "pcb set-pad rejects rotated pad outside board");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id MISSING --component U2 --pin 2 --net N2 --layers F.Cu"
              " --rotation-deg 0") != 0,
          "pcb set-pad rejects missing pad");

  const std::filesystem::path set_region_board_path = temp / "set-region-board.ccad.json";
  const std::string set_region_board_init_command =
      quote(CCAD_BINARY) +
      " init --name set-region-board --width-mm 42 --height-mm 28 --out " +
      quote(set_region_board_path);
  require(run(set_region_board_init_command) == 0, "set region board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(set_region_board_path) +
              " --id SRK1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3") ==
              0,
          "set region fixture add keepout exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-placement-region --file " +
              quote(set_region_board_path) +
              " --id SRPR1 --kind component --x-mm 2 --y-mm 3 --width-mm 10 --height-mm 6") ==
              0,
          "set region fixture add placement region exits zero");
  require(run(quote(CCAD_BINARY) + " pcb set-region-kind --file " +
              quote(set_region_board_path) + " --id SRK1 --kind routing") == 0,
          "pcb set-region-kind updates keepout");
  require(run(quote(CCAD_BINARY) + " pcb set-region-kind --file " +
              quote(set_region_board_path) + " --id SRPR1 --kind module") == 0,
          "pcb set-region-kind updates placement region");
  const std::string set_region_json = readFile(set_region_board_path);
  require(set_region_json.find("\"kind\": \"routing\"") != std::string::npos,
          "pcb set-region-kind writes keepout kind");
  require(set_region_json.find("\"kind\": \"module\"") != std::string::npos,
          "pcb set-region-kind writes placement region kind");
  require(run(quote(CCAD_BINARY) + " pcb set-region-kind --file " +
              quote(set_region_board_path) + " --id MISSING --kind routing") != 0,
          "pcb set-region-kind rejects missing region");

  const std::string bad_layer_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T_BAD --net N1 --layer Inner.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
  require(run(bad_layer_command) != 0, "pcb add-track rejects unknown layer");

  const std::string non_copper_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(board_project_path) +
      " --id P_SILK --component U1 --pin 1 --net N1 --layers F.SilkS"
      " --x-mm 10 --y-mm 6 --width-mm 1.0 --height-mm 1.0";
  require(run(non_copper_pad_command) != 0, "pcb add-pad rejects non-copper layer");

  const std::string non_copper_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T_SILK --net N1 --layer F.SilkS"
      " --start-x-mm 10 --start-y-mm 6 --end-x-mm 12 --end-y-mm 6 --width-mm 0.25";
  require(run(non_copper_track_command) != 0, "pcb add-track rejects non-copper layer");

  const std::string bad_via_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
      " --id V_BAD --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.4 --drill-mm 0.8";
  require(run(bad_via_command) != 0, "pcb add-via rejects drill larger than diameter");

  const std::string outside_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T_OUT --net N1 --layer F.Cu"
      " --start-x-mm 5 --start-y-mm 6 --end-x-mm 99 --end-y-mm 9 --width-mm 0.25";
  require(run(outside_track_command) != 0, "pcb add-track rejects endpoint outside board");

  const std::string edge_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(board_project_path) +
      " --id P_EDGE --component U1 --pin 1 --net N1 --layers F.Cu"
      " --x-mm 0.2 --y-mm 6 --width-mm 1.0 --height-mm 1.0";
  require(run(edge_pad_command) != 0, "pcb add-pad rejects geometry outside board");

  const std::string edge_via_command =
      quote(CCAD_BINARY) + " pcb add-via --file " + quote(board_project_path) +
      " --id V_EDGE --net N1 --x-mm 0.2 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4";
  require(run(edge_via_command) != 0, "pcb add-via rejects geometry outside board");

  const std::string edge_track_command =
      quote(CCAD_BINARY) + " pcb add-track --file " + quote(board_project_path) +
      " --id T_EDGE --net N1 --layer F.Cu"
      " --start-x-mm 0.05 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25";
  require(run(edge_track_command) != 0, "pcb add-track rejects copper outside board");

  const std::string outside_keepout_command =
      quote(CCAD_BINARY) + " pcb add-keepout --file " + quote(board_project_path) +
      " --id K_OUT --kind placement --x-mm 40 --y-mm 26 --width-mm 4 --height-mm 3";
  require(run(outside_keepout_command) != 0, "pcb add-keepout rejects area outside board");

  const std::string outside_placement_region_command =
      quote(CCAD_BINARY) + " pcb add-placement-region --file " + quote(board_project_path) +
      " --id PR_OUT --kind component --x-mm 40 --y-mm 26 --width-mm 4 --height-mm 3";
  require(run(outside_placement_region_command) != 0,
          "pcb add-placement-region rejects area outside board");

  const std::string set_outline_command =
      quote(CCAD_BINARY) + " pcb set-outline --file " + quote(board_project_path) +
      " --x-mm 2 --y-mm 2 --width-mm 50 --height-mm 30";
  require(run(set_outline_command) == 0, "pcb set-outline exits zero");
  const std::string outline_json = readFile(board_project_path);
  require(outline_json.find("\"x_nm\": 2000000") != std::string::npos,
          "pcb set-outline writes origin x");
  require(outline_json.find("\"y_nm\": 2000000") != std::string::npos,
          "pcb set-outline writes origin y");
  require(outline_json.find("\"width_nm\": 50000000") != std::string::npos,
          "pcb set-outline writes width");
  require(outline_json.find("\"height_nm\": 30000000") != std::string::npos,
          "pcb set-outline writes height");

  const std::filesystem::path get_outline_path = temp / "get-outline.json";
  require(run(quote(CCAD_BINARY) + " pcb get-outline --file " + quote(board_project_path) +
              " > " + quote(get_outline_path)) == 0,
          "pcb get-outline exits zero");
  const std::string get_outline_json = readFile(get_outline_path);
  require(get_outline_json.find("\"outline_kind\": \"ccad_board_outline\"") !=
              std::string::npos,
          "pcb get-outline reports outline kind");
  require(get_outline_json.find("\"kicad_handler\": \"GetBoundingBox\"") != std::string::npos,
          "pcb get-outline records KiCad bounding-box reference");
  require(get_outline_json.find("\"kicad_class\": \"BOARD_BOUNDING_BOX\"") !=
              std::string::npos,
          "pcb get-outline reports KiCad bounding-box class");
  require(get_outline_json.find("\"kicad_view_layer\": \"LAYER_BOARD_BOUNDING_BOX\"") !=
              std::string::npos,
          "pcb get-outline reports KiCad bounding-box view layer");
  require(get_outline_json.find("\"kicad_skip_struct\": true") != std::string::npos,
          "pcb get-outline reports KiCad skip-struct behavior");
  require(get_outline_json.find("\"bounding_box\"") != std::string::npos,
          "pcb get-outline reports explicit bounding box payload");
  require(get_outline_json.find("\"x_nm\": 2000000") != std::string::npos,
          "pcb get-outline reports origin x");
  require(get_outline_json.find("\"y_nm\": 2000000") != std::string::npos,
          "pcb get-outline reports origin y");
  require(get_outline_json.find("\"width_nm\": 50000000") != std::string::npos,
          "pcb get-outline reports width");
  require(get_outline_json.find("\"height_nm\": 30000000") != std::string::npos,
          "pcb get-outline reports height");

  const std::filesystem::path outline_polygon_board_path = temp / "outline-polygon-board.ccad.json";
  require(run(quote(CCAD_BINARY) +
              " init --name outline-polygon-board --width-mm 42 --height-mm 28 --out " +
              quote(outline_polygon_board_path)) == 0,
          "outline polygon board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-standard-layers --file " +
              quote(outline_polygon_board_path)) == 0,
          "pcb add-standard-layers prepares Edge.Cuts for outline polygon reporting");
  const std::string edge_cut_a =
      quote(CCAD_BINARY) + " pcb add-graphic-line --file " + quote(outline_polygon_board_path) +
      " --id E1 --layer Edge.Cuts --start-x-mm 5 --start-y-mm 5"
      " --end-x-mm 25 --end-y-mm 5 --width-mm 0.05";
  require(run(edge_cut_a) == 0, "pcb add-graphic-line writes first Edge.Cuts segment");
  const std::string edge_cut_b =
      quote(CCAD_BINARY) + " pcb add-graphic-line --file " + quote(outline_polygon_board_path) +
      " --id E2 --layer Edge.Cuts --start-x-mm 25 --start-y-mm 5"
      " --end-x-mm 25 --end-y-mm 20 --width-mm 0.05";
  require(run(edge_cut_b) == 0, "pcb add-graphic-line writes second Edge.Cuts segment");
  const std::string edge_cut_c =
      quote(CCAD_BINARY) + " pcb add-graphic-line --file " + quote(outline_polygon_board_path) +
      " --id E3 --layer Edge.Cuts --start-x-mm 25 --start-y-mm 20"
      " --end-x-mm 5 --end-y-mm 20 --width-mm 0.05";
  require(run(edge_cut_c) == 0, "pcb add-graphic-line writes third Edge.Cuts segment");
  const std::string edge_cut_d =
      quote(CCAD_BINARY) + " pcb add-graphic-line --file " + quote(outline_polygon_board_path) +
      " --id E4 --layer Edge.Cuts --start-x-mm 5 --start-y-mm 20"
      " --end-x-mm 5 --end-y-mm 5 --width-mm 0.05";
  require(run(edge_cut_d) == 0, "pcb add-graphic-line writes fourth Edge.Cuts segment");
  const std::filesystem::path outline_polygon_path = temp / "outline-polygon.json";
  require(run(quote(CCAD_BINARY) + " pcb outline-polygon --file " +
              quote(outline_polygon_board_path) + " > " + quote(outline_polygon_path)) == 0,
          "pcb outline-polygon exits zero");
  const std::string outline_polygon_json = readFile(outline_polygon_path);
  require(outline_polygon_json.find("\"kicad_function\": \"ConvertOutlineToPolygon\"") !=
              std::string::npos,
          "pcb outline-polygon reports KiCad function");
  require(outline_polygon_json.find("\"edge_cut_segment_count\": 4") != std::string::npos,
          "pcb outline-polygon counts Edge.Cuts segments");
  require(outline_polygon_json.find("\"closed\": true") != std::string::npos,
          "pcb outline-polygon reports closed outline");
  require(outline_polygon_json.find("\"valid\": true") != std::string::npos,
          "pcb outline-polygon reports valid outline");
  require(outline_polygon_json.find("\"used_inferred_outline\": false") != std::string::npos,
          "pcb outline-polygon does not infer when Edge.Cuts are closed");
  require(outline_polygon_json.find("\"width_nm\": 20000000") != std::string::npos,
          "pcb outline-polygon reports chained bounding box width");
  require(outline_polygon_json.find("\"height_nm\": 15000000") != std::string::npos,
          "pcb outline-polygon reports chained bounding box height");

  const std::filesystem::path cross_probe_path = temp / "cross-probe.json";
  require(run(quote(CCAD_BINARY) + " pcb cross-probe --file " + quote(board_project_path) +
              " --packet " + shellArgument("\\$NET: \"N1\"") + " > " + quote(cross_probe_path)) == 0,
          "pcb cross-probe exits zero");
  const std::string cross_probe_json = readFile(cross_probe_path);
  require(cross_probe_json.find("\"kicad_source\": \"pcbnew/cross-probing.cpp\"") !=
              std::string::npos,
          "pcb cross-probe reports KiCad source file");
  require(cross_probe_json.find("\"packet_kind\": \"net\"") != std::string::npos,
          "pcb cross-probe reports net packet kind; output: " + cross_probe_json);
  require(cross_probe_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb cross-probe resolves same-net pad target");
  require(cross_probe_json.find("\"id\": \"V1\"") != std::string::npos,
          "pcb cross-probe resolves same-net via target");
  require(cross_probe_json.find("\"id\": \"T1\"") != std::string::npos,
          "pcb cross-probe resolves same-net track target");
  require(cross_probe_json.find("\"pending_kicad_features\"") != std::string::npos,
          "pcb cross-probe reports remaining KiCad IPC parity features");

  const std::string invalid_outline_command =
      quote(CCAD_BINARY) + " pcb set-outline --file " + quote(board_project_path) +
      " --x-mm 10 --y-mm 10 --width-mm 5 --height-mm 5";
  require(run(invalid_outline_command) != 0,
          "pcb set-outline rejects outline that excludes existing objects");

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
          << "  \"symbols\": [],\n"
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
  require(inspect_output.find("\"symbols\": 0") != std::string::npos,
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
  require(board_inspect_output.find("\"x_nm\": 2000000") != std::string::npos,
          "inspect reports board origin x");
  require(board_inspect_output.find("\"y_nm\": 2000000") != std::string::npos,
          "inspect reports board origin y");
  require(board_inspect_output.find("\"width_nm\": 50000000") != std::string::npos,
          "inspect reports board width");
  require(board_inspect_output.find("\"layers\": 4") != std::string::npos,
          "inspect reports layer count");
  require(board_inspect_output.find("\"pads\": 1") != std::string::npos,
          "inspect reports pad count");
  require(board_inspect_output.find("\"vias\": 1") != std::string::npos,
          "inspect reports via count");
  require(board_inspect_output.find("\"tracks\": 1") != std::string::npos,
          "inspect reports track count");
  require(board_inspect_output.find("\"route_requests\": 0") != std::string::npos,
          "inspect reports route request count");
  require(board_inspect_output.find("\"route_progress\"") != std::string::npos,
          "inspect reports route progress object");
  require(board_inspect_output.find("\"routed_segments\": 0") != std::string::npos,
          "inspect reports routed segment count");
  require(board_inspect_output.find("\"placement_regions\": 1") != std::string::npos,
          "inspect reports placement region count");
  require(board_inspect_output.find("\"keepouts\": 1") != std::string::npos,
          "inspect reports keepout count");

  std::string board_with_net = readFile(board_project_path);
  const std::string empty_symbols = "  \"symbols\": [\n  ]";
  const std::string empty_components = "  \"components\": [\n  ]";
  const std::string logical_u1_symbols =
      "  \"symbols\": [\n"
      "    {\n"
      "      \"id\": \"U1\",\n"
      "      \"part\": \"test-component\",\n"
      "      \"pins\": [\n"
      "        {\"name\": \"1\", \"kind\": \"passive\"}\n"
      "      ]\n"
      "    }\n"
      "  ]";
  const std::string logical_u1_components =
      "  \"components\": [\n"
      "    {\n"
      "      \"id\": \"U1\",\n"
      "      \"part\": \"test-component\",\n"
      "      \"pins\": [\n"
      "        {\"name\": \"1\", \"kind\": \"passive\"}\n"
      "      ]\n"
      "    }\n"
      "  ]";
  const std::string empty_nets = "  \"nets\": [\n  ]";
  const std::string logical_n1 =
      "  \"nets\": [\n"
      "    {\n"
      "      \"id\": \"N1\",\n"
      "      \"members\": [\n"
      "        {\"component_id\": \"U1\", \"pin_name\": \"1\"}\n"
      "      ]\n"
      "    }\n"
      "  ]";
  const std::size_t symbols_position = board_with_net.find(empty_symbols);
  const std::size_t components_position = board_with_net.find(empty_components);
  require(symbols_position != std::string::npos || components_position != std::string::npos,
          "board fixture has empty schematic symbol list before clean drc");
  if (symbols_position != std::string::npos) {
    board_with_net.replace(symbols_position, empty_symbols.size(), logical_u1_symbols);
  } else {
    board_with_net.replace(components_position, empty_components.size(), logical_u1_components);
  }
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
  require(readFile(drc_output_path).find("\"summary\": {") != std::string::npos,
          "drc writes diagnostic summary json");
  require(readFile(drc_output_path).find("\"total\": 0") != std::string::npos,
          "drc summary reports zero diagnostics for clean board");

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
              << "  \"symbols\": [],\n"
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
  require(readFile(drc_output_path).find("\"errors\": ") != std::string::npos,
          "drc summary reports error count");

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
          << "      \"usage_summary\": \"0603 resistor footprint for compact passive placement\",\n"
          << "      \"layout_notes\": [\n"
          << "        \"keep near the driven net and check assembly spacing\"\n"
          << "      ],\n"
          << "      \"source_confidence\": \"library_metadata\",\n"
          << "      \"review_status\": \"reviewed\",\n"
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
  require(catalog_find_output.find("\"usage_summary\": \"0603 resistor footprint for compact passive placement\"") !=
              std::string::npos,
          "lib catalog-find writes usage summary");
  require(catalog_find_output.find("keep near the driven net") != std::string::npos,
          "lib catalog-find writes layout notes");
  require(catalog_find_output.find("\"source_confidence\": \"library_metadata\"") !=
              std::string::npos,
          "lib catalog-find writes source confidence");
  require(catalog_find_output.find("\"review_status\": \"reviewed\"") != std::string::npos,
          "lib catalog-find writes review status");
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
  require(catalog_search_output.find("\"summary\": {") != std::string::npos,
          "lib catalog-search writes summary");
  require(catalog_search_output.find("\"match_count\": 2") != std::string::npos,
          "lib catalog-search summary writes match count");
  require(catalog_search_output.find("\"count\": 2") != std::string::npos,
          "lib catalog-search writes match count");
  require(catalog_search_output.find("\"id\": \"footprint:Resistor_SMD:R_0603_1608Metric\"") !=
              std::string::npos,
          "lib catalog-search writes resistor match");
  require(catalog_search_output.find("\"id\": \"footprint:Capacitor_SMD:C_0603_1608Metric\"") !=
              std::string::npos,
          "lib catalog-search writes capacitor match");
  const std::string catalog_search_reviewed_command =
      quote(CCAD_BINARY) + " lib catalog-search --catalog " + quote(catalog_path) +
      " --query reviewed > " + quote(catalog_search_path);
  require(run(catalog_search_reviewed_command) == 0,
          "lib catalog-search review status exits zero");
  const std::string catalog_search_reviewed_output = readFile(catalog_search_path);
  require(catalog_search_reviewed_output.find("\"count\": 1") != std::string::npos,
          "lib catalog-search finds reviewed item");
  require(catalog_search_reviewed_output.find("\"usage_summary\": \"0603 resistor footprint for compact passive placement\"") !=
              std::string::npos,
          "lib catalog-search writes usage summary");
  require(catalog_search_reviewed_output.find("\"review_status\": \"reviewed\"") !=
              std::string::npos,
          "lib catalog-search writes review status");
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
  require(readFile(catalog_validate_path).find("\"summary\": {") != std::string::npos,
          "lib catalog-validate writes diagnostic summary");
  require(readFile(catalog_validate_path).find("\"total\": 0") != std::string::npos,
          "lib catalog-validate clean summary reports zero diagnostics");

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
  require(bad_catalog_validate_output.find("\"errors\": ") != std::string::npos,
          "lib catalog-validate summary reports error count");

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
      " --component R1 --at-x-mm 10 --at-y-mm 12 --layer F.Cu --value 10k";
  require(run(place_footprint_command) == 0, "pcb place-footprint exits zero");
  const std::string placed_footprint_project = readFile(board_project_path);
  require(placed_footprint_project.find("\"footprints\": [") != std::string::npos,
          "pcb place-footprint writes board footprint metadata");
  require(placed_footprint_project.find("\"reference\": \"R1\"") != std::string::npos,
          "pcb place-footprint writes board footprint reference");
  require(placed_footprint_project.find("\"footprint_name\": \"R_0805_2012Metric\"") !=
              std::string::npos,
          "pcb place-footprint writes board footprint name");
  require(placed_footprint_project.find("\"value\": \"10k\"") != std::string::npos,
          "pcb place-footprint writes board footprint value");
  require(placed_footprint_project.find("\"id\": \"R1.1\"") != std::string::npos,
          "pcb place-footprint writes first pad id");
  require(placed_footprint_project.find("\"component_id\": \"R1\"") != std::string::npos,
          "pcb place-footprint writes component id");
  require(placed_footprint_project.find("\"pin_name\": \"1\"") != std::string::npos,
          "pcb place-footprint writes pin name");
  require(placed_footprint_project.find("\"x_nm\": 9050000") != std::string::npos,
          "pcb place-footprint translates pad x");
  require(run(place_footprint_command) != 0, "pcb place-footprint rejects duplicate pad ids");

  const std::filesystem::path board_bom_path = temp / "board-bom.csv";
  const std::string export_board_bom_command =
      quote(CCAD_BINARY) + " pcb export-board-bom --file " + quote(board_project_path) +
      " --output " + quote(board_bom_path);
  require(run(export_board_bom_command) == 0, "pcb export-board-bom exits zero");
  const std::string board_bom_output = readFile(board_bom_path);
  require(board_bom_output.find("\"Id\";\"Designator\";\"Footprint\";\"Quantity\";\"Designation\";\"Supplier and ref\";") !=
              std::string::npos,
          "pcb export-board-bom writes KiCad legacy header");
  require(board_bom_output.find("1;\"R1\";\"R_0805_2012Metric\";1;\"10k\";;;") !=
              std::string::npos,
          "pcb export-board-bom writes placed footprint row");

  const std::filesystem::path cleanup_actions_path = temp / "cleanup-actions.json";
  require(run(quote(CCAD_BINARY) + " pcb cleanup-actions > " +
              quote(cleanup_actions_path)) == 0,
          "pcb cleanup-actions exits zero");
  const std::string cleanup_actions_json = readFile(cleanup_actions_path);
  require(cleanup_actions_json.find("\"kicad_class\": \"CLEANUP_ITEM\"") != std::string::npos,
          "pcb cleanup-actions reports KiCad cleanup item class");
  require(cleanup_actions_json.find("\"id\": \"shorting_track\"") != std::string::npos,
          "pcb cleanup-actions reports shorting track action");
  require(cleanup_actions_json.find("\"title\": \"Remove track shorting two nets\"") !=
              std::string::npos,
          "pcb cleanup-actions reports shorting track title");
  require(cleanup_actions_json.find("\"id\": \"lines_to_rect\"") != std::string::npos,
          "pcb cleanup-actions reports graphics cleanup action");
  require(cleanup_actions_json.find("\"provider_semantics\": \"vector_indexed_rows\"") !=
              std::string::npos,
          "pcb cleanup-actions reports vector provider semantics");

  const std::string bad_place_layer_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(board_project_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component R2 --at-x-mm 10 --at-y-mm 12 --layer Inner.Cu";
  require(run(bad_place_layer_command) != 0, "pcb place-footprint rejects unknown layer");

  const std::string non_copper_place_layer_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(board_project_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component R_SILK --at-x-mm 10 --at-y-mm 12 --layer F.SilkS";
  require(run(non_copper_place_layer_command) != 0,
          "pcb place-footprint rejects non-copper layer");

  const std::string edge_place_command =
      quote(CCAD_BINARY) + " pcb place-footprint --file " + quote(board_project_path) +
      " --footprint " + quote(footprint_out_path) +
      " --component R_EDGE --at-x-mm 1 --at-y-mm 12 --layer F.Cu";
  require(run(edge_place_command) != 0,
          "pcb place-footprint rejects pad geometry outside board");

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
               << "  \"symbols\": [\n"
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

  const std::filesystem::path autoplace_board_path = temp / "autoplace-board.ccad.json";
  const std::filesystem::path autoplace_footprint_path = temp / "autoplace-footprint.json";
  const std::filesystem::path autoplace_result_path = temp / "autoplace-result.json";
  require(run(quote(CCAD_BINARY) + " init --name autoplace-board --width-mm 7 --height-mm 4 --out " +
              quote(autoplace_board_path)) == 0,
          "autoplace board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(autoplace_board_path) +
              " --id J1.1 --component J1 --pin 1 --net N_EXIST --layers F.Cu --x-mm 2 --y-mm 2 "
              "--width-mm 1 --height-mm 1") == 0,
          "autoplace fixture existing pad exits zero");
  writeFile(autoplace_footprint_path,
            "{\n"
            "  \"name\": \"AutoOnePad\",\n"
            "  \"pads\": [\n"
            "    {\"number\": \"1\", \"type\": \"smd\", \"shape\": \"rect\", "
            "\"x_nm\": 0, \"y_nm\": 0, \"width_nm\": 1000000, \"height_nm\": 1000000, "
            "\"layers\": [\"F.Cu\"]}\n"
            "  ]\n"
            "}\n");
  require(run(quote(CCAD_BINARY) + " pcb autoplace-footprint --file " +
              quote(autoplace_board_path) + " --footprint " +
              quote(autoplace_footprint_path) +
              " --component U_AUTO --layer F.Cu --grid-mm 1 > " +
              quote(autoplace_result_path)) == 0,
          "pcb autoplace-footprint exits zero");
  const std::string autoplace_result = readFile(autoplace_result_path);
  require(autoplace_result.find("\"component_id\": \"U_AUTO\"") != std::string::npos,
          "pcb autoplace-footprint reports component id");
  require(autoplace_result.find("\"origin_x_nm\": 4000000") != std::string::npos,
          "pcb autoplace-footprint reports low-cost origin away from existing pad");
  const std::string autoplace_board = readFile(autoplace_board_path);
  require(autoplace_board.find("\"id\": \"U_AUTO.1\"") != std::string::npos,
          "pcb autoplace-footprint writes placed pad");
  require(run(quote(CCAD_BINARY) + " pcb autoplace-footprint --file " +
              quote(autoplace_board_path) + " --footprint " +
              quote(autoplace_footprint_path) + " --component U_BAD --layer F.SilkS") != 0,
          "pcb autoplace-footprint rejects non-copper layer");

  const std::filesystem::path spread_board_path = temp / "spread-board.ccad.json";
  const std::filesystem::path spread_result_path = temp / "spread-result.json";
  require(run(quote(CCAD_BINARY) + " init --name spread-board --width-mm 20 --height-mm 10 --out " +
              quote(spread_board_path)) == 0,
          "spread board init exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(spread_board_path) +
              " --id R2.1 --component R2 --pin 1 --net N2 --layers F.Cu --x-mm 1 --y-mm 1 "
              "--width-mm 1 --height-mm 1") == 0,
          "spread fixture first pad exits zero");
  require(run(quote(CCAD_BINARY) + " pcb add-pad --file " + quote(spread_board_path) +
              " --id R1.1 --component R1 --pin 1 --net N1 --layers F.Cu --x-mm 1 --y-mm 1 "
              "--width-mm 1 --height-mm 1") == 0,
          "spread fixture second pad exits zero");
  require(run(quote(CCAD_BINARY) + " pcb spread-footprints --file " + quote(spread_board_path) +
              " --symbols R2,R1 --target-x-mm 12 --target-y-mm 2 --component-gap-mm 1 "
              "--group-gap-mm 1.5 > " +
              quote(spread_result_path)) == 0,
          "pcb spread-footprints exits zero");
  const std::string spread_result = readFile(spread_result_path);
  require(spread_result.find("\"moved\": 2") != std::string::npos,
          "pcb spread-footprints reports moved count");
  require(spread_result.find("\"component_id\": \"R1\"") <
              spread_result.find("\"component_id\": \"R2\""),
          "pcb spread-footprints reports naturally sorted references");
  const std::string spread_board = readFile(spread_board_path);
  require(spread_board.find("\"id\": \"R1.1\"") != std::string::npos,
          "pcb spread-footprints keeps first selected pad");
  require(spread_board.find("\"x_nm\": 12500000") != std::string::npos,
          "pcb spread-footprints moves naturally first component to target lane");
  require(spread_board.find("\"x_nm\": 14500000") != std::string::npos,
          "pcb spread-footprints moves second component after component gap");

  const std::filesystem::path diff_after_path = temp / "diff-after.ccad.json";
  std::ofstream diff_after(diff_after_path);
  diff_after << "{\n"
             << "  \"schema_version\": 1,\n"
             << "  \"id\": \"proj-diff\",\n"
             << "  \"name\": \"diff\",\n"
             << "  \"symbols\": [\n"
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

  const std::filesystem::path board_diff_before_path = temp / "board-diff-before.ccad.json";
  std::ofstream board_diff_before(board_diff_before_path);
  board_diff_before << "{\n"
                    << "  \"schema_version\": 1,\n"
                    << "  \"id\": \"proj-board-diff\",\n"
                    << "  \"name\": \"board-diff\",\n"
                    << "  \"board\": {\n"
                    << "    \"outline\": {\"x_nm\": 0, \"y_nm\": 0, \"width_nm\": 42000000, \"height_nm\": 28000000},\n"
                    << "    \"design_rules\": {\"copper_clearance_nm\": 200000, \"min_track_width_nm\": 150000, \"min_via_annular_ring_nm\": 100000},\n"
                    << "    \"layers\": [\n"
                    << "      {\"id\": \"F.Cu\", \"name\": \"Front copper\", \"kind\": \"copper\", \"visible\": true}\n"
                    << "    ],\n"
                    << "    \"placement_regions\": [\n"
                    << "      {\"id\": \"PR1\", \"kind\": \"component\", \"area\": {\"x_nm\": 2000000, \"y_nm\": 3000000, \"width_nm\": 10000000, \"height_nm\": 6000000}}\n"
                    << "    ],\n"
                    << "    \"keepouts\": [\n"
                    << "      {\"id\": \"K1\", \"kind\": \"placement\", \"area\": {\"x_nm\": 20000000, \"y_nm\": 10000000, \"width_nm\": 4000000, \"height_nm\": 3000000}}\n"
                    << "    ],\n"
                    << "    \"pads\": [\n"
                    << "      {\"id\": \"P1\", \"component_id\": \"U1\", \"pin_name\": \"1\", \"net_id\": \"N1\", \"position\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, \"rotation_degrees\": 0, \"padstack\": {\"layer_set\": [\"F.Cu\"], \"copper_props\": {\"top\": {\"shape\": {\"size\": {\"width_nm\": 1500000, \"height_nm\": 1000000}}}}}}\n"
                    << "    ],\n"
                    << "    \"vias\": [\n"
                    << "      {\"id\": \"V1\", \"net_id\": \"N1\", \"position\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"diameter_nm\": 800000, \"drill_nm\": 400000}\n"
                    << "    ],\n"
                    << "    \"tracks\": [\n"
                    << "      {\"id\": \"T1\", \"net_id\": \"N1\", \"layer_id\": \"F.Cu\", \"start\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, \"end\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"width_nm\": 250000}\n"
                    << "    ]\n"
                    << "  },\n"
                    << "  \"symbols\": [],\n"
                    << "  \"constraints\": [],\n"
                    << "  \"nets\": []\n"
                    << "}\n";
  board_diff_before.close();

  const std::filesystem::path board_diff_after_path = temp / "board-diff-after.ccad.json";
  std::ofstream board_diff_after(board_diff_after_path);
  board_diff_after << "{\n"
                   << "  \"schema_version\": 1,\n"
                   << "  \"id\": \"proj-board-diff\",\n"
                   << "  \"name\": \"board-diff\",\n"
                   << "  \"board\": {\n"
                   << "    \"outline\": {\"x_nm\": 0, \"y_nm\": 0, \"width_nm\": 42000000, \"height_nm\": 28000000},\n"
                   << "    \"design_rules\": {\"copper_clearance_nm\": 250000, \"min_track_width_nm\": 150000, \"min_via_annular_ring_nm\": 100000},\n"
                   << "    \"layers\": [\n"
                   << "      {\"id\": \"F.Cu\", \"name\": \"Front copper\", \"kind\": \"copper\", \"visible\": false}\n"
                   << "    ],\n"
                   << "    \"placement_regions\": [\n"
                   << "      {\"id\": \"PR1\", \"kind\": \"module\", \"area\": {\"x_nm\": 2000000, \"y_nm\": 3000000, \"width_nm\": 10000000, \"height_nm\": 6000000}}\n"
                   << "    ],\n"
                   << "    \"keepouts\": [\n"
                   << "      {\"id\": \"K1\", \"kind\": \"placement\", \"area\": {\"x_nm\": 20000000, \"y_nm\": 10000000, \"width_nm\": 5000000, \"height_nm\": 3000000}}\n"
                   << "    ],\n"
                   << "    \"pads\": [\n"
                   << "      {\"id\": \"P1\", \"component_id\": \"U1\", \"pin_name\": \"1\", \"net_id\": \"N1\", \"position\": {\"x_nm\": 6000000, \"y_nm\": 6000000}, \"rotation_degrees\": 0, \"padstack\": {\"layer_set\": [\"F.Cu\"], \"copper_props\": {\"top\": {\"shape\": {\"size\": {\"width_nm\": 1500000, \"height_nm\": 1000000}}}}}}\n"
                   << "    ],\n"
                   << "    \"vias\": [\n"
                   << "      {\"id\": \"V1\", \"net_id\": \"N1\", \"position\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"diameter_nm\": 900000, \"drill_nm\": 400000}\n"
                   << "    ],\n"
                   << "    \"tracks\": [\n"
                   << "      {\"id\": \"T1\", \"net_id\": \"N1\", \"layer_id\": \"F.Cu\", \"start\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, \"end\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"width_nm\": 300000}\n"
                   << "    ]\n"
                   << "  },\n"
                   << "  \"symbols\": [],\n"
                   << "  \"constraints\": [],\n"
                   << "  \"nets\": []\n"
                   << "}\n";
  board_diff_after.close();

  const std::filesystem::path board_diff_output_path = temp / "board-diff.json";
  const std::string board_diff_command =
      quote(CCAD_BINARY) + " diff " + quote(board_diff_before_path) + " " +
      quote(board_diff_after_path) + " > " + quote(board_diff_output_path);
  require(run(board_diff_command) == 0, "board diff exits zero");
  const std::string board_diff_output = readFile(board_diff_output_path);
  require(board_diff_output.find("\"object_type\": \"design_rules\"") != std::string::npos,
          "board diff has design rules entry");
  require(board_diff_output.find("\"object_type\": \"layer\"") != std::string::npos,
          "board diff has layer entry");
  require(board_diff_output.find("\"object_type\": \"placement_region\"") != std::string::npos,
          "board diff has placement region entry");
  require(board_diff_output.find("\"object_type\": \"keepout\"") != std::string::npos,
          "board diff has keepout entry");
  require(board_diff_output.find("\"object_type\": \"pad\"") != std::string::npos,
          "board diff has pad entry");
  require(board_diff_output.find("\"object_type\": \"via\"") != std::string::npos,
          "board diff has via entry");
  require(board_diff_output.find("\"object_type\": \"track\"") != std::string::npos,
          "board diff has track entry");

  const std::filesystem::path escaped_path = temp / "escaped.ccad.json";
  const std::string init_escaped = quote(CCAD_BINARY) +
                                   " init --name \"demo\tname\" --out " + quote(escaped_path);
  require(run(init_escaped) == 0, "init accepts escaped shell tab value");
  require(readFile(escaped_path).find("\\t") != std::string::npos,
          "init emits valid escaped JSON string");

  const std::filesystem::path kicad_out_path = temp / "board.kicad_pcb";
  const std::string export_kicad_cmd = quote(CCAD_BINARY) + " pcb export-kicad --file " +
                                       quote(board_project_path) + " --output " + quote(kicad_out_path);
  require(run(export_kicad_cmd) == 0, "export-kicad command exits zero");
  require(std::filesystem::exists(kicad_out_path), "export-kicad command creates output file");
  const std::string kicad_out_content = readFile(kicad_out_path);
  require(kicad_out_content.find("(kicad_pcb") != std::string::npos, "exported content has kicad_pcb");

  const std::filesystem::path dsn_out_path = temp / "board.dsn";
  const std::string export_dsn_cmd = quote(CCAD_BINARY) + " pcb export-dsn --file " +
                                     quote(board_project_path) + " --output " + quote(dsn_out_path);
  require(run(export_dsn_cmd) == 0, "export-dsn command exits zero");
  require(std::filesystem::exists(dsn_out_path), "export-dsn command creates output file");
  const std::string dsn_out_content = readFile(dsn_out_path);
  require(dsn_out_content.find("(pcb ") != std::string::npos, "exported dsn has pcb root");

  const std::filesystem::path fp_export_in_path = temp / "export_fp_in.json";
  writeFile(fp_export_in_path, "{\n  \"name\": \"TestFP\",\n  \"pads\": [\n    {\n      \"number\": \"1\",\n      \"type\": \"smd\",\n      \"shape\": \"rect\",\n      \"x_nm\": 1000000,\n      \"y_nm\": 2000000,\n      \"rotation_degrees\": 90,\n      \"width_nm\": 1000000,\n      \"height_nm\": 2000000,\n      \"layers\": [\"F.Cu\"]\n    }\n  ]\n}");
  const std::filesystem::path fp_export_out_path = temp / "export_fp_out.kicad_mod";
  const std::string fp_export_cmd = quote(CCAD_BINARY) + " lib export-footprint --in " +
                                    quote(fp_export_in_path) + " --out " + quote(fp_export_out_path);
  require(run(fp_export_cmd) == 0, "export-footprint exits zero");
  require(std::filesystem::exists(fp_export_out_path), "export-footprint creates file");
  const std::string fp_export_content = readFile(fp_export_out_path);
  require(fp_export_content.find("(footprint") != std::string::npos, "export footprint has root");
}
