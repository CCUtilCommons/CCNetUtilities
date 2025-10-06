print_banner("Configuring the pre-config files to fit the platforms")

message("Configure Socket Impls...")
message("Configures implements/socket_impl.h -> library/implement/socket_impl.h...")
message(WARNING "Only Linux Platform is supported only now...")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/implement/socket_impl.h"
    "${CMAKE_BINARY_DIR}/library/implement/socket_impl.h"
    COPYONLY
)


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/implement/coro_platform_impl.h"
    "${CMAKE_BINARY_DIR}/platform/implement/coro_platform_impl.h"
    COPYONLY
)


