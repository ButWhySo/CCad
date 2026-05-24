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
  require(help_json.find("\"name\": \"pcb add-layer\"") != std::string::npos,
          "help json describes layer authoring");
  require(help_json.find("\"name\": \"pcb set-layer\"") != std::string::npos,
          "help json describes layer metadata editing");
  require(help_json.find("\"name\": \"pcb get-object\"") != std::string::npos,
          "help json describes pcb object lookup");
  require(help_json.find("\"name\": \"pcb list-objects\"") != std::string::npos,
          "help json describes pcb object listing");
  require(help_json.find("\"name\": \"pcb list-nets\"") != std::string::npos,
          "help json describes pcb net listing");
  require(help_json.find("\"name\": \"pcb list-route-requests\"") != std::string::npos,
          "help json describes route request listing");
  require(help_json.find("\"name\": \"pcb remove-layer\"") != std::string::npos,
          "help json describes layer removal");
  require(help_json.find("\"name\": \"pcb set-layer-visibility\"") != std::string::npos,
          "help json describes layer visibility authoring");
  require(help_json.find("\"name\": \"pcb set-rules\"") != std::string::npos,
          "help json describes drc rule authoring");
  require(help_json.find("\"name\": \"pcb set-outline\"") != std::string::npos,
          "help json describes outline authoring");
  require(help_json.find("\"name\": \"pcb set-track\"") != std::string::npos,
          "help json describes track editing");
  require(help_json.find("\"name\": \"pcb add-route-request\"") != std::string::npos,
          "help json describes route request authoring");
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

  const std::filesystem::path board_project_path = temp / "board.ccad.json";
  const std::string board_init_command = quote(CCAD_BINARY) +
                                         " init --name board --width-mm 42 --height-mm 28 --out " +
                                         quote(board_project_path);
  require(run(board_init_command) == 0, "board init exits zero");
  const std::string board_json = readFile(board_project_path);
  require(board_json.find("\"board\"") != std::string::npos, "board init writes board");
  require(board_json.find("\"width_nm\": 42000000") != std::string::npos,
          "board init writes width");

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
      " --id LP1 --component U1 --pin 1 --net N1 --layer In1.Cu"
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
      " --min-via-annular-ring-mm 0.08";
  require(run(set_rules_command) == 0, "pcb set-rules exits zero");
  const std::string rules_json = readFile(board_project_path);
  require(rules_json.find("\"design_rules\"") != std::string::npos,
          "pcb set-rules writes design rules");
  require(rules_json.find("\"copper_clearance_nm\": 150000") != std::string::npos,
          "pcb set-rules writes copper clearance");
  require(rules_json.find("\"min_track_width_nm\": 120000") != std::string::npos,
          "pcb set-rules writes minimum track width");
  require(rules_json.find("\"min_via_annular_ring_nm\": 80000") != std::string::npos,
          "pcb set-rules writes minimum via annular ring");
  const std::filesystem::path inspect_rules_path = temp / "inspect-rules.json";
  const std::string inspect_rules_command =
      quote(CCAD_BINARY) + " inspect " + quote(board_project_path) + " > " +
      quote(inspect_rules_path);
  require(run(inspect_rules_command) == 0, "inspect after set-rules exits zero");
  const std::string inspect_rules_json = readFile(inspect_rules_path);
  require(inspect_rules_json.find("\"design_rules\"") != std::string::npos,
          "inspect writes design rules");
  require(inspect_rules_json.find("\"copper_clearance_nm\": 150000") != std::string::npos,
          "inspect writes copper clearance rule");
  require(inspect_rules_json.find("\"min_track_width_nm\": 120000") != std::string::npos,
          "inspect writes minimum track width rule");
  require(inspect_rules_json.find("\"min_via_annular_ring_nm\": 80000") != std::string::npos,
          "inspect writes minimum via annular ring rule");

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
  require(list_objects_json.find("\"total\": 9") != std::string::npos,
          "pcb list-objects reports total");
  require(list_objects_json.find("\"type\": \"layer\"") != std::string::npos,
          "pcb list-objects includes layers");
  require(list_objects_json.find("\"id\": \"P1\"") != std::string::npos,
          "pcb list-objects includes pad id");
  require(list_objects_json.find("\"net_id\": \"N1\"") != std::string::npos,
          "pcb list-objects includes net metadata");

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
  require(list_route_requests_json.find("\"from_object_id\": \"P1\"") != std::string::npos,
          "pcb list-route-requests includes source object");
  require(list_route_requests_json.find("\"to_object_id\": \"V1\"") != std::string::npos,
          "pcb list-route-requests includes target object");

  require(run(add_pad_command) != 0, "pcb add-pad rejects duplicate id");
  require(run(add_keepout_command) != 0, "pcb add-keepout rejects duplicate id");
  require(run(add_placement_region_command) != 0,
          "pcb add-placement-region rejects duplicate id");

  const std::filesystem::path remove_board_path = temp / "remove-board.ccad.json";
  const std::string remove_board_init_command =
      quote(CCAD_BINARY) +
      " init --name remove-board --width-mm 42 --height-mm 28 --out " +
      quote(remove_board_path);
  require(run(remove_board_init_command) == 0, "remove board init exits zero");
  const std::string remove_add_pad_command =
      quote(CCAD_BINARY) + " pcb add-pad --file " + quote(remove_board_path) +
      " --id RP1 --component U1 --pin 1 --net N1 --layer F.Cu"
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
  const std::string remove_add_region_command =
      quote(CCAD_BINARY) + " pcb add-placement-region --file " + quote(remove_board_path) +
      " --id RPR1 --kind component --x-mm 2 --y-mm 3 --width-mm 10 --height-mm 6";
  require(run(remove_add_pad_command) == 0, "remove fixture add pad exits zero");
  require(run(remove_add_via_command) == 0, "remove fixture add via exits zero");
  require(run(remove_add_track_command) == 0, "remove fixture add track exits zero");
  require(run(remove_add_keepout_command) == 0, "remove fixture add keepout exits zero");
  require(run(remove_add_region_command) == 0, "remove fixture add placement region exits zero");

  const std::string remove_pad_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RP1";
  const std::string remove_via_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RV1";
  const std::string remove_track_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RT1";
  const std::string remove_keepout_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RK1";
  const std::string remove_region_command =
      quote(CCAD_BINARY) + " pcb remove-object --file " + quote(remove_board_path) +
      " --id RPR1";
  require(run(remove_pad_command) == 0, "pcb remove-object removes pad");
  require(run(remove_via_command) == 0, "pcb remove-object removes via");
  require(run(remove_track_command) == 0, "pcb remove-object removes track");
  require(run(remove_keepout_command) == 0, "pcb remove-object removes keepout");
  require(run(remove_region_command) == 0, "pcb remove-object removes placement region");
  const std::string removed_objects_json = readFile(remove_board_path);
  require(removed_objects_json.find("\"id\": \"RP1\"") == std::string::npos,
          "pcb remove-object deletes pad id");
  require(removed_objects_json.find("\"id\": \"RV1\"") == std::string::npos,
          "pcb remove-object deletes via id");
  require(removed_objects_json.find("\"id\": \"RT1\"") == std::string::npos,
          "pcb remove-object deletes track id");
  require(removed_objects_json.find("\"id\": \"RK1\"") == std::string::npos,
          "pcb remove-object deletes keepout id");
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
      " --id MP1 --component U1 --pin 1 --net N1 --layer F.Cu"
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
              " --id SP1 --component U1 --pin 1 --net N1 --layer F.Cu"
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
              " --id SPD1 --component U1 --pin 1 --net N1 --layer F.Cu"
              " --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0") == 0,
          "set pad fixture add pad exits zero");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layer B.Cu"
              " --rotation-deg 90") == 0,
          "pcb set-pad updates pad metadata");
  const std::string set_pad_json = readFile(set_pad_board_path);
  require(set_pad_json.find("\"component_id\": \"U2\"") != std::string::npos,
          "pcb set-pad writes component");
  require(set_pad_json.find("\"pin_name\": \"2\"") != std::string::npos,
          "pcb set-pad writes pin");
  require(set_pad_json.find("\"net_id\": \"N2\"") != std::string::npos,
          "pcb set-pad writes net");
  require(set_pad_json.find("\"layer_id\": \"B.Cu\"") != std::string::npos,
          "pcb set-pad writes layer");
  require(set_pad_json.find("\"rotation_degrees\": 90") != std::string::npos,
          "pcb set-pad writes rotation");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layer F.SilkS"
              " --rotation-deg 90") != 0,
          "pcb set-pad rejects non-copper layer");
  require(run(quote(CCAD_BINARY) + " pcb move-object --file " + quote(set_pad_board_path) +
              " --id SPD1 --x-mm 0.6 --y-mm 6") == 0,
          "set pad fixture moves pad near edge");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id SPD1 --component U2 --pin 2 --net N2 --layer F.Cu"
              " --rotation-deg 0") != 0,
          "pcb set-pad rejects rotated pad outside board");
  require(run(quote(CCAD_BINARY) + " pcb set-pad --file " + quote(set_pad_board_path) +
              " --id MISSING --component U2 --pin 2 --net N2 --layer F.Cu"
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
      " --id P_SILK --component U1 --pin 1 --net N1 --layer F.SilkS"
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
      " --id P_EDGE --component U1 --pin 1 --net N1 --layer F.Cu"
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
  require(board_inspect_output.find("\"placement_regions\": 1") != std::string::npos,
          "inspect reports placement region count");
  require(board_inspect_output.find("\"keepouts\": 1") != std::string::npos,
          "inspect reports keepout count");

  std::string board_with_net = readFile(board_project_path);
  const std::string empty_components = "  \"components\": [\n  ]";
  const std::string logical_u1 =
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
  const std::size_t components_position = board_with_net.find(empty_components);
  require(components_position != std::string::npos,
          "board fixture has empty components before clean drc");
  board_with_net.replace(components_position, empty_components.size(), logical_u1);
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
                    << "      {\"id\": \"P1\", \"component_id\": \"U1\", \"pin_name\": \"1\", \"net_id\": \"N1\", \"layer_id\": \"F.Cu\", \"position\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, \"rotation_degrees\": 0, \"size\": {\"width_nm\": 1500000, \"height_nm\": 1000000}}\n"
                    << "    ],\n"
                    << "    \"vias\": [\n"
                    << "      {\"id\": \"V1\", \"net_id\": \"N1\", \"position\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"diameter_nm\": 800000, \"drill_nm\": 400000}\n"
                    << "    ],\n"
                    << "    \"tracks\": [\n"
                    << "      {\"id\": \"T1\", \"net_id\": \"N1\", \"layer_id\": \"F.Cu\", \"start\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, \"end\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"width_nm\": 250000}\n"
                    << "    ]\n"
                    << "  },\n"
                    << "  \"components\": [],\n"
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
                   << "      {\"id\": \"P1\", \"component_id\": \"U1\", \"pin_name\": \"1\", \"net_id\": \"N1\", \"layer_id\": \"F.Cu\", \"position\": {\"x_nm\": 6000000, \"y_nm\": 6000000}, \"rotation_degrees\": 0, \"size\": {\"width_nm\": 1500000, \"height_nm\": 1000000}}\n"
                   << "    ],\n"
                   << "    \"vias\": [\n"
                   << "      {\"id\": \"V1\", \"net_id\": \"N1\", \"position\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"diameter_nm\": 900000, \"drill_nm\": 400000}\n"
                   << "    ],\n"
                   << "    \"tracks\": [\n"
                   << "      {\"id\": \"T1\", \"net_id\": \"N1\", \"layer_id\": \"F.Cu\", \"start\": {\"x_nm\": 5000000, \"y_nm\": 6000000}, \"end\": {\"x_nm\": 8000000, \"y_nm\": 9000000}, \"width_nm\": 300000}\n"
                   << "    ]\n"
                   << "  },\n"
                   << "  \"components\": [],\n"
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
}
