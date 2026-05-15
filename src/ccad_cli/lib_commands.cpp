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

void printCatalogItem(const ccad::LibraryItem& item) {
  std::cout << "{\n"
            << "  \"found\": true,\n"
            << "  \"item\": {\n"
            << "    \"id\": \"" << ccad::escapeJson(item.id) << "\",\n"
            << "    \"kind\": \"" << ccad::escapeJson(item.kind) << "\",\n"
            << "    \"license\": \"" << ccad::escapeJson(item.license) << "\",\n"
            << "    \"name\": \"" << ccad::escapeJson(item.name) << "\",\n"
            << "    \"native_path\": \"" << ccad::escapeJson(item.native_path) << "\",\n"
            << "    \"provenance\": \"" << ccad::escapeJson(item.provenance) << "\",\n"
            << "    \"sha256\": \"" << ccad::escapeJson(item.sha256) << "\",\n"
            << "    \"source_path\": \"" << ccad::escapeJson(item.source_path) << "\"\n"
            << "  }\n"
            << "}\n";
}

void printMissingCatalogItem(const std::string& id) {
  std::cout << "{\n"
            << "  \"found\": false,\n"
            << "  \"id\": \"" << ccad::escapeJson(id) << "\"\n"
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

    std::cerr << "unknown lib subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to import library data: " << error.what() << '\n';
    return 2;
  }
}

}  // namespace ccad_cli
