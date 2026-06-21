#include "ccad_core/board_design_settings.hpp"

#include <cmath>
#include <sstream>

namespace ccad {
namespace {

std::string rangeMessage(const double min_mm, const double max_mm) {
  std::ostringstream out;
  out << "Value must be between " << min_mm << " mm and " << max_mm << " mm";
  return out.str();
}

void addLengthRangeError(std::vector<DesignRuleValidationError>& errors,
                         const std::string& field, const std::string& code,
                         const Length& value, const double min_mm,
                         const double max_mm) {
  const std::int64_t min_nm = millimeters(min_mm).nanometers;
  const std::int64_t max_nm = millimeters(max_mm).nanometers;
  if (value.nanometers < min_nm || value.nanometers > max_nm) {
    errors.push_back(DesignRuleValidationError{
        .field = field,
        .code = code,
        .message = rangeMessage(min_mm, max_mm),
    });
  }
}

void addRatioRangeError(std::vector<DesignRuleValidationError>& errors,
                        const std::string& field, const std::string& code,
                        const double value, const double min_value,
                        const double max_value) {
  if (!std::isfinite(value) || value < min_value || value > max_value) {
    std::ostringstream message;
    message << "Value must be between " << min_value << " and " << max_value;
    errors.push_back(DesignRuleValidationError{
        .field = field,
        .code = code,
        .message = message.str(),
    });
  }
}

}  // namespace

std::vector<DesignRuleValidationError> validateDesignRules(const DesignRules& rules) {
  std::vector<DesignRuleValidationError> errors;

  addLengthRangeError(errors, "min_clearance", "INVALID_COPPER_CLEARANCE",
                      rules.copper_clearance, 0.0, 25.0);
  addLengthRangeError(errors, "min_connection", "INVALID_MIN_CONNECTION",
                      rules.min_connection, 0.0, 100.0);
  addLengthRangeError(errors, "min_track_width", "INVALID_MIN_TRACK_WIDTH",
                      rules.min_track_width, 0.0, 25.0);
  addLengthRangeError(errors, "min_via_annular_width", "INVALID_MIN_VIA_ANNULAR_RING",
                      rules.min_via_annular_ring, 0.0, 25.0);
  addLengthRangeError(errors, "min_via_diameter", "INVALID_MIN_VIA_DIAMETER",
                      rules.min_via_diameter, 0.0, 25.0);
  addLengthRangeError(errors, "min_through_hole_diameter",
                      "INVALID_MIN_THROUGH_HOLE_DRILL", rules.min_through_hole_drill,
                      0.0, 25.0);
  addLengthRangeError(errors, "min_microvia_diameter",
                      "INVALID_MIN_MICROVIA_DIAMETER", rules.min_microvia_diameter,
                      0.0, 10.0);
  addLengthRangeError(errors, "min_microvia_drill", "INVALID_MIN_MICROVIA_DRILL",
                      rules.min_microvia_drill, 0.0, 10.0);
  addLengthRangeError(errors, "min_hole_to_hole", "INVALID_MIN_HOLE_TO_HOLE",
                      rules.min_hole_to_hole, 0.0, 10.0);
  addLengthRangeError(errors, "min_hole_clearance", "INVALID_HOLE_CLEARANCE",
                      rules.hole_clearance, 0.0, 100.0);
  addLengthRangeError(errors, "min_silk_clearance", "INVALID_SILK_CLEARANCE",
                      rules.silk_clearance, -10.0, 100.0);
  addLengthRangeError(errors, "min_groove_width", "INVALID_MIN_GROOVE_WIDTH",
                      rules.min_groove_width, 0.0, 25.0);
  addLengthRangeError(errors, "min_copper_edge_clearance",
                      "INVALID_COPPER_EDGE_CLEARANCE", rules.copper_edge_clearance,
                      -0.01, 25.0);
  addLengthRangeError(errors, "solder_mask_expansion", "INVALID_SOLDER_MASK_EXPANSION",
                      rules.solder_mask_expansion, -25.0, 25.0);
  addLengthRangeError(errors, "solder_mask_min_width", "INVALID_SOLDER_MASK_MIN_WIDTH",
                      rules.solder_mask_min_width, 0.0, 25.0);
  addLengthRangeError(errors, "solder_mask_to_copper_clearance",
                      "INVALID_SOLDER_MASK_TO_COPPER_CLEARANCE",
                      rules.solder_mask_to_copper_clearance, 0.0, 25.0);
  addLengthRangeError(errors, "solder_paste_margin", "INVALID_SOLDER_PASTE_MARGIN",
                      rules.solder_paste_margin, -25.0, 25.0);
  addRatioRangeError(errors, "solder_paste_margin_ratio",
                     "INVALID_SOLDER_PASTE_MARGIN_RATIO",
                     rules.solder_paste_margin_ratio, -1.0, 1.0);

  if (rules.board_thickness.nanometers <= 0) {
    errors.push_back(DesignRuleValidationError{
        .field = "board_thickness",
        .code = "INVALID_BOARD_THICKNESS",
        .message = "Board thickness must be positive",
    });
  }

  return errors;
}

}  // namespace ccad
