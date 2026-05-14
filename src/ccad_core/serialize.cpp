#include "ccad_core/serialize.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace ccad {
namespace {

std::string escapeJson(const std::string& value) {
  std::string out;
  for (const unsigned char ch : value) {
    switch (ch) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (ch < 0x20) {
          std::ostringstream escaped;
          escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                  << static_cast<int>(ch);
          out += escaped.str();
        } else {
          out.push_back(static_cast<char>(ch));
        }
        break;
    }
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
    if (consume('}')) {
      return project;
    }
    while (true) {
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
      if (consume('}')) {
        return project;
      }
      expect(',');
      if (peek('}')) {
        throw std::runtime_error("trailing comma in project object");
      }
    }
  }

  void finish() {
    skipWhitespace();
    if (pos_ != source_.size()) {
      throw std::runtime_error("trailing content after root object");
    }
  }

 private:
  std::vector<Component> readComponents() {
    std::vector<Component> components;
    expect('[');
    if (consume(']')) {
      return components;
    }
    while (true) {
      Component component;
      expect('{');
      if (!consume('}')) {
        while (true) {
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
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in component object");
        }
        }
      }
      components.push_back(component);
      if (consume(']')) {
        return components;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in components array");
      }
    }
  }

  std::vector<Pin> readPins() {
    std::vector<Pin> pins;
    expect('[');
    if (consume(']')) {
      return pins;
    }
    while (true) {
      Pin pin;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "name") {
          pin.name = readString();
        } else if (key == "kind") {
          pin.kind = readString();
        } else {
          throw std::runtime_error("unknown pin key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in pin object");
        }
        }
      }
      pins.push_back(pin);
      if (consume(']')) {
        return pins;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in pins array");
      }
    }
  }

  std::vector<Net> readNets() {
    std::vector<Net> nets;
    expect('[');
    if (consume(']')) {
      return nets;
    }
    while (true) {
      Net net;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "id") {
          net.id = readString();
        } else if (key == "members") {
          net.members = readMembers();
        } else {
          throw std::runtime_error("unknown net key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in net object");
        }
        }
      }
      nets.push_back(net);
      if (consume(']')) {
        return nets;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in nets array");
      }
    }
  }

  std::vector<NetMember> readMembers() {
    std::vector<NetMember> members;
    expect('[');
    if (consume(']')) {
      return members;
    }
    while (true) {
      NetMember member;
      expect('{');
      if (!consume('}')) {
        while (true) {
        const std::string key = readString();
        expect(':');
        if (key == "component_id") {
          member.component_id = readString();
        } else if (key == "pin_name") {
          member.pin_name = readString();
        } else {
          throw std::runtime_error("unknown net member key: " + key);
        }
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in net member object");
        }
        }
      }
      members.push_back(member);
      if (consume(']')) {
        return members;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in members array");
      }
    }
  }

  std::vector<Constraint> readConstraints() {
    std::vector<Constraint> constraints;
    expect('[');
    if (consume(']')) {
      return constraints;
    }
    while (true) {
      Constraint constraint;
      expect('{');
      if (!consume('}')) {
        while (true) {
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
        if (consume('}')) {
          break;
        }
        expect(',');
        if (peek('}')) {
          throw std::runtime_error("trailing comma in constraint object");
        }
        }
      }
      constraints.push_back(constraint);
      if (consume(']')) {
        return constraints;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in constraints array");
      }
    }
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
        value += readEscape();
        continue;
      }
      if (static_cast<unsigned char>(source_[pos_]) < 0x20) {
        throw std::runtime_error("unescaped control character in string");
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

  bool peek(const char expected) {
    skipWhitespace();
    return pos_ < source_.size() && source_[pos_] == expected;
  }

  std::string readEscape() {
    const char escaped = source_[pos_++];
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        return std::string(1, escaped);
      case 'b':
        return "\b";
      case 'f':
        return "\f";
      case 'n':
        return "\n";
      case 'r':
        return "\r";
      case 't':
        return "\t";
      case 'u':
        return readUnicodeEscape();
      default:
        throw std::runtime_error("invalid string escape");
    }
  }

  std::string readUnicodeEscape() {
    int codepoint = 0;
    for (int i = 0; i < 4; ++i) {
      if (pos_ >= source_.size() || std::isxdigit(static_cast<unsigned char>(source_[pos_])) == 0) {
        throw std::runtime_error("invalid unicode escape");
      }
      const char ch = source_[pos_++];
      codepoint *= 16;
      if (ch >= '0' && ch <= '9') {
        codepoint += ch - '0';
      } else if (ch >= 'a' && ch <= 'f') {
        codepoint += 10 + ch - 'a';
      } else {
        codepoint += 10 + ch - 'A';
      }
    }

    std::string out;
    if (codepoint <= 0x7F) {
      out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
      out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
      out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return out;
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
  Project project = reader.readProject();
  reader.finish();
  return project;
}

}  // namespace ccad
