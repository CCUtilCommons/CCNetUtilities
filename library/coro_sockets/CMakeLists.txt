message("Configure CoroSysSocket")
add_library(CoroSysSocket coro_sys_socket.cpp coro_helper.cpp)
target_include_directories(
    CoroSysSocket PUBLIC 
    . 
    ..
    ${CMAKE_BINARY_DIR}/library/implement)

target_link_libraries(CoroSysSocket PUBLIC NetUtilsEnv SimpleSyncSocket)

