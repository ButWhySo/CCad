#include "ccad_core/cleanup_item.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_cleanup_action_catalog_matches_kicad_text() {
  const std::vector<ccad::CleanupActionInfo> actions = ccad::cleanupActionCatalog();

  require(actions.size() == 13, "cleanup catalog exposes all KiCad cleanup item codes");
  require(actions.front().code == ccad::CleanupActionCode::shorting_track,
          "first cleanup action is shorting track");
  require(actions.front().id == "shorting_track", "first cleanup action id is stable");
  require(actions.front().title == "Remove track shorting two nets",
          "shorting track title matches KiCad");
  require(actions.front().domain == "tracks_and_vias",
          "shorting track action is in tracks and vias domain");

  const ccad::CleanupActionInfo* redundant_via =
      ccad::findCleanupActionInfo(ccad::CleanupActionCode::redundant_via);
  require(redundant_via != nullptr, "finds redundant via action");
  require(redundant_via->title == "Remove redundant via",
          "redundant via title matches KiCad");

  const ccad::CleanupActionInfo* lines_to_rect =
      ccad::findCleanupActionInfo(ccad::CleanupActionCode::lines_to_rect);
  require(lines_to_rect != nullptr, "finds lines to rectangle action");
  require(lines_to_rect->domain == "graphics", "lines to rectangle is graphics cleanup");
  require(lines_to_rect->title == "Convert lines to rectangle",
          "lines to rectangle title matches KiCad");

  require(ccad::cleanupActionTitle(ccad::CleanupActionCode::merge_pad) ==
              "Merge overlapping shapes into pad",
          "merge pad title matches KiCad");
}

void test_cleanup_action_provider_supports_indexed_rows_and_deep_delete() {
  std::vector<ccad::CleanupActionInfo> rows = {
      *ccad::findCleanupActionInfo(ccad::CleanupActionCode::duplicate_track),
      *ccad::findCleanupActionInfo(ccad::CleanupActionCode::zero_length_track),
      *ccad::findCleanupActionInfo(ccad::CleanupActionCode::duplicate_graphic),
  };

  ccad::CleanupActionProvider provider(rows);
  require(provider.count() == 3, "provider reports source row count");
  require(provider.item(0).id == "duplicate_track", "provider returns indexed row");
  require(provider.item(1).title == "Remove zero-length track",
          "provider returns indexed zero-length row");

  provider.deleteItem(1, false);
  require(provider.count() == 3, "non-deep delete keeps provider row");

  provider.deleteItem(1, true);
  require(provider.count() == 2, "deep delete removes provider row");
  require(provider.item(1).id == "duplicate_graphic",
          "deep delete shifts remaining rows like KiCad vector provider");

  bool bounds_checked = false;
  try {
    (void) provider.item(8);
  } catch (const std::out_of_range&) {
    bounds_checked = true;
  }
  require(bounds_checked, "provider row lookup is bounds checked");
}

}  // namespace

int main() {
  try {
    test_cleanup_action_catalog_matches_kicad_text();
    test_cleanup_action_provider_supports_indexed_rows_and_deep_delete();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
