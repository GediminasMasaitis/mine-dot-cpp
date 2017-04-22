file(GLOB_RECURSE minedotcpp_src
    "*.h"
    "*.cpp"
)

add_library(minedotcpp ${minedotcpp_src})
set_property(TARGET minedotcpp PROPERTY CXX_STANDARD 11)
add_definitions(-DMINE_EXPORTS)

install(TARGETS minedotcpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)