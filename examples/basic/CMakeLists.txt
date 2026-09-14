cmake_minimum_required(VERSION 3.15)
project(mdf4_basic_example LANGUAGES CXX)

find_package(mdf4parser CONFIG REQUIRED)
add_executable(mdf4_basic main.cpp)
target_link_libraries(mdf4_basic PRIVATE mdf4parser::mdf4parser)
