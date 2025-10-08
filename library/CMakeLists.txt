print_banner("Configuring the Libraries")

message("Configuring the Basics Libraries")
add_subdirectory(sys_socket) # Export SimpleSyncSocket
add_subdirectory(http) # Export HttpBase

message("Configuring the Coroutine Supports Libraries")
add_subdirectory(coro_sockets) # Export CoroSysSocket
add_subdirectory(coro_http) # Export CoroHttp

print_banner("Configuring the Libraries Done")
