#pragma once

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

struct Project {
  int schema_version = 1;
  std::string id;
  std::string name;
  std::vector<Component> components;
  std::vector<Net> nets;
  std::vector<Constraint> constraints;
};

}  // namespace ccad

