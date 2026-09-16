#include "ccad_core/board_design_settings.hpp"

#include "ccad_core/drc.hpp"
#include "ccad_core/model.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

bool hasField(const std::vector<ccad::DesignRuleValidationError>& errors,
              const std::string& field) {
  for (const ccad::DesignRuleValidationError& error : errors) {
    if (error.field == field) {
      return true;
    }
  }
  return false;
}

bool hasCode(const std::vector<ccad::Diagnostic>& diagnostics, const std::string& code) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code) {
      return true;
    }
  }
  return false;
}

ccad::Project validBoardProject() {
  ccad::Board board{
      .outline =
          ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
                     .size = ccad::Size{.width = ccad::millimeters(40),
                                        .height = ccad::millimeters(30)}},
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper"},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper"}},
  };
  return ccad::Project{.id = "rules", .name = "Rules", .boards = {board}};
}

}  // namespace

int main() {
  ccad::DesignRules zero_allowed = ccad::DesignRules{};
  zero_allowed.copper_clearance = ccad::nanometers(0);
  zero_allowed.min_track_width = ccad::nanometers(0);
  zero_allowed.min_via_annular_ring = ccad::nanometers(0);
  zero_allowed.solder_mask_expansion = ccad::millimeters(-0.01);
  zero_allowed.solder_paste_margin_ratio = -1.0;
  require(ccad::validateDesignRules(zero_allowed).empty(),
          "KiCad-style board rules allow zero minima and small negative mask expansion");

  auto nonfinite = zero_allowed;
  nonfinite.solder_paste_margin_ratio = std::numeric_limits<double>::quiet_NaN();
  require(hasField(ccad::validateDesignRules(nonfinite), "solder_paste_margin_ratio"),
          "reject nonfinite paste ratio");

  ccad::DesignRules out_of_range = ccad::DesignRules{};
  out_of_range.copper_clearance = ccad::millimeters(25.01);
  out_of_range.min_connection = ccad::millimeters(100.01);
  out_of_range.min_track_width = ccad::millimeters(25.01);
  out_of_range.min_via_annular_ring = ccad::millimeters(25.01);
  out_of_range.min_via_diameter = ccad::millimeters(25.01);
  out_of_range.min_through_hole_drill = ccad::millimeters(25.01);
  out_of_range.min_microvia_diameter = ccad::millimeters(10.01);
  out_of_range.min_microvia_drill = ccad::millimeters(10.01);
  out_of_range.min_hole_to_hole = ccad::millimeters(10.01);
  out_of_range.hole_clearance = ccad::millimeters(100.01);
  out_of_range.silk_clearance = ccad::millimeters(100.01);
  out_of_range.min_groove_width = ccad::millimeters(25.01);
  out_of_range.solder_mask_expansion = ccad::millimeters(-25.01);
  out_of_range.solder_mask_min_width = ccad::millimeters(25.01);
  out_of_range.solder_mask_to_copper_clearance = ccad::millimeters(25.01);
  out_of_range.solder_paste_margin = ccad::millimeters(25.01);
  out_of_range.solder_paste_margin_ratio = 1.01;
  out_of_range.board_thickness = ccad::nanometers(0);

  const std::vector<ccad::DesignRuleValidationError> errors =
      ccad::validateDesignRules(out_of_range);
  require(hasField(errors, "min_clearance"), "validates KiCad min_clearance range");
  require(hasField(errors, "min_connection"), "validates KiCad min_connection range");
  require(hasField(errors, "min_track_width"), "validates KiCad min_track_width range");
  require(hasField(errors, "min_via_annular_width"),
          "validates KiCad min_via_annular_width range");
  require(hasField(errors, "min_via_diameter"), "validates KiCad min_via_diameter range");
  require(hasField(errors, "min_through_hole_diameter"),
          "validates KiCad min_through_hole_diameter range");
  require(hasField(errors, "min_microvia_diameter"),
          "validates KiCad min_microvia_diameter range");
  require(hasField(errors, "min_microvia_drill"), "validates KiCad min_microvia_drill range");
  require(hasField(errors, "min_hole_to_hole"), "validates KiCad min_hole_to_hole range");
  require(hasField(errors, "min_hole_clearance"), "validates KiCad min_hole_clearance range");
  require(hasField(errors, "min_silk_clearance"), "validates KiCad min_silk_clearance range");
  require(hasField(errors, "min_groove_width"), "validates KiCad min_groove_width range");
  require(hasField(errors, "solder_mask_expansion"),
          "validates KiCad solder_mask_expansion range");
  require(hasField(errors, "solder_mask_min_width"),
          "validates KiCad solder_mask_min_width range");
  require(hasField(errors, "solder_mask_to_copper_clearance"),
          "validates KiCad solder_mask_to_copper_clearance range");
  require(hasField(errors, "solder_paste_margin"),
          "validates KiCad solder_paste_margin range");
  require(hasField(errors, "solder_paste_margin_ratio"),
          "validates KiCad solder_paste_margin_ratio range");
  require(hasField(errors, "board_thickness"), "validates positive board thickness");

  ccad::Project zero_clearance_project = validBoardProject();
  zero_clearance_project.boards[0].design_rules.copper_clearance = ccad::nanometers(0);
  require(!hasCode(ccad::runDrc(zero_clearance_project), "INVALID_COPPER_CLEARANCE"),
          "DRC accepts KiCad-compatible zero minimum copper clearance");

  ccad::Project excessive_track_width = validBoardProject();
  excessive_track_width.boards[0].design_rules.min_track_width = ccad::millimeters(25.01);
  require(hasCode(ccad::runDrc(excessive_track_width), "INVALID_MIN_TRACK_WIDTH"),
          "DRC reports KiCad board design setting range errors");

  return 0;
}
