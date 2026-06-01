#include "ccad_core/kicad_footprint_export.hpp"

#include <iomanip>
#include <sstream>

namespace ccad {

std::string exportKiCadFootprint(const Footprint& footprint) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "(footprint \"" << footprint.name << "\"\n";

  for (const FootprintPad& pad : footprint.pads) {
    double px = pad.position.x.nanometers / 1000000.0;
    double py = pad.position.y.nanometers / 1000000.0;
    double pw = pad.size.width.nanometers / 1000000.0;
    double ph = pad.size.height.nanometers / 1000000.0;

    out << "  (pad \"" << pad.number << "\" " << pad.type << " " << pad.shape 
        << " (at " << px << " " << py;
    if (pad.rotation_degrees != 0.0) {
      out << " " << pad.rotation_degrees;
    }
    out << ") (size " << pw << " " << ph << ")";

    if (pad.drill.has_value()) {
      out << " (drill " << (pad.drill->nanometers / 1000000.0) << ")";
    }

    if (pad.roundrect_rratio.has_value()) {
      out << " (roundrect_rratio " << *pad.roundrect_rratio << ")";
    }

    if (pad.chamfer_ratio.has_value()) {
      out << " (chamfer_ratio " << *pad.chamfer_ratio << ")";
    }

    if (!pad.layers.empty()) {
      out << " (layers";
      for (const std::string& layer : pad.layers) {
        out << " \"" << layer << "\"";
      }
      out << ")";
    }

    out << ")\n";
  }

  out << ")\n";
  return out.str();
}

}  // namespace ccad
