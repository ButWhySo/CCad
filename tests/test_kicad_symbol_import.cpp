#include "ccad_core/kicad_symbol_import.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string readFile(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) throw std::runtime_error("Could not open " + path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream output(path, std::ios::binary);
  output << content;
}

void testLocalSymbolInheritance() {
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / "ccad-symbol-inheritance-test";
  std::filesystem::create_directories(dir);
  writeFile(dir / "1N4001.json",
            "{\n"
            "  \"name\": \"1N4001\",\n"
            "  \"extends\": \"\",\n"
            "  \"pins\": [\n"
            "    {\"name\":\"K\", \"number\":\"1\", \"electrical_type\":\"passive\", \"x_nm\":-3810000, \"y_nm\":0, \"rotation_degrees\":0},\n"
            "    {\"name\":\"A\", \"number\":\"2\", \"electrical_type\":\"passive\", \"x_nm\":3810000, \"y_nm\":0, \"rotation_degrees\":180}\n"
            "  ],\n"
            "  \"properties\": [],\n"
            "  \"rectangles\": [],\n"
            "  \"lines\": [],\n"
            "  \"arcs\": [],\n"
            "  \"circles\": [],\n"
            "  \"polylines\": [],\n"
            "  \"texts\": []\n"
            "}\n");
  writeFile(dir / "1N4007.json",
            "{\n"
            "  \"name\": \"1N4007\",\n"
            "  \"extends\": \"1N4001\",\n"
            "  \"pins\": [],\n"
            "  \"properties\": [],\n"
            "  \"rectangles\": [],\n"
            "  \"lines\": [],\n"
            "  \"arcs\": [],\n"
            "  \"circles\": [],\n"
            "  \"polylines\": [],\n"
            "  \"texts\": []\n"
            "}\n");

  const ccad::Symbol child = ccad::loadSymbolJsonFileWithLocalInheritance(dir / "1N4007.json");
  require(child.name == "1N4007", "child symbol keeps its own name");
  require(child.pins.size() == 2, "derived symbol inherits parent pins");
  require(child.pins.at(0).number == "1", "derived symbol inherits pin number");
  require(child.pins.at(1).name == "A", "derived symbol inherits pin name");
}

void testTopLevelSymbolLibraryItemListing() {
  const std::string content =
      "(kicad_symbol_lib\n"
      "  (version 20240101)\n"
      "  (generator ccad-test)\n"
      "  (symbol \"Parent\"\n"
      "    (property \"Reference\" \"U\" (id 0) (at 0 0 0))\n"
      "    (symbol \"Parent_1_1\"\n"
      "      (pin passive line (at 0 0 0) (length 2.54) (name \"A\") (number \"1\"))\n"
      "    )\n"
      "  )\n"
      "  (symbol \"Derived\"\n"
      "    (extends \"Parent\")\n"
      "    (property \"Reference\" \"U\" (id 0) (at 0 0 0))\n"
      "  )\n"
      ")\n";

  const auto items = ccad::listKiCadSymbolLibraryItems(content);
  require(items.size() == 2, "top-level item listing ignores nested unit symbols");
  require(items.at(0).name == "Parent", "first top-level symbol is listed");
  require(items.at(0).extends.empty(), "parent has no extends metadata");
  require(items.at(1).name == "Derived", "second top-level symbol is listed");
  require(items.at(1).extends == "Parent", "extends metadata is listed");
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <file.kicad_sym>\n";
    return 1;
  }
  try {
    std::string content = readFile(argv[1]);
    auto symbols = ccad::importKiCadSymbolLibrary(content);
    std::cout << "Imported " << symbols.size() << " symbols.\n";
    for (const auto& sym : symbols) {
      std::cout << "Symbol: " << sym.name << " (Pins: " << sym.pins.size() << ")\n";
    }
    testTopLevelSymbolLibraryItemListing();
    testLocalSymbolInheritance();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
