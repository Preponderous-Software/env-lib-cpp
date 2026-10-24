# env-lib-cpp as a CMake target, for projects that build with CMake.
#
#   cmake -S . -B build && cmake --build build && ctest --test-dir build
#
# It builds the same four sources the Makefile compiles into the static
# library `envlibcpp` (alias `envlibcpp::envlibcpp`, headers under
# src/header/), plus the existing test suite (src/tests.cpp) as a CTest test.
# The Makefile is unchanged and still the simplest way to run the tests.
#
# A consumer can use it with add_subdirectory() or FetchContent; the opt-in
# Strata frontend in strata/ does (see docs/STRATA.md).
cmake_minimum_required(VERSION 3.16)
project(envlibcpp LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_library(envlibcpp STATIC
    src/entity.cpp
    src/environment.cpp
    src/grid.cpp
    src/location.cpp)
add_library(envlibcpp::envlibcpp ALIAS envlibcpp)
target_include_directories(envlibcpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/header")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(ENVLIBCPP_TESTS_DEFAULT ON)
else()
    set(ENVLIBCPP_TESTS_DEFAULT OFF)  # not when another project adds this one
endif()
option(ENVLIBCPP_BUILD_TESTS "Build the env-lib-cpp test suite (src/tests.cpp)" ${ENVLIBCPP_TESTS_DEFAULT})
if(ENVLIBCPP_BUILD_TESTS)
    enable_testing()
    add_executable(envlibcpp_tests src/tests.cpp)
    target_link_libraries(envlibcpp_tests PRIVATE envlibcpp)
    # The suite asserts with assert(); keep it active in every build type.
    target_compile_options(envlibcpp_tests PRIVATE -UNDEBUG)
    add_test(NAME envlibcpp_tests COMMAND envlibcpp_tests)
endif()
