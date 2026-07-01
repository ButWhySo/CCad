#include "ccad_core/cross_probing.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ccad {
namespace {

std::string trim(std::string_view value) {
  std::size_t first = 0;
  while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  std::size_t last = value.size();
  while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return std::string(value.substr(first, last - first));
}

std::string quoteIpc(std::string_view value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '\\' || ch == '"') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

std::string quotedValueAfter(std::string_view text, std::string_view key) {
  const std::size_t key_pos = text.find(key);
  if (key_pos == std::string_view::npos) {
    return "";
  }
  const std::size_t first_quote = text.find('"', key_pos + key.size());
  if (first_quote == std::string_view::npos) {
    return trim(text.substr(key_pos + key.size()));
  }
  std::string out;
  bool escaped = false;
  for (std::size_t index = first_quote + 1; index < text.size(); ++index) {
    const char ch = text[index];
    if (escaped) {
      out.push_back(ch);
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      return out;
    }
    out.push_back(ch);
  }
  return "";
}

std::vector<std::string> splitCsv(std::string_view value) {
  std::vector<std::string> items;
  std::stringstream stream{std::string(value)};
  std::string item;
  while (std::getline(stream, item, ',')) {
    const std::string cleaned = trim(item);
    if (!cleaned.empty()) {
      items.push_back(cleaned);
    }
  }
  return items;
}

void addPending(CrossProbeReport& report) {
  report.pending_kicad_features = {
      "live_socket_or_kiway_mail_dispatch",
      "gui_highlight_flash_and_zoom_to_fit",
      "net_chain_highlighting",
      "sheet_path_selection_prefixes",
      "full_kicad_ipc_escape_context",
      "3d_view_highlight_refresh",
      "footprint_library_config_and_drc_dialog_remote_commands",
  };
}

void addUniqueTarget(std::vector<CrossProbeTarget>& targets, CrossProbeTarget target) {
  const auto duplicate = std::find_if(
      targets.begin(), targets.end(), [&](const CrossProbeTarget& existing) {
        return existing.type == target.type && existing.id == target.id &&
               existing.selection_index == target.selection_index;
      });
  if (duplicate == targets.end()) {
    targets.push_back(std::move(target));
  }
}

void addSchematicNetTarget(const Project& project, const std::string& net_id,
                           std::vector<CrossProbeTarget>& targets) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return;
  }
  for (const Net& net : schematic->nets) {
    if (net.id == net_id) {
      addUniqueTarget(targets, CrossProbeTarget{.type = "schematic_net",
                                                .id = net.id,
                                                .component_id = "",
                                                .pin_name = "",
                                                .net_id = net.id,
                                                .source = "schematic",
                                                .selection_index = -1,
                                                .focus = false});
      return;
    }
  }
}

void addNetTargets(const Project& project, const std::string& net_id,
                   std::vector<CrossProbeTarget>& targets) {
  addSchematicNetTarget(project, net_id, targets);
  const Board* board = primaryBoard(project);
  if (board == nullptr) {
    return;
  }
  for (const Pad& pad : board->pads) {
    if (pad.net_id == net_id) {
      addUniqueTarget(targets, CrossProbeTarget{.type = "pad",
                                                .id = pad.id,
                                                .component_id = pad.component_id,
                                                .pin_name = pad.pin_name,
                                                .net_id = pad.net_id,
                                                .source = "board",
                                                .selection_index = -1,
                                                .focus = false});
    }
  }
  for (const Via& via : board->vias) {
    if (via.net_id == net_id) {
      addUniqueTarget(targets, CrossProbeTarget{.type = "via",
                                                .id = via.id,
                                                .component_id = "",
                                                .pin_name = "",
                                                .net_id = via.net_id,
                                                .source = "board",
                                                .selection_index = -1,
                                                .focus = false});
    }
  }
  for (const TrackSegment& track : board->tracks) {
    if (track.net_id == net_id) {
      addUniqueTarget(targets, CrossProbeTarget{.type = "track",
                                                .id = track.id,
                                                .component_id = "",
                                                .pin_name = "",
                                                .net_id = track.net_id,
                                                .source = "board",
                                                .selection_index = -1,
                                                .focus = false});
    }
  }
  for (const BoardZone& zone : board->zones) {
    if (zone.net_id == net_id) {
      addUniqueTarget(targets, CrossProbeTarget{.type = "zone",
                                                .id = zone.id,
                                                .component_id = "",
                                                .pin_name = "",
                                                .net_id = zone.net_id,
                                                .source = "board",
                                                .selection_index = -1,
                                                .focus = false});
    }
  }
}

void addPartTargets(const Project& project, const std::string& reference,
                    std::vector<CrossProbeTarget>& targets, int selection_index = -1,
                    bool focus = false) {
  const Board* board = primaryBoard(project);
  if (board != nullptr) {
    for (const BoardFootprint& footprint : board->footprints) {
      if (footprint.reference == reference) {
        addUniqueTarget(targets,
                        CrossProbeTarget{.type = "footprint",
                                         .id = footprint.reference,
                                         .component_id = footprint.reference,
                                         .pin_name = "",
                                         .net_id = "",
                                         .source = "board",
                                         .selection_index = selection_index,
                                         .focus = focus});
      }
    }
  }
  const Schematic* schematic = primarySchematic(project);
  if (schematic != nullptr) {
    for (const SchSymbol& component : schematic->symbols) {
      if (component.id == reference) {
        addUniqueTarget(targets,
                        CrossProbeTarget{.type = "component",
                                         .id = component.id,
                                         .component_id = component.id,
                                         .pin_name = "",
                                         .net_id = "",
                                         .source = "schematic",
                                         .selection_index = selection_index,
                                         .focus = focus});
      }
    }
  }
}

