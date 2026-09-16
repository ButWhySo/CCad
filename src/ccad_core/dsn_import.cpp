#include "dsn_import.hpp"
#include "sexpr_parser.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

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
        via.diameter = millimeters(0.6); // stub
        via.drill = millimeters(0.3); // stub
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
