file(GLOB_RECURSE minedotcpp_umsi_src
    "*.h"
    "*.cpp"
)

add_executable(minedotcpp_umsi ${minedotcpp_umsi_src})

set_property(TARGET minedotcpp_umsi PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
if(shared)
    add_definitions(-DSHARED_LIB)
endif()

if(USE_OPENCL)
    add_definitions(-DENABLE_OPEN_CL)
    find_package(OpenCL REQUIRED)
    include_directories(${OpenCL_INCLUDE_DIR})
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/minedotcpp)
include_directories(${CMAKE_SOURCE_DIR}/external/ctpl)
include_directories(${CMAKE_SOURCE_DIR}/external/sparsehash)
target_link_libraries(minedotcpp_umsi minedotcpp)

# Bake a relative rpath so the ELF runtime linker finds libminedotcpp.so
# next to the binary without the caller having to set LD_LIBRARY_PATH. The
# build tree puts the .so in build-linux/lib/ and the exe in build-linux/bin/,
# hence $ORIGIN/../lib. No-op on Windows (PE ignores rpath).
set_target_properties(minedotcpp_umsi PROPERTIES
    BUILD_RPATH "$ORIGIN/../lib"
    INSTALL_RPATH "$ORIGIN/../lib")

install(TARGETS minedotcpp_umsi RUNTIME DESTINATION bin)
