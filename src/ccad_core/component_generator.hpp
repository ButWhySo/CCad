#pragma once

#include "ccad_core/footprint.hpp"
#include "ccad_core/symbol.hpp"

#include <string>

namespace ccad {

struct FootprintParams {
  std::string name;
  std::string package_type; // e.g. "SOP", "DIP", "QFN"
  int pin_count = 8;
  double pitch_mm = 1.27;
  double span_mm = 6.0;
  double pad_width_mm = 0.6;
  double pad_length_mm = 1.5;
};

struct SymbolParams {
  std::string name;
  std::string ref_des = "U";
  int pin_count = 8;
};

Footprint generateParametricFootprint(const FootprintParams& params);
Symbol generateParametricSymbol(const SymbolParams& params);

} // namespace ccad
