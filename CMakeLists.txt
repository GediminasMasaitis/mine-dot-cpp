cmake_minimum_required(VERSION 3.6)
project(minedotcpp_root CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(shared true)

add_subdirectory(src/minedotcpp)
add_subdirectory(src/minedotcpp_testcli)