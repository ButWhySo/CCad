#include "ccad_cli/common.hpp"

#include "ccad_core/json.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/serialize.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <optional>

#include "ccad_core/transaction.hpp"

namespace ccad_cli {

static std::optional<ccad::Project> g_last_loaded_project;
static std::string g_current_command;

void setAuditCommand(const std::string& command) {
  g_current_command = command;
}

bool hasError(const std::vector<ccad::Diagnostic>& diagnostics) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == "error") {
      return true;
    }
  }
  return false;
}

std::string diagnosticsJson(const std::vector<ccad::Diagnostic>& diagnostics) {
  std::size_t error_count = 0;
  std::size_t warning_count = 0;
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == "error") {
      ++error_count;
    } else if (diagnostic.severity == "warning") {
      ++warning_count;
    }
  }

  std::ostringstream out;
  out << "{\n";
  out << "  \"summary\": {\n";
  out << "    \"total\": " << diagnostics.size() << ",\n";
  out << "    \"errors\": " << error_count << ",\n";
  out << "    \"warnings\": " << warning_count << "\n";
  out << "  },\n";
  out << "  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < diagnostics.size(); ++i) {
    const ccad::Diagnostic& diagnostic = diagnostics.at(i);
    out << "    {\n"
        << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n"
        << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n"
        << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\",\n"
        << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\"\n"
        << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

std::string reviewJson(const ccad::ProjectReview& review) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"project\": {\n";
  out << "    \"id\": \"" << ccad::escapeJson(review.project_id) << "\",\n";
  out << "    \"name\": \"" << ccad::escapeJson(review.project_name) << "\"\n";
  out << "  },\n";
  out << "  \"counts\": {\n";
  out << "    \"components\": " << review.component_count << ",\n";
  out << "    \"constraints\": " << review.constraint_count << ",\n";
  out << "    \"layers\": " << review.layer_count << ",\n";
  out << "    \"nets\": " << review.net_count << "\n";
  out << "  },\n";
  out << "  \"board\": {\n";
  out << "    \"has_board\": " << (review.has_board ? "true" : "false") << ",\n";
  out << "    \"x_nm\": " << review.board_origin_x_nm << ",\n";
  out << "    \"y_nm\": " << review.board_origin_y_nm << ",\n";
  out << "    \"width_nm\": " << review.board_width_nm << ",\n";
  out << "    \"height_nm\": " << review.board_height_nm << ",\n";
  out << "    \"design_rules\": {\n";
  out << "      \"copper_clearance_nm\": " << review.copper_clearance_nm << ",\n";
  out << "      \"min_track_width_nm\": " << review.min_track_width_nm << ",\n";
  out << "      \"min_via_annular_ring_nm\": " << review.min_via_annular_ring_nm << ",\n";
  out << "      \"min_via_diameter_nm\": " << review.min_via_diameter_nm << ",\n";
  out << "      \"board_thickness_nm\": " << review.board_thickness_nm << "\n";
  out << "    },\n";
  out << "    \"layer_summary\": {\n";
  out << "      \"copper\": " << review.copper_layer_count << ",\n";
  out << "      \"non_copper\": " << review.non_copper_layer_count << ",\n";
  out << "      \"visible\": " << review.visible_layer_count << ",\n";
  out << "      \"hidden\": " << review.hidden_layer_count << "\n";
  out << "    },\n";
  out << "    \"pads\": " << review.pad_count << ",\n";
  out << "    \"vias\": " << review.via_count << ",\n";
  out << "    \"tracks\": " << review.track_count << ",\n";
  out << "    \"route_requests\": " << review.route_request_count << ",\n";
  out << "    \"route_progress\": {\n";
  out << "      \"open\": " << review.open_route_count << ",\n";
  out << "      \"partial\": " << review.partial_route_count << ",\n";
  out << "      \"completed\": " << review.completed_route_count << ",\n";
  out << "      \"routed_segments\": " << review.routed_segment_count << "\n";
  out << "    },\n";
  out << "    \"placement_regions\": " << review.placement_region_count << ",\n";
  out << "    \"keepouts\": " << review.keepout_count << "\n";
  out << "  },\n";
  out << "  \"status\": \"" << ccad::escapeJson(review.status) << "\",\n";
  out << "  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < review.diagnostics.size(); ++i) {
    const ccad::Diagnostic& diagnostic = review.diagnostics.at(i);
    out << "    {\n"
        << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n"
        << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n"
        << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\",\n"
        << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\"\n"
        << "    }" << (i + 1 == review.diagnostics.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::optional<double> parsePositiveDouble(const std::string& value) {
  try {
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size() || number <= 0.0) {
      return std::nullopt;
    }
    return number;
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

std::map<std::string, std::string> parseOptions(const std::vector<std::string>& args,
                                                const std::size_t start,
                                                const std::vector<std::string>& allowed) {
  std::map<std::string, std::string> options;
  for (std::size_t i = start; i < args.size(); i += 2) {
    if (i + 1 >= args.size()) {
      throw std::runtime_error("missing value for option: " + args.at(i));
    }
    const std::string& key = args.at(i);
    bool known = false;
    for (const std::string& allowed_key : allowed) {
      if (key == allowed_key) {
        known = true;
        break;
      }
    }
    if (!known) {
      throw std::runtime_error("unknown option: " + key);
    }
    if (options.contains(key)) {
      throw std::runtime_error("duplicate option: " + key);
    }
    options.emplace(key, args.at(i + 1));
  }
  return options;
}

std::string requireOption(const std::map<std::string, std::string>& options,
                          const std::string& key) {
  const auto found = options.find(key);
  if (found == options.end() || found->second.empty()) {
    throw std::runtime_error("missing required option: " + key);
  }
  return found->second;
}

ccad::Length requirePositiveMillimeters(const std::map<std::string, std::string>& options,
                                        const std::string& key) {
  const std::string value = requireOption(options, key);
  const std::optional<double> parsed = parsePositiveDouble(value);
  if (!parsed.has_value()) {
    throw std::runtime_error(key + " must be a positive number");
  }
  return ccad::millimeters(*parsed);
}

double requireDoubleOption(const std::map<std::string, std::string>& options,
                           const std::string& key) {
  const std::string value = requireOption(options, key);
  try {
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size()) {
      throw std::runtime_error(key + " must be a number");
    }
    return number;
  } catch (const std::exception&) {
    throw std::runtime_error(key + " must be a number");
  }
}

double optionDoubleOrDefault(const std::map<std::string, std::string>& options,
                             const std::string& key, const double default_value) {
  if (!options.contains(key)) {
    return default_value;
  }
  return requireDoubleOption(options, key);
}

ccad::Project loadProjectFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open project file: " + path);
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  ccad::Project project = ccad::loadProjectJson(buffer.str());
  g_last_loaded_project = project;
  return project;
}

