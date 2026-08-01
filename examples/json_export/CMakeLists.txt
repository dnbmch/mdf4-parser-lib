cmake_minimum_required(VERSION 3.14)
project(mdf4_json_export LANGUAGES CXX)

# Pull in mdf4-parser as a subdirectory.
# Adjust the path if you place this example elsewhere.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/mdf4parser)

add_executable(mdf4_json main.cpp)
target_link_libraries(mdf4_json PRIVATE mdf4parser)
