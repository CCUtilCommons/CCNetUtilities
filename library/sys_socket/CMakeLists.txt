message("Configure SimpleSysSocket")
add_library(SimpleSyncSocket sys_socket.cpp socket_address.cpp)
target_include_directories(
    SimpleSyncSocket PUBLIC 
    . 
    ..
    ${CMAKE_CURRENT_BINARY_DIR}/../implement)

