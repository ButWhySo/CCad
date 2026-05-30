cmake_minimum_required(VERSION 3.20)

project(CCad VERSION 0.1.0 LANGUAGES CXX)

option(CCAD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(CCAD_BUILD_GUI "Build Qt review GUI when Qt 6 Widgets is available" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ccad_core
  src/ccad_core/canvas.cpp
  src/ccad_core/model.cpp
  src/ccad_core/diff.cpp
  src/ccad_core/drc.cpp
  src/ccad_core/erc.cpp
  src/ccad_core/geometry.cpp
  src/ccad_core/json.cpp
  src/ccad_core/kicad_footprint_import.cpp
  src/ccad_core/kicad_pcb_export.cpp
  src/ccad_core/layers.cpp
  src/ccad_core/library_catalog.cpp
  src/ccad_core/review.cpp
  src/ccad_core/serialize.cpp
  src/ccad_core/transaction.cpp
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

add_executable(ccad_layer_tests
  tests/test_layers.cpp
)
target_link_libraries(ccad_layer_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_layer_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME layers COMMAND ccad_layer_tests)

add_executable(ccad_erc_tests
  tests/test_erc.cpp
)
target_link_libraries(ccad_erc_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_erc_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME erc COMMAND ccad_erc_tests)

add_executable(ccad_drc_tests
  tests/test_drc.cpp
)
target_link_libraries(ccad_drc_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_drc_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME drc COMMAND ccad_drc_tests)

add_executable(ccad_kicad_footprint_import_tests
  tests/test_kicad_footprint_import.cpp
)
target_link_libraries(ccad_kicad_footprint_import_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_kicad_footprint_import_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME kicad_footprint_import COMMAND ccad_kicad_footprint_import_tests)

add_executable(ccad_library_catalog_tests
  tests/test_library_catalog.cpp
)
target_link_libraries(ccad_library_catalog_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_library_catalog_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME library_catalog COMMAND ccad_library_catalog_tests)

add_executable(ccad_kicad_pcb_export_tests
  tests/test_kicad_pcb_export.cpp
)
target_link_libraries(ccad_kicad_pcb_export_tests PRIVATE ccad_core)
if(MINGW)
  target_link_options(ccad_kicad_pcb_export_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME kicad_pcb_export COMMAND ccad_kicad_pcb_export_tests)

add_executable(ccad_cli_tests
  tests/test_cli.cpp
)
add_dependencies(ccad_cli_tests ccad)
if(MINGW)
  target_link_options(ccad_cli_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(CCAD_BUILD_GUI)
  find_package(Qt6 QUIET COMPONENTS Widgets)
  if(Qt6Widgets_FOUND)
    add_executable(ccad_gui
      src/ccad_gui/board_canvas_renderer.cpp
      src/ccad_gui/diagnostics_panel.cpp
      src/ccad_gui/main.cpp
      src/ccad_gui/object_browser_panel.cpp
      src/ccad_gui/project_summary_panel.cpp
      src/ccad_gui/review_window.cpp
      src/ccad_gui/selection_inspector_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
    )
    target_link_libraries(ccad_gui PRIVATE ccad_core Qt6::Widgets)
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

    add_executable(ccad_gui_review_window_cursor_status_tests
      tests/test_gui_review_window_cursor_status.cpp
      src/ccad_gui/review_window.cpp
      src/ccad_gui/project_summary_panel.cpp
      src/ccad_gui/diagnostics_panel.cpp
      src/ccad_gui/transaction_timeline_panel.cpp
      src/ccad_gui/object_browser_panel.cpp
      src/ccad_gui/selection_inspector_panel.cpp
      src/ccad_gui/board_canvas_renderer.cpp
    )
    target_link_libraries(ccad_gui_review_window_cursor_status_tests PRIVATE ccad_core Qt6::Widgets)
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
    message(STATUS "Qt6 Widgets not found; skipping ccad_gui target")
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
