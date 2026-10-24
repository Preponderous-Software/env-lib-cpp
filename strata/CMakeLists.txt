# envlib-strata: the opt-in Strata 3D frontend for env-lib-cpp (RFC 0029 in
# Preponderous-Software/rfcs). It is separate from the Makefile and from the
# top-level CMakeLists.txt on purpose: the library itself still needs only a
# C++ compiler, and nothing here is built unless asked for.
#
#   cmake -S strata -B build-strata -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-strata -j4
#   ctest --test-dir build-strata --output-on-failure
#
# Strata is fetched at the commit in strata.ref (the v0.6.0 release), or taken
# from a local checkout with -DFETCHCONTENT_SOURCE_DIR_STRATA=<path>.
# See docs/STRATA.md.
#
# The browser build (Emscripten, Strata's WebGL 2 backend) is the same
# directory under emcmake; it builds envlib-strata.html + .js + .wasm:
#
#   emcmake cmake -S strata -B build-web -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-web -j4
cmake_minimum_required(VERSION 3.24)
project(envlib_strata VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ENVLIB_STRATA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(ENVLIB_STRATA_BUILD_TESTS "Build the tests" ON)
option(ENVLIB_STRATA_RENDER_TESTS "Also build the headless render tests (need EGL + a GL 4.5 driver at run time)" ON)

if(EMSCRIPTEN)
    # Strata compiles C++ exceptions as wasm exceptions; everything in one
    # .wasm must match (Strata's docs/WEB.md).
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fwasm-exceptions>")
    add_link_options(-fwasm-exceptions -sALLOW_MEMORY_GROWTH=1)
    # No EGL in a browser: the render tests are desktop-only.
    set(ENVLIB_STRATA_RENDER_TESTS OFF CACHE BOOL "" FORCE)
endif()

# env-lib-cpp itself, from the parent directory's CMakeLists.txt.
add_subdirectory(.. envlibcpp EXCLUDE_FROM_ALL)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/strata.ref" ENVLIB_STRATA_DEFAULT_REF)
string(STRIP "${ENVLIB_STRATA_DEFAULT_REF}" ENVLIB_STRATA_DEFAULT_REF)
set(ENVLIB_STRATA_REF "${ENVLIB_STRATA_DEFAULT_REF}" CACHE STRING "Strata git ref (a full commit SHA; strata.ref pins it)")

include(FetchContent)
set(STRATA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(STRATA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(STRATA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(STRATA_WITH_SIM ON CACHE BOOL "" FORCE)
set(STRATA_WITH_USD OFF CACHE BOOL "" FORCE)
FetchContent_Declare(strata
    GIT_REPOSITORY https://github.com/Preponderous-Software/strata.git
    GIT_TAG ${ENVLIB_STRATA_REF})
FetchContent_MakeAvailable(strata)

function(envlib_strata_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(ENVLIB_STRATA_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
        if(ENVLIB_STRATA_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# EnvironmentView (no GL): the read-only snapshot of an Environment.
add_library(envlib_strata_view STATIC src/style.cpp src/environment_view.cpp)
target_include_directories(envlib_strata_view PUBLIC include)
target_link_libraries(envlib_strata_view PUBLIC envlibcpp::envlibcpp)
# strata.h only, for the primitive kinds in Style; nothing is linked.
target_include_directories(envlib_strata_view PUBLIC "${strata_SOURCE_DIR}/include")
envlib_strata_warnings(envlib_strata_view)

# Scene + ViewerApp: the Strata calls.
add_library(envlib_strata STATIC src/scene.cpp src/viewer_app.cpp)
target_link_libraries(envlib_strata PUBLIC envlib_strata_view strata::static strata::sim)
envlib_strata_warnings(envlib_strata)
add_library(envlib_strata::envlib_strata ALIAS envlib_strata)

if(NOT EMSCRIPTEN)
    add_executable(envlib_strata_demo app/demo.cpp)
    target_link_libraries(envlib_strata_demo PRIVATE envlib_strata)
    envlib_strata_warnings(envlib_strata_demo)
else()
    # The demo worlds in a page: envlib-strata.html (+ .js, .wasm), served as
    # index.html. web/shell.html is the page around the canvas.
    add_executable(envlib_strata_web web/web_main.cpp)
    target_include_directories(envlib_strata_web PRIVATE app)
    target_link_libraries(envlib_strata_web PRIVATE envlib_strata)
    envlib_strata_warnings(envlib_strata_web)
    set_target_properties(envlib_strata_web PROPERTIES OUTPUT_NAME envlib-strata SUFFIX ".html")
    target_link_options(envlib_strata_web PRIVATE
        -sSTACK_SIZE=1MB
        "-sEXPORTED_FUNCTIONS=_main,_envlib_open,_envlib_resize,_envlib_key,_envlib_zoom,_envlib_rotate,_envlib_home,_envlib_pick,_envlib_status"
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    set_property(TARGET envlib_strata_web APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    # The page's ?bench mode (web/bench.js) is served next to it.
    configure_file(web/bench.js "${CMAKE_CURRENT_BINARY_DIR}/bench.js" COPYONLY)
endif()

if(ENVLIB_STRATA_BUILD_TESTS)
    enable_testing()
    add_executable(envlib_strata_tests test/test_main.cpp test/test_view.cpp)
    target_include_directories(envlib_strata_tests PRIVATE app test)
    target_link_libraries(envlib_strata_tests PRIVATE envlib_strata)
    envlib_strata_warnings(envlib_strata_tests)
    add_test(NAME envlib_strata_view COMMAND envlib_strata_tests "[view]")
    if(ENVLIB_STRATA_RENDER_TESTS)
        target_sources(envlib_strata_tests PRIVATE test/test_render.cpp)
        target_compile_definitions(envlib_strata_tests PRIVATE
            ENVLIB_STRATA_GOLDEN="${CMAKE_CURRENT_SOURCE_DIR}/test/golden-walkers-seed7.txt")
        add_test(NAME envlib_strata_render COMMAND envlib_strata_tests "[render]")
    endif()
endif()
