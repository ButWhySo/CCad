#pragma once

#include "ccad_core/symbol.hpp"

#include <string>
#include <vector>

namespace ccad {

// Parses a KiCad symbol library file and returns all symbols found in it.
std::vector<Symbol> importKiCadSymbolLibrary(const std::string& kicad_sym_content);

// Dumps a list of symbols to JSON.
std::string dumpSymbolsJson(const std::vector<Symbol>& symbols);

}  // namespace ccad
