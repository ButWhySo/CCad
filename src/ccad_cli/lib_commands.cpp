#include "ccad_cli/lib_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/library_catalog.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace ccad_cli {
namespace {

ccad::LibraryCatalog loadCatalogFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open catalog file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return ccad::loadLibraryCatalogJson(buffer.str());
}

void printCatalogInfo(const ccad::LibraryCatalog& catalog) {
  std::cout << "{\n"
            << "  \"item_count\": " << catalog.items.size() << ",\n"
            << "  \"name\": \"" << ccad::escapeJson(catalog.name) << "\",\n"
            << "  \"schema_version\": " << catalog.schema_version << ",\n"
            << "  \"source\": {\n"
            << "    \"commit\": \"" << ccad::escapeJson(catalog.source.commit) << "\",\n"
            << "    \"kind\": \"" << ccad::escapeJson(catalog.source.kind) << "\",\n"
            << "    \"mirror\": \"" << ccad::escapeJson(catalog.source.mirror) << "\",\n"
            << "    \"name\": \"" << ccad::escapeJson(catalog.source.name) << "\",\n"
            << "    \"url\": \"" << ccad::escapeJson(catalog.source.url) << "\"\n"
            << "  }\n"
            << "}\n";
}

void printStringArray(const std::vector<std::string>& values, int indent) {
  const std::string padding(static_cast<std::size_t>(indent), ' ');
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << padding << "\"" << ccad::escapeJson(values.at(i)) << "\""
              << (i + 1 == values.size() ? "" : ",") << '\n';
  }
}

void printCatalogItemFields(const ccad::LibraryItem& item, int indent) {
  const std::string padding(static_cast<std::size_t>(indent), ' ');
  std::cout << padding << "\"id\": \"" << ccad::escapeJson(item.id) << "\",\n"
            << padding << "\"kind\": \"" << ccad::escapeJson(item.kind) << "\",\n"
            << padding << "\"layout_notes\": [\n";
  printStringArray(item.layout_notes, indent + 2);
  std::cout << padding << "],\n"
            << padding << "\"license\": \"" << ccad::escapeJson(item.license) << "\",\n"
            << padding << "\"name\": \"" << ccad::escapeJson(item.name) << "\",\n"
            << padding << "\"native_path\": \"" << ccad::escapeJson(item.native_path)
            << "\",\n"
            << padding << "\"provenance\": \"" << ccad::escapeJson(item.provenance)
            << "\",\n"
            << padding << "\"review_status\": \"" << ccad::escapeJson(item.review_status)
            << "\",\n"
            << padding << "\"sha256\": \"" << ccad::escapeJson(item.sha256) << "\",\n"
            << padding << "\"source_confidence\": \""
            << ccad::escapeJson(item.source_confidence) << "\",\n"
            << padding << "\"source_path\": \"" << ccad::escapeJson(item.source_path)
            << "\",\n"
            << padding << "\"usage_summary\": \"" << ccad::escapeJson(item.usage_summary)
            << "\"\n";
}

void printCatalogItem(const ccad::LibraryItem& item) {
  std::cout << "{\n"
            << "  \"found\": true,\n"
            << "  \"item\": {\n";
  printCatalogItemFields(item, 4);
  std::cout << "  }\n"
            << "}\n";
}

void printCatalogSearchResults(const std::string& query, const std::string& kind,
                               const std::vector<const ccad::LibraryItem*>& items) {
  std::cout << "{\n"
            << "  \"count\": " << items.size() << ",\n"
            << "  \"items\": [\n";
  for (std::size_t i = 0; i < items.size(); ++i) {
    const ccad::LibraryItem& item = *items.at(i);
    std::cout << "    {\n";
    printCatalogItemFields(item, 6);
    std::cout << "    }" << (i + 1 == items.size() ? "" : ",") << '\n';
  }
  std::cout << "  ],\n"
            << "  \"kind\": \"" << ccad::escapeJson(kind) << "\",\n"
            << "  \"query\": \"" << ccad::escapeJson(query) << "\"\n"
            << "}\n";
}

void printMissingCatalogItem(const std::string& id) {
  std::cout << "{\n"
            << "  \"found\": false,\n"
            << "  \"id\": \"" << ccad::escapeJson(id) << "\"\n"
            << "}\n";
}

void printCatalogDiagnostics(const std::vector<ccad::CatalogDiagnostic>& diagnostics) {
  std::cout << "{\n"
            << "  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < diagnostics.size(); ++i) {
    const ccad::CatalogDiagnostic& diagnostic = diagnostics.at(i);
    std::cout << "    {\n"
              << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n"
              << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n"
              << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\",\n"
              << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\"\n"
              << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << '\n';
  }
  std::cout << "  ]\n"
            << "}\n";
}

}  // namespace

int libCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "lib requires a subcommand\n";
    return 2;
  }

  try {
    const std::string& subcommand = args.at(0);
    if (subcommand == "import-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--in", "--out"});
      const std::string in_path = requireOption(options, "--in");
      const std::string out_path = requireOption(options, "--out");

      std::ifstream input(in_path);
      if (!input) {
        std::cerr << "failed to open footprint input file: " << in_path << '\n';
        return 2;
      }
      std::ostringstream buffer;
      buffer << input.rdbuf();
      const ccad::Footprint footprint = ccad::importKiCadFootprint(buffer.str());

      std::ofstream output(out_path);
      if (!output) {
        std::cerr << "failed to open footprint output file: " << out_path << '\n';
        return 2;
      }
      output << ccad::dumpFootprintJson(footprint);
      return static_cast<bool>(output) ? 0 : 2;
    }

    if (subcommand == "catalog-info") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--catalog"});
      printCatalogInfo(loadCatalogFile(requireOption(options, "--catalog")));
      return 0;
    }

    if (subcommand == "catalog-find") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--catalog", "--id"});
      const std::string id = requireOption(options, "--id");
      const ccad::LibraryCatalog catalog = loadCatalogFile(requireOption(options, "--catalog"));
      const ccad::LibraryItem* item = ccad::findLibraryItem(catalog, id);
      if (item == nullptr) {
        printMissingCatalogItem(id);
        return 1;
      }
      printCatalogItem(*item);
      return 0;
    }

    if (subcommand == "catalog-search") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--catalog", "--query", "--kind"});
      const std::string query = requireOption(options, "--query");
      std::string kind;
      if (options.contains("--kind")) {
        kind = requireOption(options, "--kind");
      }
      const ccad::LibraryCatalog catalog = loadCatalogFile(requireOption(options, "--catalog"));
      printCatalogSearchResults(query, kind, ccad::searchLibraryItems(catalog, query, kind));
      return 0;
    }

    if (subcommand == "catalog-validate") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--catalog", "--root"});
      const ccad::LibraryCatalog catalog = loadCatalogFile(requireOption(options, "--catalog"));
      std::vector<ccad::CatalogDiagnostic> diagnostics;
      if (options.contains("--root")) {
        diagnostics = ccad::validateLibraryCatalog(catalog, requireOption(options, "--root"));
      } else {
        diagnostics = ccad::validateLibraryCatalog(catalog);
      }
      printCatalogDiagnostics(diagnostics);
      return diagnostics.empty() ? 0 : 1;
    }

    std::cerr << "unknown lib subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to import library data: " << error.what() << '\n';
    return 2;
  }
}

}  // namespace ccad_cli
