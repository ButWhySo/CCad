#pragma once

#include "ccad_core/geometry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {

struct Pin {
  std::string name;
  std::string kind;
};

struct Component {
  std::string id;
  std::string part;
  std::vector<Pin> pins;
};

struct NetMember {
  std::string component_id;
  std::string pin_name;
};

struct Net {
  std::string id;
  std::vector<NetMember> members;
};

struct Constraint {
  std::string id;
  std::string kind;
  std::string target;
  std::string value;
};

struct Layer {
  std::string id;
  std::string name;
  std::string kind;
  bool visible = true;
};

struct Board {
  Rect outline;
  std::vector<Layer> layers;
};

struct Project {
  int schema_version = 1;
  std::string id;
  std::string name;
  std::optional<Board> board;
  std::vector<Component> components;
  std::vector<Net> nets;
  std::vector<Constraint> constraints;
};

}  // namespace ccad

