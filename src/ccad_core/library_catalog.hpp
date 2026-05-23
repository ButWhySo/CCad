#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ccad {

struct LibrarySource {
  std::string name;
  std::string kind;
  std::string url;
  std::string commit;
  std::string mirror;
  std::string fetched_at;
};

struct LibraryItem {
  std::string id;
  std::string kind;
  std::string name;
  std::string source_path;
  std::string native_path;
  std::string sha256;
  std::string license;
  std::string provenance;
  std::string usage_summary;
  std::vector<std::string> layout_notes;
  std::string source_confidence;
  std::string review_status;
  std::vector<std::string> warnings;
};

struct LibraryCatalog {
  int schema_version = 1;
  std::string name;
  LibrarySource source;
  std::vector<LibraryItem> items;
};

struct CatalogDiagnostic {
  std::string severity;
  std::string code;
  std::string message;
  std::string object_id;
};

std::string dumpLibraryCatalogJson(const LibraryCatalog& catalog);
LibraryCatalog loadLibraryCatalogJson(const std::string& json);
const LibraryItem* findLibraryItem(const LibraryCatalog& catalog, const std::string& id);
std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,
                                                   const std::string& query);
std::vector<const LibraryItem*> searchLibraryItems(const LibraryCatalog& catalog,
                                                   const std::string& query,
                                                   const std::string& kind);
std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog);
std::vector<CatalogDiagnostic> validateLibraryCatalog(const LibraryCatalog& catalog,
                                                      const std::filesystem::path& root);

}  // namespace ccad
