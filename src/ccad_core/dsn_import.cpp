#include "dsn_import.hpp"
#include "sexpr_parser.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace ccad {

static double parseMm(const std::string& str) {
  try {
    return std::stod(str);
  } catch (...) {
    return 0.0;
  }
}

static Length mmToLength(double mm) {
  return nanometers(static_cast<std::int64_t>(mm * 1000000.0));
}

SesRouting importSpecctraSes(std::string_view source) {
  SesRouting result;
  auto root = parseSExpr(source);
  if (!root) {
    throw std::runtime_error("failed to parse SES file");
  }

  std::unordered_map<std::string, double> via_diameters_mm;
  std::vector<const SExpr*> library_stack = {root.get()};
  while (!library_stack.empty()) {
    const SExpr* node = library_stack.back();
    library_stack.pop_back();
    if (node->is_list && node->children.size() >= 2 && node->children[0]->value == "padstack") {
      const std::string& id = node->children[1]->value;
      for (const auto& child : node->children) {
        if (child->is_list && child->children.size() >= 2 && child->children[0]->value == "shape" &&
            child->children[1]->is_list && child->children[1]->children.size() >= 3 &&
            child->children[1]->children[0]->value == "circle") {
          via_diameters_mm[id] = 2.0 * parseMm(child->children[1]->children[2]->value);
          break;
        }
      }
    }
    for (const auto& child : node->children) library_stack.push_back(child.get());
  }

  struct PendingNode {
    const SExpr* node;
    std::string net_id;
  };
  std::vector<PendingNode> stack = {{root.get(), {}}};
  while (!stack.empty()) {
    const PendingNode pending = std::move(stack.back());
    stack.pop_back();
    const SExpr* node = pending.node;
    std::string net_id = pending.net_id;

    if (node->is_list && !node->children.empty()) {
      const std::string& type = node->children[0]->value;
      if (type == "net" && node->children.size() >= 2) {
        net_id = node->children[1]->value;
      }
      
      if (type == "wire" && node->children.size() >= 2) {
        // (wire (path layer_name width x1 y1 x2 y2 ...))
        const SExpr* path = findSExprChild(node, "path");
        if (path && path->children.size() >= 6) {
          std::string layer = path->children[1]->value;
          Length width = mmToLength(parseMm(path->children[2]->value));
          
          for (size_t i = 3; i + 3 < path->children.size(); i += 2) {
            TrackSegment track;
            track.id = "ses_" + std::to_string(result.tracks.size());
            track.net_id = net_id;
            track.layer_id = layer;
            track.width = width;
            track.start.x = mmToLength(parseMm(path->children[i]->value));
            track.start.y = mmToLength(parseMm(path->children[i+1]->value));
            track.end.x = mmToLength(parseMm(path->children[i+2]->value));
            track.end.y = mmToLength(parseMm(path->children[i+3]->value));
            result.tracks.push_back(track);
          }
        }
      } else if (type == "via" && node->children.size() >= 4) {
        // (via via_name x y)
        Via via;
        via.id = "ses_via_" + std::to_string(result.vias.size());
        via.net_id = net_id;
        via.diameter = millimeters(0.6);
        if (auto it = via_diameters_mm.find(node->children[1]->value); it != via_diameters_mm.end()) {
          via.diameter = mmToLength(it->second);
        }
        via.drill = millimeters(0.3);
        via.position.x = mmToLength(parseMm(node->children[2]->value));
        via.position.y = mmToLength(parseMm(node->children[3]->value));
        result.vias.push_back(via);
      } else {
        // push children in reverse order so they are processed in order
        for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
          stack.push_back(PendingNode{it->get(), net_id});
        }
      }
    }
  }

  return result;
}

} // namespace ccad
