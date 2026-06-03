#pragma once

#include "ccad_core/symbol.hpp"

#include <string>
#include <vector>
#include <filesystem>

namespace ccad {

struct KiCadSymbolLibraryItem {
  std::string name;
  std::string extends;
};

// Lists top-level symbols in a KiCad symbol library without expanding all
// graphics into CCad symbol objects. Nested unit symbols are not returned.
std::vector<KiCadSymbolLibraryItem> listKiCadSymbolLibraryItems(const std::string& kicad_sym_content);

// Parses a KiCad symbol library file and returns all symbols found in it.
std::vector<Symbol> importKiCadSymbolLibrary(const std::string& kicad_sym_content);

// Dumps a list of symbols to JSON.
std::string dumpSymbolsJson(const std::vector<Symbol>& symbols);

// Loads a single symbol from JSON.
Symbol loadSymbolJson(std::string_view source);

// Loads a converted symbol JSON file and resolves a same-directory `extends`
// parent when the child relies on inherited KiCad symbol pins or graphics.
Symbol loadSymbolJsonFileWithLocalInheritance(const std::filesystem::path& path);

}  // namespace ccad
