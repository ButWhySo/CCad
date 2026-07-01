#include "ccad_cli/lib_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/kicad_symbol_import.hpp"
#include "ccad_core/kicad_footprint_export.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/library_catalog.hpp"
#include "ccad_core/footprint_losslessness.hpp"
#include "ccad_core/filesystem_u8.hpp"

#include "ccad_core/component_generator.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace ccad_cli {
namespace {

ccad::LibraryCatalog loadCatalogFile(const std::string& path) {
  std::ifstream input(ccad::u8ToPath(path));
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
            << "  \"summary\": {\n"
            << "    \"match_count\": " << items.size() << "\n"
            << "  },\n"
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
  std::size_t error_count = 0;
  std::size_t warning_count = 0;
  for (const ccad::CatalogDiagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == "error") {
      ++error_count;
    } else if (diagnostic.severity == "warning") {
      ++warning_count;
    }
  }

  std::cout << "{\n"
            << "  \"summary\": {\n"
            << "    \"total\": " << diagnostics.size() << ",\n"
            << "    \"errors\": " << error_count << ",\n"
            << "    \"warnings\": " << warning_count << "\n"
            << "  },\n"
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

      std::ifstream input(ccad::u8ToPath(in_path));
      if (!input) {
        std::cerr << "failed to open footprint input file: " << in_path << '\n';
        return 2;
      }
      std::ostringstream buffer;
      buffer << input.rdbuf();
      const ccad::Footprint footprint = ccad::importKiCadFootprint(buffer.str());

      std::ofstream output(ccad::u8ToPath(out_path));
      if (!output) {
        std::cerr << "failed to open footprint output file: " << out_path << '\n';
        return 2;
      }
      output << ccad::dumpFootprintJson(footprint);
      return static_cast<bool>(output) ? 0 : 2;
    }

    if (subcommand == "import-symbol") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--in", "--out"});
      const std::string in_path = requireOption(options, "--in");
      const std::string out_path = requireOption(options, "--out");

      std::ifstream input(ccad::u8ToPath(in_path));
      if (!input) {
        std::cerr << "failed to open symbol input file: " << in_path << '\n';
        return 2;
      }
      std::ostringstream buffer;
      buffer << input.rdbuf();
      auto symbols = ccad::importKiCadSymbolLibrary(buffer.str());

      std::ofstream output(ccad::u8ToPath(out_path));
      if (!output) {
        std::cerr << "failed to open symbol output file: " << out_path << '\n';
        return 2;
      }
      output << ccad::dumpSymbolsJson(symbols);
      return static_cast<bool>(output) ? 0 : 2;
    }

    if (subcommand == "new-symbol") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--name", "--pins", "--out"});
      const std::string name = requireOption(options, "--name");
      const std::string out_path = requireOption(options, "--out");
      
      int pins = 8;
      if (options.contains("--pins")) {
        pins = std::stoi(options.at("--pins"));
      }

      std::ofstream output(ccad::u8ToPath(out_path));
      if (!output) {
        std::cerr << "failed to open output file: " << out_path << '\n';
        return 2;
      }

      ccad::SymbolParams params;
      params.name = name;
      params.pin_count = pins;
      ccad::Symbol sym = ccad::generateParametricSymbol(params);
      output << ccad::dumpSymbolsJson({sym});
      return static_cast<bool>(output) ? 0 : 2;
    }

    if (subcommand == "new-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--name", "--pins", "--package", "--out"});
      const std::string name = requireOption(options, "--name");
      const std::string out_path = requireOption(options, "--out");
      
      int pins = 8;
      if (options.contains("--pins")) {
        pins = std::stoi(options.at("--pins"));
      }

      std::ofstream output(out_path);
      if (!output) {
        std::cerr << "failed to open output file: " << out_path << '\n';
        return 2;
      }

      ccad::FootprintParams params;
      params.name = name;
      params.pin_count = pins;
      if (options.contains("--package")) {
        params.package_type = options.at("--package");
      } else {
        params.package_type = "SOP"; // default
      }
      ccad::Footprint fp = ccad::generateParametricFootprint(params);
      output << ccad::dumpFootprintJson(fp);
      return static_cast<bool>(output) ? 0 : 2;
    }

    if (subcommand == "export-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--in", "--out"});
      const std::string in_path = requireOption(options, "--in");
      const std::string out_path = requireOption(options, "--out");

      const ccad::Footprint footprint = loadFootprintFile(in_path);

      std::ofstream output(ccad::u8ToPath(out_path));
      if (!output) {
        std::cerr << "failed to open footprint output file: " << out_path << '\n';
        return 2;
      }
      output << ccad::exportKiCadFootprint(footprint);
      return static_cast<bool>(output) ? 0 : 2;
    }

    if (subcommand == "verify-footprint-losslessness") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--in-kicad", "--in-ccad"});
      const std::string in_kicad = requireOption(options, "--in-kicad");
      const std::string in_ccad = requireOption(options, "--in-ccad");

      std::ifstream input_kicad(ccad::u8ToPath(in_kicad));
      if (!input_kicad) {
        std::cerr << "failed to open KiCad footprint file: " << in_kicad << '\n';
        return 2;
      }
      std::ostringstream buffer_kicad;
      buffer_kicad << input_kicad.rdbuf();
      const ccad::Footprint original = ccad::importKiCadFootprint(buffer_kicad.str());

      const ccad::Footprint candidate = loadFootprintFile(in_ccad);

      const auto diagnostics = ccad::verifyFootprintLosslessness(original, candidate);

      std::cout << "{\n"
                << "  \"lossless\": " << (diagnostics.empty() ? "true" : "false") << ",\n"
                << "  \"diagnostics\": [\n";
      for (std::size_t i = 0; i < diagnostics.size(); ++i) {
        const auto& diag = diagnostics.at(i);
        std::cout << "    {\n"
                  << "      \"severity\": \"" << ccad::escapeJson(diag.severity) << "\",\n"
                  << "      \"field\": \"" << ccad::escapeJson(diag.field) << "\",\n"
                  << "      \"message\": \"" << ccad::escapeJson(diag.message) << "\"\n"
                  << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << '\n';
      }
      std::cout << "  ]\n"
                << "}\n";

      return diagnostics.empty() ? 0 : 1;
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
