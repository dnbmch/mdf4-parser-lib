cmake_minimum_required(VERSION 3.14)
project(mdf4_basic_example LANGUAGES CXX)

# Pull in mdf4-parser as a subdirectory unless the parent build already defines
# the target (this CMakeLists is built both standalone and from the repo root).
# Adjust the path if you place this example elsewhere.
if(NOT TARGET mdf4parser)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/mdf4parser)
endif()

add_executable(mdf4_basic main.cpp)
target_link_libraries(mdf4_basic PRIVATE mdf4parser)
