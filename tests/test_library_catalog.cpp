#include "ccad_core/library_catalog.hpp"
#include "test_support.hpp"

#include <string>

int main() {
  ccad::LibraryCatalog catalog;
  catalog.schema_version = 1;
  catalog.name = "local-kicad-cache";
  catalog.source = ccad::LibrarySource{
      .name = "kicad-official",
      .kind = "kicad",
      .url = "https://gitlab.com/kicad/libraries/kicad-footprints",
      .commit = "abc123",
      .mirror = "official",
      .fetched_at = "2026-05-15T00:00:00Z",
  };
  catalog.items.push_back(ccad::LibraryItem{
      .id = "footprint:Resistor_SMD:R_0603_1608Metric",
      .kind = "footprint",
      .name = "R_0603_1608Metric",
      .source_path = "Resistor_SMD.pretty/R_0603_1608Metric.kicad_mod",
      .native_path = "footprints/Resistor_SMD/R_0603_1608Metric.ccad-footprint.json",
      .sha256 = "0123456789abcdef",
      .license = "CC-BY-SA-4.0 WITH KiCad-library-exception",
      .provenance = "kicad-official@abc123",
      .warnings = {"unsupported 3d model reference skipped"},
  });

  const std::string json = ccad::dumpLibraryCatalogJson(catalog);
  require(json.find("\"name\": \"local-kicad-cache\"") != std::string::npos,
          "catalog json writes name");
  require(json.find("\"sha256\": \"0123456789abcdef\"") != std::string::npos,
          "catalog json writes checksum");
  require(json.find("\"provenance\": \"kicad-official@abc123\"") != std::string::npos,
          "catalog json writes provenance");
  require(json.find("unsupported 3d model reference skipped") != std::string::npos,
          "catalog json writes warnings");

  ccad::LibraryCatalog parsed = ccad::loadLibraryCatalogJson(json);
  require(parsed.schema_version == 1, "catalog round trips schema version");
  require(parsed.source.name == "kicad-official", "catalog round trips source name");
  require(parsed.source.commit == "abc123", "catalog round trips source commit");
  require(parsed.items.size() == 1, "catalog round trips item count");
  require(parsed.items.at(0).id == "footprint:Resistor_SMD:R_0603_1608Metric",
          "catalog round trips item id");
  require(parsed.items.at(0).license == "CC-BY-SA-4.0 WITH KiCad-library-exception",
          "catalog round trips license");
  require(parsed.items.at(0).warnings.size() == 1, "catalog round trips warnings");

  const ccad::LibraryItem* found =
      ccad::findLibraryItem(parsed, "footprint:Resistor_SMD:R_0603_1608Metric");
  require(found != nullptr, "catalog finds item by id");
  require(found->native_path == "footprints/Resistor_SMD/R_0603_1608Metric.ccad-footprint.json",
          "catalog found item has native path");
  require(ccad::findLibraryItem(parsed, "missing") == nullptr,
          "catalog missing item returns null");

  parsed.items.push_back(ccad::LibraryItem{
      .id = "footprint:Capacitor_SMD:C_0603_1608Metric",
      .kind = "footprint",
      .name = "C_0603_1608Metric",
      .source_path = "Capacitor_SMD.pretty/C_0603_1608Metric.kicad_mod",
      .native_path = "footprints/Capacitor_SMD/C_0603_1608Metric.ccad-footprint.json",
      .sha256 = "fedcba9876543210",
      .license = "CC-BY-SA-4.0 WITH KiCad-library-exception",
      .provenance = "kicad-official@abc123",
  });
  const std::vector<const ccad::LibraryItem*> resistor_matches =
      ccad::searchLibraryItems(parsed, "resistor");
  require(resistor_matches.size() == 1, "catalog search matches source path case-insensitively");
  require(resistor_matches.at(0)->id == "footprint:Resistor_SMD:R_0603_1608Metric",
          "catalog search returns matching item");
  const std::vector<const ccad::LibraryItem*> metric_matches =
      ccad::searchLibraryItems(parsed, "0603");
  require(metric_matches.size() == 2, "catalog search returns multiple matches");
  const std::vector<const ccad::LibraryItem*> capacitor_matches =
      ccad::searchLibraryItems(parsed, "0603", "footprint");
  require(capacitor_matches.size() == 2, "catalog search kind filter keeps footprints");
  require(ccad::searchLibraryItems(parsed, "0603", "symbol").empty(),
          "catalog search kind filter removes non-matching kind");
  require(ccad::searchLibraryItems(parsed, "").empty(), "catalog search rejects empty query");

  bool rejected = false;
  try {
    (void)ccad::loadLibraryCatalogJson("{\"schema_version\": 1, \"name\": \"bad\"}");
  } catch (const std::exception&) {
    rejected = true;
  }
  require(rejected, "catalog rejects missing source/items");
}