bool writeProjectFile(const std::string& path, const ccad::Project& project) {
  const std::string project_json = ccad::dumpProjectJson(project);
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << project_json;
  
  if (g_last_loaded_project.has_value()) {
    std::string audit_path = path + ".audit.jsonl";
    std::ofstream audit(audit_path, std::ios::app);
    if (audit) {
      auto now = std::chrono::system_clock::now().time_since_epoch();
      long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
      std::string tx_id = "txn-" + std::to_string(ms);
      
      ccad::Transaction tx = ccad::buildTransaction(
          tx_id, 
          g_current_command.empty() ? "ccad_cli_mutation" : g_current_command, 
          "CLI mutation", 
          *g_last_loaded_project, 
          project
      );
      
      std::string tx_json = ccad::dumpTransactionJson(tx);
      std::string jsonl;
      for (char c : tx_json) {
        if (c != '\n' && c != '\r') {
          jsonl += c;
        }
      }
      audit << jsonl << "\n";
    }
    g_last_loaded_project = std::nullopt;
  }
  return static_cast<bool>(output);
}

ccad::Footprint loadFootprintFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open footprint file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return ccad::loadFootprintJson(buffer.str());
}

ccad::Board& requireBoard(ccad::Project& project) {
  if (!!project.boards.empty()) {
    throw std::runtime_error("project has no board");
  }
  return project.boards[0];
}

void requireLayer(const ccad::Board& board, const std::string& layer_id) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return;
    }
  }
  throw std::runtime_error("unknown layer: " + layer_id);
}

void requireCopperLayer(const ccad::Board& board, const std::string& layer_id) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      if (layer.kind != "copper") {
        throw std::runtime_error("layer is not copper: " + layer_id);
      }
      return;
    }
  }
  throw std::runtime_error("unknown layer: " + layer_id);
}

void requireUniqueLayerId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.id == id) {
      throw std::runtime_error("duplicate layer id: " + id);
    }
  }
}

void requireInsideBoard(const ccad::Board& board, const ccad::Point point,
                        const std::string& label) {
  const ccad::Point min = board.outline.origin;
  const ccad::Point max = ccad::maxPoint(board.outline);
  if (point.x.nanometers < min.x.nanometers || point.x.nanometers > max.x.nanometers ||
      point.y.nanometers < min.y.nanometers || point.y.nanometers > max.y.nanometers) {
    throw std::runtime_error(label + " is outside board outline");
  }
}

void requirePointWithMarginInsideBoard(const ccad::Board& board, const ccad::Point point,
                                       const ccad::Length margin,
                                       const std::string& label) {
  const ccad::Point min = board.outline.origin;
  const ccad::Point max = ccad::maxPoint(board.outline);
  if (point.x.nanometers - margin.nanometers < min.x.nanometers ||
      point.x.nanometers + margin.nanometers > max.x.nanometers ||
      point.y.nanometers - margin.nanometers < min.y.nanometers ||
      point.y.nanometers + margin.nanometers > max.y.nanometers) {
    throw std::runtime_error(label + " geometry is outside board outline");
  }
}

