#include "ccad_core/kicad_symbol_import.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) throw std::runtime_error("Could not open " + path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
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
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
