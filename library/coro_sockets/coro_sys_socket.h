#pragma once
#include "Task.hpp"
#include "socket_address.h"
#include "sys_socket.h"

namespace CNetUtils {
class CoroClientSocket : private ClientSocket {
public:
	friend class CoroServerSocket;
	CoroClientSocket(const socket_raw_t fd)
	    : ClientSocket(fd) { }
	~CoroClientSocket() = default;
	CoroClientSocket(CoroClientSocket&&) = default;
	CoroClientSocket& operator=(CoroClientSocket&& socket) = default;
	Task<ssize_t> async_read(void* buffer, size_t buffer_size);
	Task<ssize_t> async_write(const void* buffer, size_t buffer_size);

	void close() { Socket::close(); }

	FullAddress dump_self() const { return ClientSocket::dump_self(); }

private:
	CoroClientSocket(const CoroClientSocket&) = delete;
	CoroClientSocket& operator=(const CoroClientSocket&) = delete;
};

class CoroServerSocket : private ServerSocket {
public:
	using async_client_comming_callback_t = Task<void> (*)(std::shared_ptr<CoroClientSocket> socket);
	CoroServerSocket(CoroServerSocket&&) = default;
	CoroServerSocket& operator=(CoroServerSocket&& socket) = default;

	CoroServerSocket(const ServerAddress& addr)
	    : ServerSocket(addr) { }
	CoroServerSocket(ServerAddress&& addr)
	    : ServerSocket(std::move(addr)) { }

	void run_server(async_client_comming_callback_t callback);

	CNETUTILS_FORCEINLINE ServerAddress
	dump_address() const noexcept { return ServerSocket::dump_address(); }

	CNETUTILS_FORCEINLINE Sync sync() const { return ServerSocket::sync(); }
	void close() { Socket::close(); }

private:
	std::shared_ptr<CoroClientSocket> accept() const;
	Task<std::shared_ptr<CoroClientSocket>> __async_accept();
	Task<void> __accept_loop(async_client_comming_callback_t callback);

	CoroServerSocket() = delete;
	CoroServerSocket(const CoroServerSocket&) = delete;
};

}