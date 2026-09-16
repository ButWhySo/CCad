#include "board_design_settings.hpp"
#include "model.hpp"

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
namespace ccad {

void BoardDesignSettings::setDefaultTrackWidth(double width) {
    defaultTrackWidth_ = width;
}

double BoardDesignSettings::getDefaultTrackWidth() const {
    return defaultTrackWidth_;
}

void BoardDesignSettings::setDefaultViaSize(double diameter, double drill) {
    defaultViaDiameter_ = diameter;
    defaultViaDrill_ = drill;
}

double BoardDesignSettings::getDefaultViaDiameter() const {
    return defaultViaDiameter_;
}

double BoardDesignSettings::getDefaultViaDrill() const {
    return defaultViaDrill_;
}

void BoardDesignSettings::setDefaultClearance(double clearance) {
    defaultClearance_ = clearance;
}

double BoardDesignSettings::getDefaultClearance() const {
    return defaultClearance_;
}

std::vector<DesignRuleValidationError> validateDesignRules(const DesignRules& rules) {
    std::vector<DesignRuleValidationError> errors;
    auto checkRange = [&](Length value, const char* code, const char* field,
                          double minimum, double maximum) {
        if (value.nanometers < millimeters(minimum).nanometers ||
            value.nanometers > millimeters(maximum).nanometers)
            errors.push_back({code, field, std::string(field) + " must be between " +
                std::to_string(minimum) + " and " + std::to_string(maximum) + " mm"});
    };
    checkRange(rules.copper_clearance, "INVALID_COPPER_CLEARANCE", "min_clearance", 0, 25);
    checkRange(rules.min_track_width, "INVALID_MIN_TRACK_WIDTH", "min_track_width", 0, 25);
    checkRange(rules.max_track_width, "INVALID_MAX_TRACK_WIDTH", "max_track_width", 0, 25);
    checkRange(rules.min_via_annular_ring, "INVALID_MIN_VIA_ANNULAR_RING", "min_via_annular_width", 0, 25);
    checkRange(rules.min_connection, "INVALID_MIN_CONNECTION", "min_connection", 0, 100);
    checkRange(rules.min_via_diameter, "INVALID_MIN_VIA_DIAMETER", "min_via_diameter", 0, 25);
    checkRange(rules.max_via_diameter, "INVALID_MAX_VIA_DIAMETER", "max_via_diameter", 0, 25);
    checkRange(rules.min_through_hole_drill, "INVALID_MIN_THROUGH_HOLE_DRILL", "min_through_hole_diameter", 0, 25);
    checkRange(rules.min_microvia_diameter, "INVALID_MIN_MICROVIA_DIAMETER", "min_microvia_diameter", 0, 10);
    checkRange(rules.min_microvia_drill, "INVALID_MIN_MICROVIA_DRILL", "min_microvia_drill", 0, 10);
    checkRange(rules.min_hole_to_hole, "INVALID_MIN_HOLE_TO_HOLE", "min_hole_to_hole", 0, 10);
    checkRange(rules.hole_clearance, "INVALID_HOLE_CLEARANCE", "min_hole_clearance", 0, 100);
    checkRange(rules.copper_edge_clearance, "INVALID_COPPER_EDGE_CLEARANCE", "min_copper_edge_clearance", -0.01, 25);
    checkRange(rules.silk_clearance, "INVALID_SILK_CLEARANCE", "min_silk_clearance", -10, 100);
    checkRange(rules.min_text_height, "INVALID_MIN_TEXT_HEIGHT", "min_text_height", 0, 100);
    if (!std::isfinite(rules.min_track_angle_degrees) || rules.min_track_angle_degrees < 0 || rules.min_track_angle_degrees > 180)
      errors.push_back({"INVALID_MIN_TRACK_ANGLE", "min_track_angle_degrees", "must be finite in [0,180]"});
    if (!std::isfinite(rules.max_track_angle_degrees) || rules.max_track_angle_degrees < 0 || rules.max_track_angle_degrees > 180)
      errors.push_back({"INVALID_MAX_TRACK_ANGLE", "max_track_angle_degrees", "must be finite in [0,180]"});
    if (rules.max_track_angle_degrees > 0 && rules.min_track_angle_degrees > rules.max_track_angle_degrees)
      errors.push_back({"INVALID_TRACK_ANGLE_RANGE", "max_track_angle_degrees", "max must be zero or at least min"});
    checkRange(rules.min_track_segment_length, "INVALID_MIN_TRACK_SEGMENT_LENGTH", "min_track_segment_length", 0, 1000);
    checkRange(rules.max_track_segment_length, "INVALID_MAX_TRACK_SEGMENT_LENGTH", "max_track_segment_length", 0, 1000);
    if (rules.max_track_segment_length.nanometers > 0 && rules.min_track_segment_length.nanometers > rules.max_track_segment_length.nanometers)
      errors.push_back({"INVALID_TRACK_SEGMENT_LENGTH_RANGE", "max_track_segment_length", "max must be zero or at least min"});
    checkRange(rules.min_groove_width, "INVALID_MIN_GROOVE_WIDTH", "min_groove_width", 0, 25);
    checkRange(rules.solder_mask_expansion, "INVALID_SOLDER_MASK_EXPANSION", "solder_mask_expansion", -25, 25);
    checkRange(rules.solder_mask_min_width, "INVALID_SOLDER_MASK_MIN_WIDTH", "solder_mask_min_width", 0, 25);
    checkRange(rules.solder_mask_to_copper_clearance, "INVALID_SOLDER_MASK_TO_COPPER_CLEARANCE", "solder_mask_to_copper_clearance", 0, 25);
    checkRange(rules.solder_paste_margin, "INVALID_SOLDER_PASTE_MARGIN", "solder_paste_margin", -25, 25);
    if (rules.board_thickness.nanometers <= 0)
        errors.push_back({"INVALID_BOARD_THICKNESS", "board_thickness", "board_thickness must be positive"});
    if (!std::isfinite(rules.solder_paste_margin_ratio) || rules.solder_paste_margin_ratio < -1.0 || rules.solder_paste_margin_ratio > 1.0)
        errors.push_back({"INVALID_SOLDER_PASTE_MARGIN_RATIO", "solder_paste_margin_ratio",
                          "solder_paste_margin_ratio must be finite and in [-1, 1]"});
    if (rules.max_track_width.nanometers > 0 && rules.max_track_width.nanometers < rules.min_track_width.nanometers)
        errors.push_back({"INVALID_TRACK_WIDTH_RANGE", "max_track_width", "max_track_width must be zero or at least min_track_width"});
    if (rules.max_via_diameter.nanometers > 0 && rules.max_via_diameter.nanometers < rules.min_via_diameter.nanometers)
        errors.push_back({"INVALID_VIA_DIAMETER_RANGE", "max_via_diameter", "max_via_diameter must be zero or at least min_via_diameter"});
    return errors;
}

} // namespace ccad
