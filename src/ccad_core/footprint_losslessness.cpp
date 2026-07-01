#include "ccad_core/footprint_losslessness.hpp"

#include <cmath>
#include <algorithm>

namespace ccad {
namespace {

bool approxEqual(double a, double b, double epsilon = 1e-5) {
  return std::abs(a - b) < epsilon;
}

bool approxEqual(Length a, Length b) {
  return a.nanometers == b.nanometers;
}

bool approxEqual(Point a, Point b) {
  return approxEqual(a.x, b.x) && approxEqual(a.y, b.y);
}

bool approxEqual(Size a, Size b) {
  return approxEqual(a.width, b.width) && approxEqual(a.height, b.height);
}

} // namespace

std::vector<LosslessnessDiagnostic> verifyFootprintLosslessness(
    const Footprint& original,
    const Footprint& candidate) {
  std::vector<LosslessnessDiagnostic> diagnostics;

  if (original.name != candidate.name) {
    diagnostics.push_back({"error", "name", "Footprint name mismatch: '" + original.name + "' vs '" + candidate.name + "'"});
  }

  if (original.exclude_from_bom != candidate.exclude_from_bom) {
    diagnostics.push_back({"warning", "exclude_from_bom", "BOM exclusion mismatch"});
  }

  // Compare pads
  if (original.pads.size() != candidate.pads.size()) {
    diagnostics.push_back({"error", "pads", "Pad count mismatch: " + std::to_string(original.pads.size()) + " vs " + std::to_string(candidate.pads.size())});
  } else {
    for (std::size_t i = 0; i < original.pads.size(); ++i) {
      const auto& p1 = original.pads.at(i);
      const auto& p2 = candidate.pads.at(i);
      const std::string pad_ref = "Pad[" + p1.number + "]";

      if (p1.number != p2.number) {
        diagnostics.push_back({"error", "pad.number", pad_ref + " number mismatch: '" + p1.number + "' vs '" + p2.number + "'"});
      }
      if (p1.type != p2.type) {
        diagnostics.push_back({"error", "pad.type", pad_ref + " type mismatch: '" + p1.type + "' vs '" + p2.type + "'"});
      }
      if (p1.shape != p2.shape) {
        diagnostics.push_back({"error", "pad.shape", pad_ref + " shape mismatch: '" + p1.shape + "' vs '" + p2.shape + "'"});
      }
      if (!approxEqual(p1.position, p2.position)) {
        diagnostics.push_back({"error", "pad.position", pad_ref + " position mismatch"});
      }
      if (!approxEqual(p1.rotation_degrees, p2.rotation_degrees)) {
        diagnostics.push_back({"error", "pad.rotation", pad_ref + " rotation mismatch"});
      }
      if (!approxEqual(p1.size, p2.size)) {
        diagnostics.push_back({"error", "pad.size", pad_ref + " size mismatch"});
      }
      if (p1.drill.has_value() != p2.drill.has_value()) {
        diagnostics.push_back({"error", "pad.drill", pad_ref + " drill presence mismatch"});
      } else if (p1.drill.has_value() && !approxEqual(*p1.drill, *p2.drill)) {
        diagnostics.push_back({"error", "pad.drill", pad_ref + " drill size mismatch"});
      }
      if (p1.drill_height.has_value() != p2.drill_height.has_value()) {
        diagnostics.push_back({"error", "pad.drill_height", pad_ref + " drill_height presence mismatch"});
      } else if (p1.drill_height.has_value() && !approxEqual(*p1.drill_height, *p2.drill_height)) {
        diagnostics.push_back({"error", "pad.drill_height", pad_ref + " drill_height mismatch"});
      }
      if (p1.drill_shape != p2.drill_shape) {
        diagnostics.push_back({"error", "pad.drill_shape", pad_ref + " drill_shape mismatch"});
      }
      if (p1.layers != p2.layers) {
        diagnostics.push_back({"error", "pad.layers", pad_ref + " layers mismatch"});
      }
      if (p1.roundrect_rratio.has_value() != p2.roundrect_rratio.has_value()) {
        diagnostics.push_back({"error", "pad.roundrect_rratio", pad_ref + " roundrect_rratio presence mismatch"});
      } else if (p1.roundrect_rratio.has_value() && !approxEqual(*p1.roundrect_rratio, *p2.roundrect_rratio)) {
        diagnostics.push_back({"error", "pad.roundrect_rratio", pad_ref + " roundrect_rratio value mismatch"});
      }
      if (p1.chamfer_ratio.has_value() != p2.chamfer_ratio.has_value()) {
        diagnostics.push_back({"error", "pad.chamfer_ratio", pad_ref + " chamfer_ratio presence mismatch"});
      } else if (p1.chamfer_ratio.has_value() && !approxEqual(*p1.chamfer_ratio, *p2.chamfer_ratio)) {
        diagnostics.push_back({"error", "pad.chamfer_ratio", pad_ref + " chamfer_ratio value mismatch"});
      }
    }
  }

  // Compare graphical primitives
  if (original.lines.size() != candidate.lines.size()) {
    diagnostics.push_back({"warning", "lines", "Line count mismatch"});
  }
  if (original.arcs.size() != candidate.arcs.size()) {
    diagnostics.push_back({"warning", "arcs", "Arc count mismatch"});
  }
  if (original.circles.size() != candidate.circles.size()) {
    diagnostics.push_back({"warning", "circles", "Circle count mismatch"});
  }
  if (original.texts.size() != candidate.texts.size()) {
    diagnostics.push_back({"warning", "texts", "Text count mismatch"});
  }
  if (original.models.size() != candidate.models.size()) {
    diagnostics.push_back({"warning", "models", "Model count mismatch"});
  }

  return diagnostics;
}

} // namespace ccad
