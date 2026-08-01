cmake_minimum_required(VERSION 3.15)
project(mdf4parser-lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0091 NEW)

# ---------------------------------------------------------------------------
# Find dependencies (needed for linking)
# ---------------------------------------------------------------------------
find_package(Protobuf CONFIG QUIET)
if(NOT TARGET protobuf::libprotobuf)
    find_package(Protobuf REQUIRED)
endif()

# The reader inflates DZBLOCK payloads, so the static library carries zlib
# symbols that the consumer must resolve.
find_package(ZLIB REQUIRED)

# ---------------------------------------------------------------------------
# Prebuilt static library
# ---------------------------------------------------------------------------
if(NOT MDF4_LIB_DIR)
    set(MDF4_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib" CACHE PATH
        "Directory containing prebuilt mdf4parser library and generated .pb.h files")
endif()
get_filename_component(MDF4_LIB_DIR "${MDF4_LIB_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Find the library file (.a on MinGW/Linux, .lib on MSVC)
find_library(MDF4_LIB_PATH NAMES mdf4parser PATHS "${MDF4_LIB_DIR}" NO_DEFAULT_PATH)
if(NOT MDF4_LIB_PATH)
    # Fallback: try explicit names
    if(EXISTS "${MDF4_LIB_DIR}/libmdf4parser.a")
        set(MDF4_LIB_PATH "${MDF4_LIB_DIR}/libmdf4parser.a")
    elseif(EXISTS "${MDF4_LIB_DIR}/mdf4parser.lib")
        set(MDF4_LIB_PATH "${MDF4_LIB_DIR}/mdf4parser.lib")
    else()
        message(FATAL_ERROR "Could not find mdf4parser library in ${MDF4_LIB_DIR}")
    endif()
endif()

if(NOT TARGET mdf4parser)
    add_library(mdf4parser STATIC IMPORTED GLOBAL)
    set_target_properties(mdf4parser PROPERTIES
        IMPORTED_LOCATION "${MDF4_LIB_PATH}"
    )

    target_include_directories(mdf4parser INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MDF4_LIB_DIR}
    )

    target_link_libraries(mdf4parser INTERFACE protobuf::libprotobuf ZLIB::ZLIB)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
option(MDF4_BUILD_EXAMPLES "Build example applications" ON)

if(MDF4_BUILD_EXAMPLES)
    add_executable(mdf4_basic examples/basic/main.cpp)
    target_link_libraries(mdf4_basic PRIVATE mdf4parser)

    add_executable(mdf4_json examples/json_export/main.cpp)
    target_link_libraries(mdf4_json PRIVATE mdf4parser)
endif()