void requireCenteredRectInsideBoard(const ccad::Board& board, const ccad::Point center,
                                    const ccad::Size size, const std::string& label) {
  const ccad::Length half_width = ccad::nanometers(size.width.nanometers / 2);
  const ccad::Length half_height = ccad::nanometers(size.height.nanometers / 2);
  const ccad::Point min{.x = ccad::nanometers(center.x.nanometers - half_width.nanometers),
                        .y = ccad::nanometers(center.y.nanometers - half_height.nanometers)};
  const ccad::Point max{.x = ccad::nanometers(center.x.nanometers + half_width.nanometers),
                        .y = ccad::nanometers(center.y.nanometers + half_height.nanometers)};
  requireInsideBoard(board, min, label + " min corner");
  requireInsideBoard(board, max, label + " max corner");
}

void requireRotatedRectInsideBoard(const ccad::Board& board, const ccad::Point center,
                                   const ccad::Size size, const double rotation_degrees,
                                   const std::string& label) {
  constexpr double pi = 3.14159265358979323846;
  const double radians = rotation_degrees * pi / 180.0;
  const double cos_theta = std::cos(radians);
  const double sin_theta = std::sin(radians);
  const double half_width = static_cast<double>(size.width.nanometers) / 2.0;
  const double half_height = static_cast<double>(size.height.nanometers) / 2.0;
  const double center_x = static_cast<double>(center.x.nanometers);
  const double center_y = static_cast<double>(center.y.nanometers);

  for (const auto& local : {std::pair<double, double>{-half_width, -half_height},
                            std::pair<double, double>{half_width, -half_height},
                            std::pair<double, double>{half_width, half_height},
                            std::pair<double, double>{-half_width, half_height}}) {
    const ccad::Point corner{
        .x = ccad::nanometers(static_cast<std::int64_t>(
            std::llround(center_x + (local.first * cos_theta) - (local.second * sin_theta)))),
        .y = ccad::nanometers(static_cast<std::int64_t>(
            std::llround(center_y + (local.first * sin_theta) + (local.second * cos_theta))))};
    requireInsideBoard(board, corner, label + " corner");
  }
}

void requireRectInsideBoard(const ccad::Board& board, const ccad::Rect& rect,
                            const std::string& label) {
  requireInsideBoard(board, rect.origin, label + " origin");
  requireInsideBoard(board, ccad::maxPoint(rect), label + " max corner");
}

void requireUniquePadId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Pad& pad : board.pads) {
    if (pad.id == id) {
      throw std::runtime_error("duplicate pad id: " + id);
    }
  }
}

void requireUniqueViaId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Via& via : board.vias) {
    if (via.id == id) {
      throw std::runtime_error("duplicate via id: " + id);
    }
  }
}

void requireUniqueTrackId(const ccad::Board& board, const std::string& id) {
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.id == id) {
      throw std::runtime_error("duplicate track id: " + id);
    }
  }
}

void requireUniqueKeepoutId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Keepout& keepout : board.keepouts) {
    if (keepout.id == id) {
      throw std::runtime_error("duplicate keepout id: " + id);
    }
  }
}

void requireUniquePlacementRegionId(const ccad::Board& board, const std::string& id) {
  for (const ccad::PlacementRegion& region : board.placement_regions) {
    if (region.id == id) {
      throw std::runtime_error("duplicate placement region id: " + id);
    }
  }
}

void requireUniquePhysicalObjectId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Pad& pad : board.pads) {
    if (pad.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::Via& via : board.vias) {
    if (via.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::BoardGraphic& graphic : board.graphics) {
    if (graphic.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::BoardText& text : board.texts) {
    if (text.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::BoardZone& zone : board.zones) {
    if (zone.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::Keepout& keepout : board.keepouts) {
    if (keepout.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
  for (const ccad::PlacementRegion& region : board.placement_regions) {
    if (region.id == id) {
      throw std::runtime_error("duplicate physical object id: " + id);
    }
  }
}

ccad::Point rotateAndTranslate(const ccad::Point& local, const ccad::Point& origin,
                               const double rotation_degrees) {
  constexpr double pi = 3.14159265358979323846;
  const double radians = rotation_degrees * pi / 180.0;
  const double cos_theta = std::cos(radians);
  const double sin_theta = std::sin(radians);
  const double local_x = static_cast<double>(local.x.nanometers);
  const double local_y = static_cast<double>(local.y.nanometers);
  return ccad::Point{
      .x = ccad::nanometers(origin.x.nanometers +
                            static_cast<std::int64_t>(std::llround((local_x * cos_theta) -
                                                                   (local_y * sin_theta)))),
      .y = ccad::nanometers(origin.y.nanometers +
                            static_cast<std::int64_t>(std::llround((local_x * sin_theta) +
                                                                   (local_y * cos_theta)))),
  };
}

}  // namespace ccad_cli
