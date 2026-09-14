cmake_minimum_required(VERSION 3.15)
project(mdf4_json_export LANGUAGES CXX)

find_package(mdf4parser CONFIG REQUIRED)
add_executable(mdf4_json main.cpp)
target_link_libraries(mdf4_json PRIVATE mdf4parser::mdf4parser)
