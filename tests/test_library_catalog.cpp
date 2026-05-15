#include "ccad_core/library_catalog.hpp"
#include "test_support.hpp"

#include <filesystem>
#include <fstream>
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

  std::vector<ccad::CatalogDiagnostic> clean_diagnostics = ccad::validateLibraryCatalog(parsed);
  require(clean_diagnostics.empty(), "catalog validator accepts complete unique catalog");

  parsed.items.push_back(ccad::LibraryItem{
      .id = "footprint:Resistor_SMD:R_0603_1608Metric",
      .kind = "footprint",
      .name = "duplicate resistor",
      .source_path = "Resistor_SMD.pretty/R_0603_1608Metric.kicad_mod",
      .native_path = "footprints/Resistor_SMD/R_0603_1608Metric.copy.ccad-footprint.json",
      .sha256 = "",
      .license = "",
      .provenance = "",
  });
  const std::vector<ccad::CatalogDiagnostic> dirty_diagnostics =
      ccad::validateLibraryCatalog(parsed);
  require(dirty_diagnostics.size() == 4, "catalog validator reports duplicate and empty fields");
  require(dirty_diagnostics.at(0).code == "DUPLICATE_ITEM_ID",
          "catalog validator reports duplicate id first");
  require(dirty_diagnostics.at(1).code == "MISSING_ITEM_SHA256",
          "catalog validator reports missing checksum");
  require(dirty_diagnostics.at(2).code == "MISSING_ITEM_LICENSE",
          "catalog validator reports missing license");
  require(dirty_diagnostics.at(3).code == "MISSING_ITEM_PROVENANCE",
          "catalog validator reports missing provenance");

  const std::filesystem::path temp = std::filesystem::temp_directory_path() / "ccad_catalog_test";
  std::filesystem::create_directories(temp / "footprints" / "Resistor_SMD");
  std::ofstream native_item(temp / "footprints" / "Resistor_SMD" /
                                "R_0603_1608Metric.ccad-footprint.json",
                            std::ios::binary);
  native_item << "hello\n";
  native_item.close();

  ccad::LibraryCatalog file_catalog;
  file_catalog.schema_version = 1;
  file_catalog.name = "file-cache";
  file_catalog.source = catalog.source;
  file_catalog.items.push_back(ccad::LibraryItem{
      .id = "footprint:Resistor_SMD:R_0603_1608Metric",
      .kind = "footprint",
      .name = "R_0603_1608Metric",
      .source_path = "Resistor_SMD.pretty/R_0603_1608Metric.kicad_mod",
      .native_path = "footprints/Resistor_SMD/R_0603_1608Metric.ccad-footprint.json",
      .sha256 = "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03",
      .license = "CC-BY-SA-4.0 WITH KiCad-library-exception",
      .provenance = "kicad-official@abc123",
  });
  require(ccad::validateLibraryCatalog(file_catalog, temp).empty(),
          "catalog file validator accepts matching local artifact checksum");

  file_catalog.items.at(0).sha256 = "0000000000000000000000000000000000000000000000000000000000000000";
  const std::vector<ccad::CatalogDiagnostic> checksum_diagnostics =
      ccad::validateLibraryCatalog(file_catalog, temp);
  require(checksum_diagnostics.size() == 1, "catalog file validator reports checksum mismatch");
  require(checksum_diagnostics.at(0).code == "ITEM_SHA256_MISMATCH",
          "catalog file validator uses checksum mismatch code");

  file_catalog.items.at(0).native_path = "footprints/missing.ccad-footprint.json";
  const std::vector<ccad::CatalogDiagnostic> missing_file_diagnostics =
      ccad::validateLibraryCatalog(file_catalog, temp);
  require(missing_file_diagnostics.size() == 1, "catalog file validator reports missing file");
  require(missing_file_diagnostics.at(0).code == "MISSING_NATIVE_FILE",
          "catalog file validator uses missing file code");

  bool rejected = false;
  try {
    (void)ccad::loadLibraryCatalogJson("{\"schema_version\": 1, \"name\": \"bad\"}");
  } catch (const std::exception&) {
    rejected = true;
  }
  require(rejected, "catalog rejects missing source/items");
}