void addPadTargets(const Project& project, const std::string& reference,
                   const std::string& pad_number, std::vector<CrossProbeTarget>& targets,
                   int selection_index = -1, bool focus = false) {
  const Board* board = primaryBoard(project);
  if (board == nullptr) {
    return;
  }
  for (const Pad& pad : board->pads) {
    if (pad.component_id == reference && pad.pin_name == pad_number) {
      addUniqueTarget(targets,
                      CrossProbeTarget{.type = "pad",
                                       .id = pad.id,
                                       .component_id = pad.component_id,
                                      .pin_name = pad.pin_name,
                                      .net_id = pad.net_id,
                                      .source = "board",
                                      .selection_index = selection_index,
                                       .focus = focus});
    }
  }
}

void resolveSelectionEntry(const Project& project, const std::string& entry, int index, bool focus,
                           CrossProbeReport& report) {
  if (entry.empty()) {
    return;
  }
  const char type = entry.front();
  const std::string payload = entry.substr(1);
  if (type == 'F') {
    addPartTargets(project, payload, report.targets, index, focus);
    return;
  }
  if (type == 'P') {
    const std::size_t slash = payload.find('/');
    if (slash == std::string::npos) {
      report.diagnostics.push_back("malformed_pad_selection_entry:" + entry);
      return;
    }
    addPadTargets(project, payload.substr(0, slash), payload.substr(slash + 1), report.targets,
                  index, focus);
    return;
  }
  if (type == 'S') {
    report.diagnostics.push_back("sheet_selection_not_supported:" + payload);
    return;
  }
  report.diagnostics.push_back("unsupported_selection_entry:" + entry);
}

}  // namespace

std::string formatCrossProbeClear() {
  return "$CLEAR: \"HIGHLIGHTED\"";
}

std::string formatCrossProbeNet(std::string_view net_name) {
  return "$NET: \"" + quoteIpc(net_name) + "\"";
}

std::string formatCrossProbePart(std::string_view reference) {
  return "$PART: \"" + quoteIpc(reference) + "\"";
}

std::string formatCrossProbePad(std::string_view reference, std::string_view pad_number) {
  return "$PART: \"" + quoteIpc(reference) + "\" $PAD: \"" + quoteIpc(pad_number) + "\"";
}

CrossProbeReport resolveCrossProbePacket(const Project& project, std::string_view packet) {
  CrossProbeReport report;
  report.packet = trim(packet);
  addPending(report);

  if (report.packet.rfind("$CLEAR", 0) == 0) {
    report.packet_kind = "clear";
    report.clear_highlight = true;
    return report;
  }

  if (report.packet.rfind("$NETS:", 0) == 0) {
    report.packet_kind = "nets";
    const std::string quoted = quotedValueAfter(report.packet, "$NETS:");
    report.requested_nets = splitCsv(quoted);
    for (const std::string& net : report.requested_nets) {
      addNetTargets(project, net, report.targets);
    }
    if (report.targets.empty()) {
      report.diagnostics.push_back("no_targets_for_nets");
    }
    return report;
  }

  if (report.packet.rfind("$NET:", 0) == 0) {
    report.packet_kind = "net";
    const std::string net = quotedValueAfter(report.packet, "$NET:");
    if (!net.empty()) {
      report.requested_nets.push_back(net);
      addNetTargets(project, net, report.targets);
    }
    if (report.targets.empty()) {
      report.diagnostics.push_back("no_targets_for_net:" + net);
    }
    return report;
  }

  if (report.packet.rfind("$PART:", 0) == 0) {
    report.packet_kind = "part";
    report.part_reference = quotedValueAfter(report.packet, "$PART:");
    report.pad_number = quotedValueAfter(report.packet, "$PAD:");
    if (!report.part_reference.empty() && !report.pad_number.empty()) {
      report.packet_kind = "pad";
      addPadTargets(project, report.part_reference, report.pad_number, report.targets);
    } else if (!report.part_reference.empty()) {
      addPartTargets(project, report.part_reference, report.targets);
    }
    if (report.targets.empty()) {
      report.diagnostics.push_back("no_targets_for_part:" + report.part_reference);
    }
    return report;
  }

  if (report.packet.rfind("$SELECT:", 0) == 0) {
    report.packet_kind = "select";
    const std::string payload = trim(std::string_view(report.packet).substr(8));
    const std::vector<std::string> items = splitCsv(payload);
    if (items.empty()) {
      report.diagnostics.push_back("empty_select_packet");
      return report;
    }
    report.select_connections = items.front() == "1";
    if (items.front() != "0" && items.front() != "1") {
      report.diagnostics.push_back("invalid_select_mode:" + items.front());
    }
    for (std::size_t index = 1; index < items.size(); ++index) {
      resolveSelectionEntry(project, items.at(index), static_cast<int>(index - 1),
                            report.select_connections && index == 1, report);
    }
    if (report.targets.empty()) {
      report.diagnostics.push_back("no_targets_for_selection");
    }
    return report;
  }

  report.diagnostics.push_back("unsupported_cross_probe_packet");
  return report;
}

}  // namespace ccad
