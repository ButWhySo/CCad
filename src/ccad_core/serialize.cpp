#include "ccad_core/serialize.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace ccad {
namespace {

std::string escapeJson(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

void writeField(std::ostringstream& out, const int indent, const std::string& key,
                const std::string& value, const bool comma = true) {
  out << std::string(indent, ' ') << '"' << key << "\": \"" << escapeJson(value) << '"';
  if (comma) {
    out << ',';
  }
  out << '\n';
}

class JsonReader {
 public:
  explicit JsonReader(std::string_view source) : source_(source) {}

  Project readProject() {
    Project project;
    expect('{');
    while (!consume('}')) {
      const std::string key = readString();
      expect(':');
      if (key == "schema_version") {
        project.schema_version = readInt();
      } else if (key == "id") {
        project.id = readString();
      } else if (key == "name") {
        project.name = readString();
      } else if (key == "components") {
        project.components = readComponents();
      } else if (key == "nets") {
        project.nets = readNets();
      } else if (key == "constraints") {
        project.constraints = readConstraints();
      } else {
        throw std::runtime_error("unknown project key: " + key);
      }
      consume(',');
    }
    return project;
  }

 private:
  std::vector<Component> readComponents() {
    std::vector<Component> components;
    expect('[');
    while (!consume(']')) {
      Component component;
      expect('{');
      while (!consume('}')) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          component.id = readString();
        } else if (key == "part") {
          component.part = readString();
        } else if (key == "pins") {
          component.pins = readPins();
        } else {
          throw std::runtime_error("unknown component key: " + key);
        }
        consume(',');
      }
      components.push_back(component);
      consume(',');
    }
    return components;
  }

  std::vector<Pin> readPins() {
    std::vector<Pin> pins;
    expect('[');
    while (!consume(']')) {
      Pin pin;
      expect('{');
      while (!consume('}')) {
        const std::string key = readString();
        expect(':');
        if (key == "name") {
          pin.name = readString();
        } else if (key == "kind") {
          pin.kind = readString();
        } else {
          throw std::runtime_error("unknown pin key: " + key);
        }
        consume(',');
      }
      pins.push_back(pin);
      consume(',');
    }
    return pins;
  }

  std::vector<Net> readNets() {
    std::vector<Net> nets;
    expect('[');
    while (!consume(']')) {
      Net net;
      expect('{');
      while (!consume('}')) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          net.id = readString();
        } else if (key == "members") {
          net.members = readMembers();
        } else {
          throw std::runtime_error("unknown net key: " + key);
        }
        consume(',');
      }
      nets.push_back(net);
      consume(',');
    }
    return nets;
  }

  std::vector<NetMember> readMembers() {
    std::vector<NetMember> members;
    expect('[');
    while (!consume(']')) {
      NetMember member;
      expect('{');
      while (!consume('}')) {
        const std::string key = readString();
        expect(':');
        if (key == "component_id") {
          member.component_id = readString();
        } else if (key == "pin_name") {
          member.pin_name = readString();
        } else {
          throw std::runtime_error("unknown net member key: " + key);
        }
        consume(',');
      }
      members.push_back(member);
      consume(',');
    }
    return members;
  }

  std::vector<Constraint> readConstraints() {
    std::vector<Constraint> constraints;
    expect('[');
    while (!consume(']')) {
      Constraint constraint;
      expect('{');
      while (!consume('}')) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          constraint.id = readString();
        } else if (key == "kind") {
          constraint.kind = readString();
        } else if (key == "target") {
          constraint.target = readString();
        } else if (key == "value") {
          constraint.value = readString();
        } else {
          throw std::runtime_error("unknown constraint key: " + key);
        }
        consume(',');
      }
      constraints.push_back(constraint);
      consume(',');
    }
    return constraints;
  }

  int readInt() {
    skipWhitespace();
    int value = 0;
    bool found = false;
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) {
      found = true;
      value = (value * 10) + (source_[pos_] - '0');
      ++pos_;
    }
    if (!found) {
      throw std::runtime_error("expected integer");
    }
    return value;
  }

  std::string readString() {
    skipWhitespace();
    expectRaw('"');
    std::string value;
    while (pos_ < source_.size() && source_[pos_] != '"') {
      if (source_[pos_] == '\\') {
        ++pos_;
        if (pos_ >= source_.size()) {
          throw std::runtime_error("unterminated escape");
        }
      }
      value.push_back(source_[pos_]);
      ++pos_;
    }
    expectRaw('"');
    return value;
  }

  bool consume(const char expected) {
    skipWhitespace();
    if (pos_ < source_.size() && source_[pos_] == expected) {
      ++pos_;
      return true;
    }
    return false;
  }

  void expect(const char expected) {
    if (!consume(expected)) {
      throw std::runtime_error(std::string("expected '") + expected + "'");
    }
  }

  void expectRaw(const char expected) {
    if (pos_ >= source_.size() || source_[pos_] != expected) {
      throw std::runtime_error(std::string("expected '") + expected + "'");
    }
    ++pos_;
  }

  void skipWhitespace() {
    while (pos_ < source_.size() &&
           std::isspace(static_cast<unsigned char>(source_[pos_])) != 0) {
      ++pos_;
    }
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

}  // namespace

std::string dumpProjectJson(const Project& project) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema_version\": " << project.schema_version << ",\n";
  writeField(out, 2, "id", project.id);
  writeField(out, 2, "name", project.name);

  out << "  \"components\": [\n";
  for (std::size_t i = 0; i < project.components.size(); ++i) {
    const Component& component = project.components.at(i);
    out << "    {\n";
    writeField(out, 6, "id", component.id);
    writeField(out, 6, "part", component.part);
    out << "      \"pins\": [\n";
    for (std::size_t j = 0; j < component.pins.size(); ++j) {
      const Pin& pin = component.pins.at(j);
      out << "        {\n";
      writeField(out, 10, "kind", pin.kind);
      writeField(out, 10, "name", pin.name, false);
      out << "        }" << (j + 1 == component.pins.size() ? "" : ",") << '\n';
    }
    out << "      ]\n";
    out << "    }" << (i + 1 == project.components.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"constraints\": [\n";
  for (std::size_t i = 0; i < project.constraints.size(); ++i) {
    const Constraint& constraint = project.constraints.at(i);
    out << "    {\n";
    writeField(out, 6, "id", constraint.id);
    writeField(out, 6, "kind", constraint.kind);
    writeField(out, 6, "target", constraint.target);
    writeField(out, 6, "value", constraint.value, false);
    out << "    }" << (i + 1 == project.constraints.size() ? "" : ",") << '\n';
  }
  out << "  ],\n";

  out << "  \"nets\": [\n";
  for (std::size_t i = 0; i < project.nets.size(); ++i) {
    const Net& net = project.nets.at(i);
    out << "    {\n";
    writeField(out, 6, "id", net.id);
    out << "      \"members\": [\n";
    for (std::size_t j = 0; j < net.members.size(); ++j) {
      const NetMember& member = net.members.at(j);
      out << "        {\n";
      writeField(out, 10, "component_id", member.component_id);
      writeField(out, 10, "pin_name", member.pin_name, false);
      out << "        }" << (j + 1 == net.members.size() ? "" : ",") << '\n';
    }
    out << "      ]\n";
    out << "    }" << (i + 1 == project.nets.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

Project loadProjectJson(const std::string& json) {
  JsonReader reader(json);
  return reader.readProject();
}

}  // namespace ccad

