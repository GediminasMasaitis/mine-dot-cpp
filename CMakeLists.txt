cmake_minimum_required(VERSION 3.6)
project(minedotcpp_root CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)

# External library usage
set(USE_SPARSEHASH true)
option(USE_OPENCL "Enable OpenCL GPU acceleration" ON)
# Build the solver library as a static archive. Downstream targets
# (minedotcpp_testcli, minedotcpp_umsi) link it in, producing
# self-contained executables with no .dll / .so companion to ship.
# Flip to true if a shared library is ever needed again (e.g. for a
# future P/Invoke path).
set(shared false)

add_subdirectory(src/minedotcpp)
add_subdirectory(src/minedotcpp_testcli)
add_subdirectory(src/minedotcpp_umsi)