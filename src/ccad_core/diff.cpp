#include "ccad_core/diff.hpp"

#include <map>
#include <string>

namespace ccad {
namespace {

std::string pinSignature(const Pin& pin) {
  return pin.name + "\x1f" + pin.kind;
}

std::string componentSignature(const Component& component) {
  std::string signature = component.part;
  for (const Pin& pin : component.pins) {
    signature += "\x1e" + pinSignature(pin);
  }
  return signature;
}

std::string memberSignature(const NetMember& member) {
  return member.component_id + "\x1f" + member.pin_name;
}

std::string netSignature(const Net& net) {
  std::string signature;
  for (const NetMember& member : net.members) {
    signature += "\x1e" + memberSignature(member);
  }
  return signature;
}

std::string constraintSignature(const Constraint& constraint) {
  return constraint.kind + "\x1f" + constraint.target + "\x1f" + constraint.value;
}

template <typename T, typename SignatureFn>
void diffObjectMap(ProjectDiff& diff, const std::string& object_type, const std::vector<T>& before,
                   const std::vector<T>& after, SignatureFn signature_fn) {
  std::map<std::string, std::string> before_by_id;
  std::map<std::string, std::string> after_by_id;

  for (const T& item : before) {
    before_by_id[item.id] = signature_fn(item);
  }
  for (const T& item : after) {
    after_by_id[item.id] = signature_fn(item);
  }

  for (const auto& [id, signature] : after_by_id) {
    const auto before_it = before_by_id.find(id);
    if (before_it == before_by_id.end()) {
      ++diff.added_count;
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = object_type,
          .object_id = id,
          .message = object_type + " added",
      });
    } else if (before_it->second != signature) {
      ++diff.changed_count;
      diff.entries.push_back(DiffEntry{
          .change = "changed",
          .object_type = object_type,
          .object_id = id,
          .message = object_type + " changed",
      });
    }
  }

  for (const auto& [id, signature] : before_by_id) {
    (void)signature;
    if (!after_by_id.contains(id)) {
      ++diff.removed_count;
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = object_type,
          .object_id = id,
          .message = object_type + " removed",
      });
    }
  }
}

}  // namespace

ProjectDiff diffProjects(const Project& before, const Project& after) {
  ProjectDiff diff;
  diffObjectMap(diff, "component", before.components, after.components, componentSignature);
  diffObjectMap(diff, "net", before.nets, after.nets, netSignature);
  diffObjectMap(diff, "constraint", before.constraints, after.constraints, constraintSignature);
  return diff;
}

}  // namespace ccad

