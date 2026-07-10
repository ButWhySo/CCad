cmake_minimum_required(VERSION 3.20)

project(CCad VERSION 0.1.0 LANGUAGES CXX)

option(CCAD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(CCAD_BUILD_GUI "Build Qt review GUI when Qt 6 Widgets is available" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_definitions(_USE_MATH_DEFINES)
endif()

add_library(ccad_core
  src/ccad_core/annotate.cpp
  src/ccad_core/autoplace_fields.cpp
  src/ccad_core/junction_helpers.cpp
  src/ccad_core/gfx_import_utils.cpp
  src/ccad_core/sch_connection.cpp
  src/ccad_core/sch_netchain.cpp
  src/ccad_core/net_navigator.cpp
  src/ccad_core/am_param.cpp
  src/ccad_core/am_primitive.cpp
  src/ccad_core/aperture_macro.cpp
  src/ccad_core/agent_policy.cpp
  src/ccad_core/autorouter_matrix.cpp
  src/ccad_core/autoplacer.cpp
  src/ccad_core/board_collector.cpp
  src/ccad_core/board_outline_polygon.cpp
  src/ccad_core/board_item_container.cpp
  src/ccad_core/board_item.cpp
  src/ccad_core/board_loader.cpp
  src/ccad_core/board_stackup.cpp
  src/ccad_core/board_statistics.cpp
  src/ccad_core/board_text_var_adapter.cpp
  src/ccad_core/board_design_settings.cpp
  src/ccad_core/canvas.cpp
  src/ccad_core/cleanup_item.cpp
  src/ccad_core/third_party/qrcodegen/qrcodegen.cpp
  src/ccad_core/model.cpp
  src/ccad_core/component_generator.cpp
  src/ccad_core/diff.cpp
  src/ccad_core/3d_cache.cpp
  src/ccad_core/3d_fastmath.cpp
  src/ccad_core/3d_math.cpp
  src/ccad_core/3d_resolver.cpp
  src/ccad_core/3d_material.cpp
  src/ccad_core/3d_raytracer.cpp
  src/ccad_core/3d_scene_graph.cpp
  src/ccad_core/drc_engine.cpp
  src/ccad_core/drc.cpp
  src/ccad_core/erc.cpp
  src/ccad_core/geometry.cpp
  src/ccad_core/gfx_import_utils.cpp
  src/ccad_core/json.cpp
  src/ccad_core/kicad_footprint_import.cpp
  src/ccad_core/aperture_macro.cpp
  src/ccad_core/dcode.cpp
  src/ccad_core/am_evaluate.cpp
  src/ccad_core/excellon_read_drill.cpp
  src/ccad_core/gbr_layout.cpp
  src/ccad_core/gerber_draw_item.cpp
  src/ccad_core/gerber_file_image.cpp
  src/ccad_core/gerber_file_image_list.cpp
  src/ccad_core/readgerb.cpp
  src/ccad_core/rs274x.cpp
  src/ccad_core/rs274d.cpp
  src/ccad_core/x2_gerber_attributes.cpp
  src/ccad_core/job_file_reader.cpp
  src/ccad_core/gerber_to_polyset.cpp
  src/ccad_core/gerber_collectors.cpp
  src/ccad_core/gerber_diff.cpp
  src/ccad_core/read_netlist.cpp
  src/ccad_core/cvpcb_listboxes.cpp
  src/ccad_core/auto_associate.cpp
  src/ccad_core/ar_autoplacer.cpp
  src/ccad_core/spread_footprints.cpp
  src/ccad_core/pns_algo_base.cpp
  src/ccad_core/pns_index.cpp
  src/ccad_core/pns_node.cpp
  src/ccad_core/pns_dragger.cpp
  src/ccad_core/pns_diff_pair_placer.cpp
  src/ccad_core/pns_meander_placer.cpp
  src/ccad_core/eda_importers.cpp
  src/ccad_core/sch_symbol.cpp
  src/ccad_core/gal_base.cpp
  src/ccad_core/font.cpp
  src/ccad_core/plotter.cpp
  src/ccad_core/drawing_sheet.cpp
  src/ccad_core/netlist_reader.cpp
  src/ccad_core/job_manager.cpp
  src/ccad_core/io_base.cpp
  src/ccad_core/http_client.cpp
  src/ccad_core/database_base.cpp
  src/ccad_core/kimath.cpp
  src/ccad_core/kiplatform.cpp
  src/ccad_core/kinng_base.cpp
  src/ccad_core/idf_exporter.cpp
  src/ccad_core/api_bridge.cpp
  src/ccad_core/connection_graph.cpp
  src/ccad_core/test_zone_intersection.cpp
  src/ccad_core/bus_wire_junction.cpp
  src/ccad_core/pcb_parser.cpp
  src/ccad_core/drc_item.cpp
  src/ccad_core/drc_test_provider.cpp
  src/ccad_core/drc_test_provider_clearance.cpp
  src/ccad_core/drc_test_provider_unrouted.cpp
  src/ccad_core/drc_test_provider_courtyard.cpp
  src/ccad_core/drc_test_provider_edge_clearance.cpp
  src/ccad_core/drc_engine.cpp
  src/ccad_core/pagelayout_editor.cpp
  src/ccad_core/pcb_calculator.cpp
  src/ccad_core/bitmap2component.cpp
  src/ccad_core/agent_settings.cpp
  src/ccad_core/io_mgr.cpp
  src/ccad_core/kicad_plugin.cpp
  src/ccad_core/legacy_plugin.cpp
  src/ccad_core/altium_plugin.cpp
  src/ccad_core/eagle_plugin.cpp
  src/ccad_core/kicad_symbol_import.cpp
  src/ccad_core/kicad_footprint_export.cpp
  src/ccad_core/kicad_pcb_export.cpp
  src/ccad_core/layers.cpp
  src/ccad_core/library_catalog.cpp
  src/ccad_core/pad_number_provider.cpp
  src/ccad_core/review.cpp
  src/ccad_core/schematic_collector.cpp
  src/ccad_core/serialize.cpp
  src/ccad_core/spread_footprints.cpp
  src/ccad_core/transaction.cpp
  src/ccad_core/placement.cpp
  src/ccad_core/dsn_export.cpp
  src/ccad_core/bom_export.cpp
  src/ccad_core/pnp_export.cpp
  src/ccad_core/drill_export.cpp
  src/ccad_core/agent_orchestrator.cpp
  src/ccad_core/agent_runner.cpp
  src/ccad_core/cross_probing.cpp
  src/ccad_core/fix_board_shape.cpp
  src/ccad_core/net_chain_bridging.cpp
  src/ccad_core/net_info.cpp
  src/ccad_core/pad_utils.cpp
  src/ccad_core/footprint_losslessness.cpp
  src/ccad_core/item_geometry.cpp
  src/ccad_core/graphics_cleaner.cpp
  src/ccad_core/lib_symbol.cpp
  src/ccad_core/pin_type.cpp
  src/ccad_core/refdes_tracker.cpp
)

target_include_directories(ccad_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
  target_compile_options(ccad_core PRIVATE /W4)
  if(CCAD_WARNINGS_AS_ERRORS)
    target_compile_options(ccad_core PRIVATE /WX)
  endif()
else()
  target_compile_options(ccad_core PRIVATE -Wall -Wextra -Wpedantic)
  if(CCAD_WARNINGS_AS_ERRORS)
    target_compile_options(ccad_core PRIVATE -Werror)
  endif()
endif()

add_executable(ccad
  src/ccad_cli/app.cpp
  src/ccad_cli/common.cpp
  src/ccad_cli/lib_commands.cpp
  src/ccad_cli/main.cpp
  src/ccad_cli/pcb_commands.cpp
  src/ccad_cli/pcb_object_queries.cpp
  src/ccad_cli/project_commands.cpp
  src/ccad_cli/sch_commands.cpp
  src/ccad_cli/agent_commands.cpp
  src/ccad_cli/agent_kicad_evidence.cpp
  src/ccad_cli/agent_observability_config.cpp
  src/ccad_cli/agent_provider_config.cpp
  src/ccad_cli/agent_session.cpp
  src/ccad_cli/agent_orchestrator_cli.cpp
)
target_link_libraries(ccad PRIVATE ccad_core)


if(MINGW)
  target_link_options(ccad PRIVATE -static -static-libgcc -static-libstdc++)
endif()

enable_testing()

add_executable(ccad_tests
  tests/test_serialize.cpp
)
target_link_libraries(ccad_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME serialize COMMAND ccad_tests)

add_executable(ccad_schematic_collector_tests
  tests/test_schematic_collector.cpp
)
target_link_libraries(ccad_schematic_collector_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_schematic_collector_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME schematic_collector COMMAND ccad_schematic_collector_tests)

add_executable(ccad_lib_symbol_tests
  tests/test_lib_symbol.cpp
)
target_link_libraries(ccad_lib_symbol_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_lib_symbol_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME lib_symbol COMMAND ccad_lib_symbol_tests)

add_executable(ccad_placement_tests
  tests/test_placement.cpp
)
target_link_libraries(ccad_placement_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_placement_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME placement COMMAND ccad_placement_tests)

add_executable(ccad_barcode_tests
  tests/test_barcode.cpp
)
target_link_libraries(ccad_barcode_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_barcode_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME barcode COMMAND ccad_barcode_tests)

add_executable(ccad_layer_tests
  tests/test_layers.cpp
)
target_link_libraries(ccad_layer_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_layer_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME layers COMMAND ccad_layer_tests)

add_executable(ccad_pad_number_provider_tests
  tests/test_pad_number_provider.cpp
)
target_link_libraries(ccad_pad_number_provider_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_pad_number_provider_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME pad_number_provider COMMAND ccad_pad_number_provider_tests)

add_executable(ccad_autoplacer_tests
  tests/test_autoplacer.cpp
)
target_link_libraries(ccad_autoplacer_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_autoplacer_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME autoplacer COMMAND ccad_autoplacer_tests)

add_executable(ccad_autorouter_matrix_tests
  tests/test_autorouter_matrix.cpp
)
target_link_libraries(ccad_autorouter_matrix_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_autorouter_matrix_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME autorouter_matrix COMMAND ccad_autorouter_matrix_tests)

add_executable(ccad_spread_footprints_tests
  tests/test_spread_footprints.cpp
)
target_link_libraries(ccad_spread_footprints_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_spread_footprints_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME spread_footprints COMMAND ccad_spread_footprints_tests)

add_executable(ccad_erc_tests
  tests/test_erc.cpp
)
target_link_libraries(ccad_erc_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_erc_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME erc COMMAND ccad_erc_tests)

add_executable(ccad_fix_board_shape_tests
  tests/test_fix_board_shape.cpp
)
target_link_libraries(ccad_fix_board_shape_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_fix_board_shape_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME fix_board_shape COMMAND ccad_fix_board_shape_tests)

add_executable(ccad_drc_tests
  tests/test_drc.cpp
)
target_link_libraries(ccad_drc_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_drc_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME drc COMMAND ccad_drc_tests)

add_executable(ccad_board_design_settings_tests
  tests/test_board_design_settings.cpp
)
target_link_libraries(ccad_board_design_settings_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_design_settings_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_design_settings COMMAND ccad_board_design_settings_tests)

add_executable(ccad_board_loader_tests
  tests/test_board_loader.cpp
)
target_link_libraries(ccad_board_loader_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_loader_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_loader COMMAND ccad_board_loader_tests)

add_executable(ccad_board_item_container_tests
  tests/test_board_item_container.cpp
)
target_link_libraries(ccad_board_item_container_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_item_container_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_item_container COMMAND ccad_board_item_container_tests)

add_executable(ccad_board_collector_tests
  tests/test_board_collector.cpp
)
target_link_libraries(ccad_board_collector_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_collector_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_collector COMMAND ccad_board_collector_tests)

add_executable(ccad_board_outline_polygon_tests
  tests/test_board_outline_polygon.cpp
)
target_link_libraries(ccad_board_outline_polygon_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_outline_polygon_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_outline_polygon COMMAND ccad_board_outline_polygon_tests)

add_executable(ccad_board_statistics_tests
  tests/test_board_statistics.cpp
)
target_link_libraries(ccad_board_statistics_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_statistics_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_statistics COMMAND ccad_board_statistics_tests)

add_executable(ccad_board_text_var_adapter_tests
  tests/test_board_text_var_adapter.cpp
)
target_link_libraries(ccad_board_text_var_adapter_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_text_var_adapter_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_text_var_adapter COMMAND ccad_board_text_var_adapter_tests)

add_executable(ccad_board_stackup_tests
  tests/test_board_stackup.cpp
)
target_link_libraries(ccad_board_stackup_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_board_stackup_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME board_stackup COMMAND ccad_board_stackup_tests)

add_executable(ccad_cleanup_item_tests
  tests/test_cleanup_item.cpp
)
target_link_libraries(ccad_cleanup_item_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_cleanup_item_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME cleanup_item COMMAND ccad_cleanup_item_tests)

add_executable(ccad_kicad_footprint_import_tests
  tests/test_kicad_footprint_import.cpp
)
target_link_libraries(ccad_kicad_footprint_import_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_kicad_footprint_import_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME kicad_footprint_import COMMAND ccad_kicad_footprint_import_tests)

add_executable(ccad_kicad_footprint_export_tests
  tests/test_kicad_footprint_export.cpp
)
target_link_libraries(ccad_kicad_footprint_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_kicad_footprint_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME kicad_footprint_export COMMAND ccad_kicad_footprint_export_tests)

add_executable(ccad_kicad_symbol_import_tests
  tests/test_kicad_symbol_import.cpp
)
target_link_libraries(ccad_kicad_symbol_import_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_kicad_symbol_import_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME kicad_symbol_import COMMAND ccad_kicad_symbol_import_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_symbol.kicad_sym)

add_executable(ccad_library_catalog_tests
  tests/test_library_catalog.cpp
)
target_link_libraries(ccad_library_catalog_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_library_catalog_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME library_catalog COMMAND ccad_library_catalog_tests)

add_executable(ccad_agent_serve_tests 
  tests/test_agent_serve.cpp
  src/ccad_cli/app.cpp
  src/ccad_cli/common.cpp
  src/ccad_cli/lib_commands.cpp
  src/ccad_cli/pcb_commands.cpp
  src/ccad_cli/pcb_object_queries.cpp
  src/ccad_cli/project_commands.cpp
  src/ccad_cli/sch_commands.cpp
  src/ccad_cli/agent_commands.cpp
  src/ccad_cli/agent_kicad_evidence.cpp
  src/ccad_cli/agent_observability_config.cpp
  src/ccad_cli/agent_provider_config.cpp
  src/ccad_cli/agent_session.cpp
  src/ccad_cli/agent_orchestrator_cli.cpp
)
target_link_libraries(ccad_agent_serve_tests PRIVATE ccad_core)
add_test(NAME agent_serve COMMAND ccad_agent_serve_tests)

add_executable(ccad_kicad_pcb_export_tests
  tests/test_kicad_pcb_export.cpp
)
target_link_libraries(ccad_kicad_pcb_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_kicad_pcb_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME kicad_pcb_export COMMAND ccad_kicad_pcb_export_tests)

add_executable(ccad_dsn_export_tests
  tests/test_dsn_export.cpp
)
target_link_libraries(ccad_dsn_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_dsn_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME dsn_export COMMAND ccad_dsn_export_tests)

add_executable(ccad_bom_export_tests
  tests/test_bom_export.cpp
)
target_link_libraries(ccad_bom_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_bom_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME bom_export COMMAND ccad_bom_export_tests)

add_executable(ccad_pnp_export_tests
  tests/test_pnp_export.cpp
)
target_link_libraries(ccad_pnp_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_pnp_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME pnp_export COMMAND ccad_pnp_export_tests)

add_executable(ccad_drill_export_tests
  tests/test_drill_export.cpp
)
target_link_libraries(ccad_drill_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_drill_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME drill_export COMMAND ccad_drill_export_tests)

add_executable(ccad_visual_harness_policy_tests
  tests/test_visual_harness_policy.cpp
)
target_include_directories(ccad_visual_harness_policy_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_definitions(ccad_visual_harness_policy_tests PRIVATE
  CCAD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
if(MINGW)
  target_link_options(ccad_visual_harness_policy_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME visual_harness_policy COMMAND ccad_visual_harness_policy_tests)

add_executable(ccad_cross_probing_tests
  tests/test_cross_probing.cpp
)
target_link_libraries(ccad_cross_probing_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_cross_probing_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME cross_probing COMMAND ccad_cross_probing_tests)

add_executable(ccad_net_chain_bridging_tests
  tests/test_net_chain_bridging.cpp
)
target_link_libraries(ccad_net_chain_bridging_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_net_chain_bridging_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME net_chain_bridging COMMAND ccad_net_chain_bridging_tests)

add_executable(ccad_net_info_tests
  tests/test_net_info.cpp
)
target_link_libraries(ccad_net_info_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_net_info_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME net_info COMMAND ccad_net_info_tests)

add_executable(ccad_pad_utils_tests
  tests/test_pad_utils.cpp
)
target_link_libraries(ccad_pad_utils_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_pad_utils_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME pad_utils COMMAND ccad_pad_utils_tests)

add_executable(ccad_item_geometry_tests
  tests/test_item_geometry.cpp
)
target_link_libraries(ccad_item_geometry_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_item_geometry_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME item_geometry COMMAND ccad_item_geometry_tests)

add_executable(ccad_agent_orchestrator_tests
  tests/test_agent_orchestrator.cpp
)
target_link_libraries(ccad_agent_orchestrator_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_agent_orchestrator_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME agent_orchestrator COMMAND ccad_agent_orchestrator_tests)

add_executable(ccad_refdes_tracker_tests
  tests/test_refdes_tracker.cpp
)
target_link_libraries(ccad_refdes_tracker_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_refdes_tracker_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME refdes_tracker COMMAND ccad_refdes_tracker_tests)

add_executable(ccad_footprint_losslessness_tests
  tests/test_footprint_losslessness.cpp
)
target_link_libraries(ccad_footprint_losslessness_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_footprint_losslessness_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME footprint_losslessness COMMAND ccad_footprint_losslessness_tests)

add_executable(ccad_agent_runner_tests
  tests/test_agent_runner.cpp
)
target_link_libraries(ccad_agent_runner_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_agent_runner_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME agent_runner COMMAND ccad_agent_runner_tests)

add_executable(ccad_cli_tests
  tests/test_cli.cpp
)
add_dependencies(ccad_cli_tests ccad)
if(MINGW)
  target_link_options(ccad_cli_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(CCAD_BUILD_GUI)
  find_package(Qt6 QUIET COMPONENTS Widgets Test Svg Network)
  if(Qt6Widgets_FOUND AND Qt6Test_FOUND AND Qt6Svg_FOUND AND Qt6Network_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    add_executable(ccad_gui
      src/ccad_gui/agent_panel.cpp
      src/ccad_gui/agent_marketplace_dialog.cpp
      src/ccad_gui/agent_settings_dialog.cpp
      src/ccad_gui/board_canvas_renderer.cpp
      src/ccad_gui/component_wizard_dialog.cpp
      src/ccad_gui/footprint_placement_dialog.cpp
      src/ccad_gui/symbol_placement_dialog.cpp
      src/ccad_gui/library_browser_dialog.cpp
      src/ccad_gui/ui_map_server.cpp
      src/ccad_gui/diagnostics_panel.cpp
      src/ccad_gui/main.cpp
      src/ccad_gui/object_browser_panel.cpp
      src/ccad_gui/project_summary_panel.cpp
      src/ccad_gui/review_window.cpp
      src/ccad_gui/schematic_canvas_renderer.cpp
      src/ccad_gui/selection_inspector_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
    )
    target_link_libraries(ccad_gui PRIVATE ccad_core Qt6::Widgets Qt6::Svg Qt6::Network)
    if(WIN32)
      target_link_libraries(ccad_gui PRIVATE dwmapi)
    endif()
    if(MINGW)
      target_link_options(ccad_gui PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    if(MSVC)
      target_compile_options(ccad_gui PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui PRIVATE -Werror)
      endif()
    endif()

    add_executable(ccad_gui_footprint_placement_tests
      tests/test_gui_footprint_placement.cpp
      src/ccad_gui/footprint_placement_dialog.cpp
      src/ccad_gui/library_browser_dialog.cpp
    )
    target_link_libraries(ccad_gui_footprint_placement_tests PRIVATE ccad_core Qt6::Widgets Qt6::Test)
    target_include_directories(ccad_gui_footprint_placement_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    add_test(NAME gui_footprint_placement COMMAND ccad_gui_footprint_placement_tests)

    add_executable(ccad_gui_symbol_placement_tests
      tests/test_gui_symbol_placement.cpp
      src/ccad_gui/symbol_placement_dialog.cpp
      src/ccad_gui/library_browser_dialog.cpp
    )
    target_link_libraries(ccad_gui_symbol_placement_tests PRIVATE ccad_core Qt6::Widgets Qt6::Test)
    target_include_directories(ccad_gui_symbol_placement_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    add_test(NAME gui_symbol_placement COMMAND ccad_gui_symbol_placement_tests)

    add_executable(ccad_gui_inspector_tests
      tests/test_gui_inspector_panel.cpp
      src/ccad_gui/selection_inspector_panel.cpp
    )
    target_link_libraries(ccad_gui_inspector_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_inspector_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_inspector_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_inspector_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_inspector_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_inspector_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_inspector COMMAND ccad_gui_inspector_tests)

    add_executable(ccad_gui_object_browser_tests
      tests/test_gui_object_browser_panel.cpp
      src/ccad_gui/object_browser_panel.cpp
    )
    target_link_libraries(ccad_gui_object_browser_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_object_browser_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_object_browser_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_object_browser_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_object_browser_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_object_browser_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_object_browser COMMAND ccad_gui_object_browser_tests)

    add_executable(ccad_gui_project_summary_tests
      tests/test_gui_project_summary_panel.cpp
      src/ccad_gui/project_summary_panel.cpp
    )
    target_link_libraries(ccad_gui_project_summary_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_project_summary_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_project_summary_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_project_summary_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_project_summary_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_project_summary_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_project_summary COMMAND ccad_gui_project_summary_tests)

    add_executable(ccad_gui_canvas_selection_style_tests
      tests/test_gui_canvas_selection_style.cpp
      src/ccad_gui/board_canvas_renderer.cpp
    )
    target_link_libraries(ccad_gui_canvas_selection_style_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_canvas_selection_style_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_canvas_selection_style_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_canvas_selection_style_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_canvas_selection_style_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_canvas_selection_style_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_canvas_selection_style COMMAND ccad_gui_canvas_selection_style_tests)

    add_executable(ccad_gui_diagnostics_panel_tests
      tests/test_gui_diagnostics_panel.cpp
      src/ccad_gui/diagnostics_panel.cpp
    )
    target_link_libraries(ccad_gui_diagnostics_panel_tests PRIVATE Qt6::Widgets)
    target_include_directories(ccad_gui_diagnostics_panel_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_diagnostics_panel_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_diagnostics_panel_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_diagnostics_panel_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_diagnostics_panel_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_diagnostics_panel COMMAND ccad_gui_diagnostics_panel_tests)

    add_executable(ccad_gui_canvas_selection_link_tests
      tests/test_gui_canvas_selection_link.cpp
      src/ccad_gui/board_canvas_renderer.cpp
    )
    target_link_libraries(ccad_gui_canvas_selection_link_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_canvas_selection_link_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_canvas_selection_link_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_canvas_selection_link_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_canvas_selection_link_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_canvas_selection_link_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_canvas_selection_link COMMAND ccad_gui_canvas_selection_link_tests)

    add_executable(ccad_gui_diagnostic_markers_tests
      tests/test_gui_diagnostic_markers.cpp
      src/ccad_gui/board_canvas_renderer.cpp
    )
    target_link_libraries(ccad_gui_diagnostic_markers_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_diagnostic_markers_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_diagnostic_markers_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_diagnostic_markers_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_diagnostic_markers_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_diagnostic_markers_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_diagnostic_markers COMMAND ccad_gui_diagnostic_markers_tests)

    add_executable(ccad_gui_canvas_spatial_index_tests
      tests/test_canvas_spatial_index.cpp
    )
    target_link_libraries(ccad_gui_canvas_spatial_index_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_canvas_spatial_index_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_canvas_spatial_index_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_canvas_spatial_index_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_canvas_spatial_index_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_canvas_spatial_index_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_canvas_spatial_index COMMAND ccad_gui_canvas_spatial_index_tests)

    add_executable(ccad_gui_action_tools_tests
      tests/test_gui_action_tools.cpp
      src/ccad_gui/agent_panel.cpp
      src/ccad_gui/agent_marketplace_dialog.cpp
      src/ccad_gui/agent_settings_dialog.cpp
      src/ccad_gui/review_window.cpp
      src/ccad_gui/component_wizard_dialog.cpp
      src/ccad_gui/footprint_placement_dialog.cpp
      src/ccad_gui/symbol_placement_dialog.cpp
      src/ccad_gui/library_browser_dialog.cpp
      src/ccad_gui/project_summary_panel.cpp
      src/ccad_gui/diagnostics_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
      src/ccad_gui/object_browser_panel.cpp
      src/ccad_gui/selection_inspector_panel.cpp
      src/ccad_gui/board_canvas_renderer.cpp
      src/ccad_gui/schematic_canvas_renderer.cpp
      src/ccad_gui/ui_map_server.cpp
    )
    target_link_libraries(ccad_gui_action_tools_tests PRIVATE ccad_core Qt6::Widgets Qt6::Svg Qt6::Network)
    if(WIN32)
      target_link_libraries(ccad_gui_action_tools_tests PRIVATE dwmapi)
    endif()
    target_include_directories(ccad_gui_action_tools_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_action_tools_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_action_tools_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_action_tools_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_action_tools_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_action_tools COMMAND ccad_gui_action_tools_tests)

    add_executable(ccad_gui_transaction_timeline_tests
      tests/test_gui_transaction_timeline_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
    )
    target_link_libraries(ccad_gui_transaction_timeline_tests PRIVATE ccad_core Qt6::Widgets)
    target_include_directories(ccad_gui_transaction_timeline_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_transaction_timeline_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_transaction_timeline_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_transaction_timeline_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_transaction_timeline_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_transaction_timeline COMMAND ccad_gui_transaction_timeline_tests)

    add_executable(ccad_gui_board_canvas_view_tests
      tests/test_gui_board_canvas_view.cpp
    )
    target_link_libraries(ccad_gui_board_canvas_view_tests PRIVATE Qt6::Widgets)
    target_include_directories(ccad_gui_board_canvas_view_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_board_canvas_view_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_board_canvas_view_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_board_canvas_view_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_board_canvas_view_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_board_canvas_view COMMAND ccad_gui_board_canvas_view_tests)

    add_executable(ccad_gui_agent_panel_tests
      tests/test_gui_agent_panel.cpp
      src/ccad_gui/agent_panel.cpp
      src/ccad_gui/agent_marketplace_dialog.cpp
      src/ccad_gui/agent_settings_dialog.cpp
    )
    target_link_libraries(ccad_gui_agent_panel_tests PRIVATE ccad_core Qt6::Widgets Qt6::Network Qt6::Test)
    target_include_directories(ccad_gui_agent_panel_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_agent_panel_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_agent_panel_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_agent_panel_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_agent_panel_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_agent_panel COMMAND ccad_gui_agent_panel_tests)

    add_executable(ccad_gui_ui_map_tests
      tests/test_gui_ui_map.cpp
      src/ccad_gui/agent_panel.cpp
      src/ccad_gui/agent_marketplace_dialog.cpp
      src/ccad_gui/agent_settings_dialog.cpp
      src/ccad_gui/review_window.cpp
      src/ccad_gui/component_wizard_dialog.cpp
      src/ccad_gui/footprint_placement_dialog.cpp
      src/ccad_gui/symbol_placement_dialog.cpp
      src/ccad_gui/library_browser_dialog.cpp
      src/ccad_gui/project_summary_panel.cpp
      src/ccad_gui/diagnostics_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
      src/ccad_gui/object_browser_panel.cpp
      src/ccad_gui/selection_inspector_panel.cpp
      src/ccad_gui/board_canvas_renderer.cpp
      src/ccad_gui/schematic_canvas_renderer.cpp
      src/ccad_gui/ui_map_server.cpp
    )
    target_link_libraries(ccad_gui_ui_map_tests PRIVATE ccad_core Qt6::Widgets Qt6::Svg Qt6::Network)
    if(WIN32)
      target_link_libraries(ccad_gui_ui_map_tests PRIVATE dwmapi)
    endif()
    target_include_directories(ccad_gui_ui_map_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_ui_map_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_ui_map_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_ui_map_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_ui_map_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_ui_map COMMAND ccad_gui_ui_map_tests)

    add_executable(ccad_gui_review_window_cursor_status_tests
      tests/test_gui_review_window_cursor_status.cpp
      src/ccad_gui/agent_panel.cpp
      src/ccad_gui/agent_marketplace_dialog.cpp
      src/ccad_gui/agent_settings_dialog.cpp
      src/ccad_gui/review_window.cpp
      src/ccad_gui/component_wizard_dialog.cpp
      src/ccad_gui/footprint_placement_dialog.cpp
      src/ccad_gui/symbol_placement_dialog.cpp
      src/ccad_gui/library_browser_dialog.cpp
      src/ccad_gui/project_summary_panel.cpp
      src/ccad_gui/diagnostics_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
      src/ccad_gui/object_browser_panel.cpp
      src/ccad_gui/selection_inspector_panel.cpp
      src/ccad_gui/board_canvas_renderer.cpp
      src/ccad_gui/schematic_canvas_renderer.cpp
    )
    target_link_libraries(ccad_gui_review_window_cursor_status_tests PRIVATE ccad_core Qt6::Widgets Qt6::Svg Qt6::Network)
    if(WIN32)
      target_link_libraries(ccad_gui_review_window_cursor_status_tests PRIVATE dwmapi)
    endif()
    target_include_directories(ccad_gui_review_window_cursor_status_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    if(MSVC)
      target_compile_options(ccad_gui_review_window_cursor_status_tests PRIVATE /W4)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_review_window_cursor_status_tests PRIVATE /WX)
      endif()
    else()
      target_compile_options(ccad_gui_review_window_cursor_status_tests PRIVATE -Wall -Wextra -Wpedantic)
      if(CCAD_WARNINGS_AS_ERRORS)
        target_compile_options(ccad_gui_review_window_cursor_status_tests PRIVATE -Werror)
      endif()
    endif()
    add_test(NAME gui_review_window_cursor_status COMMAND ccad_gui_review_window_cursor_status_tests)
  else()
    message(STATUS "Qt6 Widgets/Test/Svg not found; skipping ccad_gui target")
  endif()
endif()
target_compile_definitions(ccad_cli_tests PRIVATE
  CCAD_BINARY="$<TARGET_FILE:ccad>"
)
add_test(NAME cli COMMAND ccad_cli_tests)

add_executable(ccad_review_tests
  tests/test_review.cpp
)
target_link_libraries(ccad_review_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_review_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME review COMMAND ccad_review_tests)

add_executable(ccad_annotate_tests
  tests/test_annotate.cpp
)
target_link_libraries(ccad_annotate_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_annotate_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME annotate COMMAND ccad_annotate_tests)

add_executable(ccad_autoplace_fields_tests
  tests/test_autoplace_fields.cpp
)
target_link_libraries(ccad_autoplace_fields_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_autoplace_fields_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME autoplace_fields COMMAND ccad_autoplace_fields_tests)

add_executable(ccad_junction_helpers_tests
  tests/test_junction_helpers.cpp
)
target_link_libraries(ccad_junction_helpers_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_junction_helpers_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME junction_helpers COMMAND ccad_junction_helpers_tests)

add_executable(ccad_diff_tests
  tests/test_diff.cpp
)
target_link_libraries(ccad_diff_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_diff_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME diff COMMAND ccad_diff_tests)

add_executable(ccad_transaction_tests
  tests/test_transaction.cpp
)
target_link_libraries(ccad_transaction_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_transaction_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME transaction COMMAND ccad_transaction_tests)

add_executable(ccad_geometry_tests
  tests/test_geometry.cpp
)
target_link_libraries(ccad_geometry_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_geometry_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME geometry COMMAND ccad_geometry_tests)

add_executable(ccad_canvas_tests
  tests/test_canvas.cpp
)
target_link_libraries(ccad_canvas_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_canvas_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME canvas COMMAND ccad_canvas_tests)

add_executable(ccad_graphics_cleaner_tests
  tests/test_graphics_cleaner.cpp
)
target_link_libraries(ccad_graphics_cleaner_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_graphics_cleaner_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME graphics_cleaner COMMAND ccad_graphics_cleaner_tests)
