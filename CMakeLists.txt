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
  src/ccad_cli/main.cpp
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
      src/ccad_gui/main.cpp
      src/ccad_gui/review_window.cpp
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
