#include "ccad_core/library_catalog.hpp"

#include "ccad_core/json.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace ccad {
namespace {

void writeField(std::ostringstream& out, const int indent, const std::string& key,
                const std::string& value, const bool comma = true) {
  out << std::string(indent, ' ') << '"' << key << "\": \"" << escapeJson(value) << '"';
  if (comma) {
    out << ',';
  }
  out << '\n';
}

class CatalogJsonReader {
 public:
  explicit CatalogJsonReader(std::string_view source) : source_(source) {}

  LibraryCatalog readCatalog() {
    LibraryCatalog catalog;
    bool saw_source = false;
    bool saw_items = false;
    expect('{');
    while (!consume('}')) {
      const std::string key = readString();
      expect(':');
      if (key == "schema_version") {
        catalog.schema_version = readInt();
      } else if (key == "name") {
        catalog.name = readString();
      } else if (key == "source") {
        catalog.source = readSource();
        saw_source = true;
      } else if (key == "items") {
        catalog.items = readItems();
        saw_items = true;
      } else {
        throw std::runtime_error("unknown catalog key: " + key);
      }
      if (consume('}')) {
        break;
      }
      expect(',');
      if (peek('}')) {
        throw std::runtime_error("trailing comma in catalog object");
      }
    }
    if (!saw_source || !saw_items) {
      throw std::runtime_error("catalog requires source and items");
    }
    return catalog;
  }

  void finish() {
    skipWhitespace();
    if (pos_ != source_.size()) {
      throw std::runtime_error("trailing content after catalog");
    }
  }

 private:
  LibrarySource readSource() {
    LibrarySource source;
    expect('{');
    while (!consume('}')) {
      const std::string key = readString();
      expect(':');
      if (key == "name") {
        source.name = readString();
      } else if (key == "kind") {
        source.kind = readString();
      } else if (key == "url") {
        source.url = readString();
      } else if (key == "commit") {
        source.commit = readString();
      } else if (key == "mirror") {
        source.mirror = readString();
      } else if (key == "fetched_at") {
        source.fetched_at = readString();
      } else {
        throw std::runtime_error("unknown source key: " + key);
      }
      if (consume('}')) {
        break;
      }
      expect(',');
      if (peek('}')) {
        throw std::runtime_error("trailing comma in source object");
      }
    }
    return source;
  }

  std::vector<LibraryItem> readItems() {
    std::vector<LibraryItem> items;
    expect('[');
    while (!consume(']')) {
      items.push_back(readItem());
      if (consume(']')) {
        break;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in items array");
      }
    }
    return items;
  }

  LibraryItem readItem() {
    LibraryItem item;
    expect('{');
    while (!consume('}')) {
      const std::string key = readString();
      expect(':');
      if (key == "id") {
        item.id = readString();
      } else if (key == "kind") {
        item.kind = readString();
      } else if (key == "name") {
        item.name = readString();
      } else if (key == "source_path") {
        item.source_path = readString();
      } else if (key == "native_path") {
        item.native_path = readString();
      } else if (key == "sha256") {
        item.sha256 = readString();
      } else if (key == "license") {
        item.license = readString();
      } else if (key == "provenance") {
        item.provenance = readString();
      } else if (key == "warnings") {
        item.warnings = readStringArray();
      } else {
        throw std::runtime_error("unknown library item key: " + key);
      }
      if (consume('}')) {
        break;
      }
      expect(',');
      if (peek('}')) {
        throw std::runtime_error("trailing comma in library item object");
      }
    }
    return item;
  }

  std::vector<std::string> readStringArray() {
    std::vector<std::string> values;
    expect('[');
    while (!consume(']')) {
      values.push_back(readString());
      if (consume(']')) {
        break;
      }
      expect(',');
      if (peek(']')) {
        throw std::runtime_error("trailing comma in string array");
      }
    }
    return values;
  }

  int readInt() {
    skipWhitespace();
    int value = 0;
    bool found = false;
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_])) != 0) {
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
      value.push_back(source_[pos_++]);
    }
    expectRaw('"');
    return value;
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
      default:
        throw std::runtime_error("invalid string escape");
    }
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

std::string dumpLibraryCatalogJson(const LibraryCatalog& catalog) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema_version\": " << catalog.schema_version << ",\n";
  writeField(out, 2, "name", catalog.name);
  out << "  \"source\": {\n";
  writeField(out, 4, "name", catalog.source.name);
  writeField(out, 4, "kind", catalog.source.kind);
  writeField(out, 4, "url", catalog.source.url);
  writeField(out, 4, "commit", catalog.source.commit);
  writeField(out, 4, "mirror", catalog.source.mirror);
  writeField(out, 4, "fetched_at", catalog.source.fetched_at, false);
  out << "  },\n";
  out << "  \"items\": [\n";
  for (std::size_t i = 0; i < catalog.items.size(); ++i) {
    const LibraryItem& item = catalog.items.at(i);
    out << "    {\n";
    writeField(out, 6, "id", item.id);
    writeField(out, 6, "kind", item.kind);
    writeField(out, 6, "name", item.name);
    writeField(out, 6, "source_path", item.source_path);
    writeField(out, 6, "native_path", item.native_path);
    writeField(out, 6, "sha256", item.sha256);
    writeField(out, 6, "license", item.license);
    writeField(out, 6, "provenance", item.provenance);
    out << "      \"warnings\": [\n";
    for (std::size_t j = 0; j < item.warnings.size(); ++j) {
      out << "        \"" << escapeJson(item.warnings.at(j)) << "\""
          << (j + 1 == item.warnings.size() ? "" : ",") << '\n';
    }
    out << "      ]\n";
    out << "    }" << (i + 1 == catalog.items.size() ? "" : ",") << '\n';
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

LibraryCatalog loadLibraryCatalogJson(const std::string& json) {
  CatalogJsonReader reader(json);
  LibraryCatalog catalog = reader.readCatalog();
  reader.finish();
  return catalog;
}

const LibraryItem* findLibraryItem(const LibraryCatalog& catalog, const std::string& id) {
  for (const LibraryItem& item : catalog.items) {
    if (item.id == id) {
      return &item;
    }
  }
  return nullptr;
}

}  // namespace ccad
