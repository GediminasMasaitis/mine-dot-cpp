cmake_minimum_required(VERSION 3.6)
project(minedotcpp_root CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)

# Aggressive Release-only optimization flags. MSVC has no literal /O3 — /O2
# is already the top tier — but /GL + /LTCG (whole-program / link-time
# codegen), /Oi (intrinsics), /Ot (speed > size), /arch:AVX2 (SIMD), and
# /fp:fast together are the MSVC equivalent of GCC -O3 -march=native.
# GCC/Clang path sets the literal -O3 + native tuning + fast math.
# Applied at root so every subproject (lib, testcli, umsi) picks it up.
if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/GL>
                        $<$<CONFIG:Release>:/Oi>
                        $<$<CONFIG:Release>:/Ot>
                        $<$<CONFIG:Release>:/arch:AVX2>
                        $<$<CONFIG:Release>:/fp:fast>)
    # /GL requires /LTCG at link time; without it, MSVC silently falls back
    # so we'd get all the codegen cost and none of the benefit.
    add_link_options($<$<CONFIG:Release>:/LTCG>)
else()
    add_compile_options($<$<CONFIG:Release>:-O3>
                        $<$<CONFIG:Release>:-march=native>
                        $<$<CONFIG:Release>:-ffast-math>)
endif()

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