$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "build-qt"

# Explicit allow-list: GUI/visual/provider tests must never enter this fast gate.
$names = @(
  "custom_drc_rules", "drc_test_provider_clearance", "drc_test_provider_courtyard",
  "drc_test_provider_edge_clearance", "drc_test_provider_unrouted", "net_tie_drc",
  "router_tool", "router_width_clearance", "nearest_neighbor_connectivity",
  "dynamic_ratnest_graph", "schematic_collector", "lib_symbol", "placement",
  "layers", "pad_number_provider", "autoplacer", "autorouter_matrix",
  "spread_footprints", "erc", "fix_board_shape", "drc", "board_design_settings",
  "board_loader", "board_item_container", "board_collector", "board_outline_polygon",
  "board_statistics", "board_text_var_adapter", "board_stackup", "cleanup_item",
  "kicad_footprint_import", "kicad_symbol_import", "library_catalog",
  "pns_board_adapter", "router_pns_adapter"
)

$forbidden = $names | Where-Object {
  $_ -match '^(gui_|visual_|provider_|agent_)'
}
if ($forbidden) {
  throw "native gate allow-list contains forbidden test(s): $($forbidden -join ', ')"
}

foreach ($name in $names) {
  & ctest --test-dir $build --output-on-failure -R "^$([regex]::Escape($name))$"
  if ($LASTEXITCODE -ne 0) { throw "native core test failed: $name" }
}
Write-Output "PASS explicit native core gate: $($names.Count) tests; GUI/visual/provider excluded"
