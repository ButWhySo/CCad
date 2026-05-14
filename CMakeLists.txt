cmake_minimum_required(VERSION 3.20)

project(CCad VERSION 0.1.0 LANGUAGES CXX)

option(CCAD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ccad_core
  src/ccad_core/model.cpp
  src/ccad_core/serialize.cpp
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

enable_testing()

add_executable(ccad_tests
  tests/test_serialize.cpp
)
target_link_libraries(ccad_tests PRIVATE ccad_core)
add_test(NAME serialize COMMAND ccad_tests)
