file(GLOB_RECURSE minedotcpp_src
    "*.h"
    "*.cpp"
)
if(shared)
    add_library(minedotcpp SHARED ${minedotcpp_src})
    add_definitions(-DSHARED_LIB)
else()
    add_library(minedotcpp ${minedotcpp_src})
endif()
set_property(TARGET minedotcpp PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
add_definitions(-DMINE_EXPORTS)

# A second, always-built SHARED library with the same sources. Exists so C#
# can P/Invoke directly into the solver (bypassing UMSI's stdio layer) as an
# opt-in alternative for benchmark runs. The static `minedotcpp` above is
# still what testcli/umsi link against; nothing downstream changes.
add_library(minedotcpp_shared SHARED ${minedotcpp_src})
set_property(TARGET minedotcpp_shared PROPERTY CXX_STANDARD 20)
target_compile_definitions(minedotcpp_shared PRIVATE SHARED_LIB MINE_EXPORTS)
set_target_properties(minedotcpp_shared PROPERTIES
    OUTPUT_NAME "minedotcpp_direct"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include_directories(${CMAKE_SOURCE_DIR}/external/ctpl)

if(USE_SPARSEHASH)
    include_directories(${CMAKE_SOURCE_DIR}/external/sparsehash)
endif()

if(USE_OPENCL)
    add_definitions(-DENABLE_OPEN_CL)
    find_package(
        OpenCL
        #HINTS "C:\\Program Files (x86)\\IntelSWTools\\system_studio_2020\\OpenCL\\sdk"
        REQUIRED
    )
    message(STATUS ${OpenCL_INCLUDE_DIR})
    message(STATUS ${OpenCL_LIBRARY})
    include_directories(${OpenCL_INCLUDE_DIR})
    target_link_libraries(minedotcpp ${OpenCL_LIBRARY})
    target_link_libraries(minedotcpp_shared ${OpenCL_LIBRARY})
endif()

install(TARGETS minedotcpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)