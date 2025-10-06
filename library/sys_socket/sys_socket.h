#pragma once

#include <cstddef>
#include <memory>
#include <sys/types.h>
#define SYNC_SOCKET_PREFER
#include "library_utils.h"
#include "socket_address.h"
#include "socket_impl.h"

namespace CNetUtils {

enum class Sync {
	Sync,
	ASync
};

/**
 * @brief   Socket is the most simple and common
 *          abstructions of OS Sockets
 *
 */
class Socket {
public:
	using socket_raw_t = CNetUtils::socket_raw_t;
	virtual ~Socket() = default;
	Socket(const socket_raw_t socket_raw) noexcept;
	Socket(Socket&&) noexcept;
	Socket& operator=(Socket&&) noexcept;
	CNETUTILS_FORCEINLINE socket_raw_t internal() const noexcept { return socket_fd; }
	CNETUTILS_FORCEINLINE bool is_valid() const noexcept { return CNetUtils::is_valid_socket_raw(socket_fd); }

	virtual void close(); // socket close

protected:
	socket_raw_t socket_fd { CNetUtils::INVALID_FD };

private:
	Socket() = delete;
	Socket(const Socket&) = delete;
	Socket& operator=(const Socket&) = delete;
};

class ClientSocket : public Socket {
public:
	friend class ServerSocket;
	friend class CoroServerSocket;
	ClientSocket(const socket_raw_t fd);
	ClientSocket(ClientSocket&& client);
	ClientSocket& operator=(ClientSocket&& client);
	virtual ~ClientSocket() override;
	/**
	 * @brief read given bytes to buffer_ptr, with target size
	 * @exception SocketException Invalid Socket handle
	 *
	 * @param buffer_ptr
	 * @param size
	 * @return ssize_t
	 */
	ssize_t read(void* buffer_ptr, size_t size);

	/**
	 * @brief send bytes to the peer
	 * @exception SocketException Invalid Socket handle
	 *
	 * @param buffer_ptr
	 * @param size
	 * @return ssize_t
	 */
	ssize_t write(const void* buffer_ptr, size_t size);

	Sync sync() const { return isSync; }

	FullAddress dump_self() const;
	void close() override;

private:
	Sync isSync;
	void* client_handle { nullptr };
	void set_internal_data(void* datas); // set the internal datas

private:
	ClientSocket(const ClientSocket& client) = delete;
	ClientSocket& operator=(const ClientSocket& client) = delete;
};

class ServerSocket : public Socket {
public:
	using ServerAddress = CNetUtils::ServerAddress;
	ServerSocket(const ServerAddress& addr);
	ServerSocket(ServerAddress&& addr);
	ServerSocket& operator=(ServerSocket&& socket);
	ServerSocket(ServerSocket&& socket);
	virtual ~ServerSocket() = default;
	/**
	 * @brief Dump the Address
	 *
	 * @return ServerAddress
	 */
	CNETUTILS_FORCEINLINE ServerAddress
	dump_address() const noexcept { return server_addr; }

	/**
	 * @brief After called listened, the socket can be valid in use
	 * @exception SocketException: failed to reuse tcps
	 * @exception CreateException: failed to setup a socket
	 * @exception BindError: failed to bind a socket
	 * @exception ListenError: failed to listen a socket
	 */
	void listen(Sync isSync = Sync::ASync);

	/**
	 * @brief accept sync a socket passively
	 * @exception SocketException: invalid socket as not listening or moved!
	 * @exception Accept Error: can not accept more!
	 */
	std::shared_ptr<ClientSocket> accept(Sync isSync = Sync::ASync) const;

	Sync sync() const { return isSync; }

private:
	Sync isSync;
	ServerAddress server_addr;

private:
	ServerSocket() = delete;
	ServerSocket(const ServerSocket&) = delete;
	ServerSocket& operator=(ServerSocket&) = delete;
};

}
