#include "sys_socket.h"
#include "socket_exception.hpp"
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace CNetUtils {

/* ------------------- Socket --------------------- */
Socket::Socket(const socket_raw_t socket_raw) noexcept
    : socket_fd(socket_raw) {
}

Socket::Socket(Socket&& other) noexcept
    : socket_fd(other.socket_fd) {
	other.socket_fd = INVALID_FD;
}

Socket& Socket::operator=(Socket&& other) noexcept {
	socket_fd = other.socket_fd;
	other.socket_fd = INVALID_FD; // already moved!
	return *this;
}

void Socket::close() {
	if (is_valid()) {
		::close(socket_fd);
		socket_fd = INVALID_FD;
	}
}

/* ------------------- ClientSocket --------------------- */

ClientSocket::ClientSocket(const socket_raw_t fd)
    : Socket(fd) {
}

ClientSocket::ClientSocket(ClientSocket&& client) = default;

ClientSocket& ClientSocket::operator=(ClientSocket&& client) = default;

ssize_t ClientSocket::read(void* buffer_ptr, size_t size) {
	if (!is_valid())
		throw SocketException("Invalid Socket Handle");

	ssize_t n;
	do {
		n = ::recv(socket_fd, buffer_ptr, size, 0);
	} while (n < 0 && errno == EINTR);
	return n;
}

ssize_t ClientSocket::write(const void* buffer_ptr, size_t size) {
	if (!is_valid())
		throw SocketException("Invalid Socket Handle");

	ssize_t n;
	do {
		n = ::send(socket_fd, buffer_ptr, size, 0);
	} while (n < 0 && errno == EINTR);
	return n;
}

ClientSocket::~ClientSocket() {
	ClientSocket::close();
}

void ClientSocket::close() {
	auto cli_addr = reinterpret_cast<sockaddr_in*>(client_handle);
	delete cli_addr;
	Socket::close();
}

void ClientSocket::set_internal_data(void* datas) {
	client_handle = new sockaddr_in(*reinterpret_cast<sockaddr_in*>(datas));
} // set the internal datas

FullAddress ClientSocket::dump_self() const {
	char ip[INET_ADDRSTRLEN];

	auto cli_addr = reinterpret_cast<sockaddr_in*>(client_handle);
	inet_ntop(AF_INET, &cli_addr->sin_addr, ip, sizeof(ip));

	return { ip, ntohs(cli_addr->sin_port) };
}

/* ------------------- ServerSocket --------------------- */

ServerSocket::ServerSocket(const ServerAddress& addr)
    : Socket(INVALID_FD)
    , server_addr(addr) {
}

ServerSocket::ServerSocket(ServerAddress&& addr)
    : Socket(INVALID_FD)
    , server_addr(std::move(addr)) {
}

ServerSocket& ServerSocket::operator=(ServerSocket&& socket) {
	this->server_addr = std::move(socket.server_addr);
	return *this;
}
ServerSocket::ServerSocket(ServerSocket&& socket)
    : Socket(socket.socket_fd)
    , server_addr(std::move(socket.server_addr)) {
	socket.socket_fd = INVALID_FD;
}

void ServerSocket::listen(Sync isSync) {
	int flags = SOCK_CLOEXEC | SOCK_STREAM;
	if (isSync == Sync::ASync) {
		flags |= SOCK_NONBLOCK;
	}
	int listen_fd = ::socket(AF_INET, flags, 0);
	if (listen_fd < 0)
		throw CreateError("Create failed!", errno);

	int opt = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw SocketException("setsockopt(SO_REUSEADDR) failed");

	sockaddr_in addr {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_addr.port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
		throw BindError("Can not bind the socket!", errno);

	if (::listen(listen_fd, SOMAXCONN) != 0)
		throw ListenError("Can not listen", errno);

	this->isSync = isSync;
	socket_fd = listen_fd;
}

std::shared_ptr<ClientSocket> ServerSocket::accept(Sync isSync) const {
	if (!is_valid())
		throw SocketException("Invalid Socket Handle");

	sockaddr_in cli {};
	socklen_t cli_len = sizeof(cli);

	int flags = SOCK_CLOEXEC;
	if (isSync == Sync::ASync) {
		flags |= SOCK_NONBLOCK;
	}

	int fd = ::accept4(socket_fd, reinterpret_cast<sockaddr*>(&cli),
	                   &cli_len, flags);
	if (fd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return nullptr;
		throw AcceptError("Error in Accept!");
	}

	auto result = std::make_shared<ClientSocket>(fd);
	result->set_internal_data(&cli);
	result->isSync = isSync;
	return result;
}

} // namespace CNetUtils
