cmake_minimum_required(VERSION 3.15)
project(mdf4parser-lib LANGUAGES CXX)

find_package(mdf4parser CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
option(MDF4_BUILD_EXAMPLES "Build example applications" ON)

if(MDF4_BUILD_EXAMPLES)
    add_executable(mdf4_basic examples/basic/main.cpp)
    target_link_libraries(mdf4_basic PRIVATE mdf4parser::mdf4parser)

    add_executable(mdf4_json examples/json_export/main.cpp)
    target_link_libraries(mdf4_json PRIVATE mdf4parser::mdf4parser)
endif()
