add_executable(minedotc_testcli main.cpp)

set_property(TARGET minedotc_testcli PROPERTY CXX_STANDARD 11)
include_directories(${CMAKE_SOURCE_DIR}/src/minedotc)
target_link_libraries(minedotc_testcli minedotc)

install(TARGETS minedotc_testcli RUNTIME DESTINATION bin)