#pragma once

#include "Task.hpp"
#include "sys_socket.h"

namespace CNetUtils {

using client_comming_callback_t = Task<void> (*)(std::shared_ptr<ClientSocket> socket);

void run_server(
    std::shared_ptr<ServerSocket> server_socket,
    client_comming_callback_t callback);

Task<ssize_t> async_read(
    std::shared_ptr<ClientSocket> socket, void* buffer, size_t buffer_size);

Task<ssize_t> async_write(
    std::shared_ptr<ClientSocket> socket, const void* buffer, size_t buffer_size);

}