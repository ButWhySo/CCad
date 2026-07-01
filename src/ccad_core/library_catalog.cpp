#include "ccad_core/library_catalog.hpp"

#include "ccad_core/json.hpp"
#include "ccad_core/filesystem_u8.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

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

void writeStringArray(std::ostringstream& out, const int indent, const std::string& key,
                      const std::vector<std::string>& values, const bool comma = true) {
  out << std::string(indent, ' ') << '"' << key << "\": [\n";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out << std::string(indent + 2, ' ') << '"' << escapeJson(values.at(i)) << '"'
        << (i + 1 == values.size() ? "" : ",") << '\n';
  }
  out << std::string(indent, ' ') << ']';
  if (comma) {
    out << ',';
  }
  out << '\n';
}

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

bool containsCaseInsensitive(const std::string& value, const std::string& query) {
  return lowercase(value).find(query) != std::string::npos;
}

void addDiagnostic(std::vector<CatalogDiagnostic>& diagnostics, std::string code,
                   std::string message, std::string object_id) {
  diagnostics.push_back(CatalogDiagnostic{
      .severity = "error",
      .code = std::move(code),
      .message = std::move(message),
      .object_id = std::move(object_id),
  });
}

std::uint32_t rotateRight(const std::uint32_t value, const std::uint32_t bits) {
  return (value >> bits) | (value << (32U - bits));
}

std::string sha256File(const std::filesystem::path& path) {
  static constexpr std::array<std::uint32_t, 64> k = {
      0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
      0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
      0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
      0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
      0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
      0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
      0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
      0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
      0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
      0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
      0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open native artifact for checksum");
  }
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
  const std::uint64_t bit_length = static_cast<std::uint64_t>(bytes.size()) * 8U;
  bytes.push_back(0x80U);
  while ((bytes.size() % 64U) != 56U) {
    bytes.push_back(0U);
  }
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<unsigned char>((bit_length >> shift) & 0xffU));
  }

  std::array<std::uint32_t, 8> h = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                   0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
  for (std::size_t offset = 0; offset < bytes.size(); offset += 64U) {
    std::array<std::uint32_t, 64> w = {};
    for (std::size_t i = 0; i < 16U; ++i) {
      const std::size_t j = offset + (i * 4U);
      w.at(i) = (static_cast<std::uint32_t>(bytes.at(j)) << 24U) |
                (static_cast<std::uint32_t>(bytes.at(j + 1U)) << 16U) |
                (static_cast<std::uint32_t>(bytes.at(j + 2U)) << 8U) |
                static_cast<std::uint32_t>(bytes.at(j + 3U));
    }
    for (std::size_t i = 16U; i < 64U; ++i) {
      const std::uint32_t s0 = rotateRight(w.at(i - 15U), 7U) ^
                               rotateRight(w.at(i - 15U), 18U) ^ (w.at(i - 15U) >> 3U);
      const std::uint32_t s1 = rotateRight(w.at(i - 2U), 17U) ^
                               rotateRight(w.at(i - 2U), 19U) ^ (w.at(i - 2U) >> 10U);
      w.at(i) = w.at(i - 16U) + s0 + w.at(i - 7U) + s1;
    }

    std::uint32_t a = h.at(0);
    std::uint32_t b = h.at(1);
    std::uint32_t c = h.at(2);
    std::uint32_t d = h.at(3);
    std::uint32_t e = h.at(4);
    std::uint32_t f = h.at(5);
    std::uint32_t g = h.at(6);
    std::uint32_t hh = h.at(7);
    for (std::size_t i = 0; i < 64U; ++i) {
      const std::uint32_t s1 = rotateRight(e, 6U) ^ rotateRight(e, 11U) ^ rotateRight(e, 25U);
      const std::uint32_t ch = (e & f) ^ ((~e) & g);
      const std::uint32_t temp1 = hh + s1 + ch + k.at(i) + w.at(i);
      const std::uint32_t s0 = rotateRight(a, 2U) ^ rotateRight(a, 13U) ^ rotateRight(a, 22U);
      const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t temp2 = s0 + maj;
      hh = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }
    h.at(0) += a;
    h.at(1) += b;
    h.at(2) += c;
    h.at(3) += d;
    h.at(4) += e;
    h.at(5) += f;
    h.at(6) += g;
    h.at(7) += hh;
  }

  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const std::uint32_t value : h) {
    out << std::setw(8) << value;
  }
  return out.str();
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
      } else if (key == "usage_summary") {
        item.usage_summary = readString();
      } else if (key == "layout_notes") {
        item.layout_notes = readStringArray();
      } else if (key == "source_confidence") {
        item.source_confidence = readString();
      } else if (key == "review_status") {
        item.review_status = readString();
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
    writeField(out, 6, "usage_summary", item.usage_summary);
    writeStringArray(out, 6, "layout_notes", item.layout_notes);
    writeField(out, 6, "source_confidence", item.source_confidence);
    writeField(out, 6, "review_status", item.review_status);
    writeStringArray(out, 6, "warnings", item.warnings, false);
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

void saveLibraryCatalog(const std::filesystem::path& path, const LibraryCatalog& catalog) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open library catalog for writing: " + path.string());
  }
  out << dumpLibraryCatalogJson(catalog);
}

