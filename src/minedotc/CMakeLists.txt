file(GLOB_RECURSE minedotc_src
    "*.h"
    "*.cpp"
)

add_library(minedotc ${minedotc_src})
set_property(TARGET minedotc PROPERTY CXX_STANDARD 11)
add_definitions(-DMINE_EXPORTS)

install(TARGETS minedotc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)