LibraryCatalog loadLibraryCatalog(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open library catalog for reading: " + path.string());
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return loadLibraryCatalogJson(buffer.str());
}

const LibraryItem* findLibraryItem(const LibraryCatalog& catalog, const std::string& id) {
  for (const LibraryItem& item : catalog.items) {
    if (item.id == id) {
      return &item;
    }
  }
  return nullptr;
}

std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,
                                                   const std::string& query) {
  return searchLibraryItems(catalog, query, "");
}

std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,
                                                   const std::string& query,
                                                   const std::string& kind) {
  const std::string normalized_query = lowercase(query);
  const std::string normalized_kind = lowercase(kind);
  std::vector<const LibraryItem*> matches;
  if (normalized_query.empty()) {
    return matches;
  }
  for (const LibraryItem& item : catalog.items) {
    if (!normalized_kind.empty() && lowercase(item.kind) != normalized_kind) {
      continue;
    }
    if (containsCaseInsensitive(item.id, normalized_query) ||
        containsCaseInsensitive(item.name, normalized_query) ||
        containsCaseInsensitive(item.kind, normalized_query) ||
        containsCaseInsensitive(item.source_path, normalized_query) ||
        containsCaseInsensitive(item.native_path, normalized_query) ||
        containsCaseInsensitive(item.usage_summary, normalized_query) ||
        containsCaseInsensitive(item.source_confidence, normalized_query) ||
        containsCaseInsensitive(item.review_status, normalized_query)) {
      matches.push_back(&item);
      continue;
    }
    for (const std::string& note : item.layout_notes) {
      if (containsCaseInsensitive(note, normalized_query)) {
        matches.push_back(&item);
        break;
      }
    }
  }
  return matches;
}

std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog) {
  std::vector<CatalogDiagnostic> diagnostics;
  std::set<std::string> item_ids;
  for (const LibraryItem& item : catalog.items) {
    if (item.id.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_ID", "Library item has no stable ID",
                    item.name);
    } else if (!item_ids.insert(item.id).second) {
      addDiagnostic(diagnostics, "DUPLICATE_ITEM_ID",
                    "Library item ID is duplicated in the catalog", item.id);
    }
    if (item.kind.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_KIND", "Library item has no kind", item.id);
    }
    if (item.name.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_NAME", "Library item has no display name",
                    item.id);
    }
    if (item.source_path.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_SOURCE_PATH",
                    "Library item has no upstream source path", item.id);
    }
    if (item.native_path.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_NATIVE_PATH",
                    "Library item has no CCad-native artifact path", item.id);
    }
    if (item.sha256.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_SHA256",
                    "Library item has no source checksum", item.id);
    }
    if (item.license.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_LICENSE",
                    "Library item has no license metadata", item.id);
    }
    if (item.provenance.empty()) {
      addDiagnostic(diagnostics, "MISSING_ITEM_PROVENANCE",
                    "Library item has no provenance metadata", item.id);
    }
    if (!item.review_status.empty()) {
      if (item.review_status != "generated" && item.review_status != "needs_review" &&
          item.review_status != "reviewed" && item.review_status != "rejected") {
        addDiagnostic(diagnostics, "INVALID_REVIEW_STATUS",
                      "Library item has invalid review status", item.id);
      }
    }
  }
  return diagnostics;
}

std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog,
                                                      const std::filesystem::path& root) {
  std::vector<CatalogDiagnostic> diagnostics = validateLibraryCatalog(catalog);
  for (const LibraryItem& item : catalog.items) {
    if (item.native_path.empty() || item.sha256.empty()) {
      continue;
    }
    const std::filesystem::path native_path = root / ccad::u8ToPath(item.native_path);
    if (!std::filesystem::exists(native_path)) {
      addDiagnostic(diagnostics, "MISSING_NATIVE_FILE",
                    "Library item native artifact is missing under catalog root", item.id);
      continue;
    }
    const std::string actual_sha256 = sha256File(native_path);
    if (lowercase(actual_sha256) != lowercase(item.sha256)) {
      addDiagnostic(diagnostics, "ITEM_SHA256_MISMATCH",
                    "Library item native artifact checksum does not match catalog metadata",
                    item.id);
    }
  }
  return diagnostics;
}

}  // namespace ccad
