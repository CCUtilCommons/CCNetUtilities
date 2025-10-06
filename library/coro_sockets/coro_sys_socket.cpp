#include "coro_sys_socket.h"
#include "IOEventMonitor.h"
#include "socket_exception.hpp"
#include "sys_socket.h"
#include <netinet/in.h>

namespace CNetUtils {

namespace {
	using Event = IOEventManager::Event;
	struct WaitForEvent {
		int fd;
		Event events;
		WaitForEvent(int f, Event e)
		    : fd(f)
		    , events(e) { }
		bool await_ready() { return false; }
		void await_suspend(std::coroutine_handle<> h) {
			IOEventManager::instance().add_waiter(fd, events, h);
		}
		void await_resume() { }
	};

	WaitForEvent await_io_event(std::shared_ptr<Socket> socket, IOEventManager::Event events) {
		return { socket->internal(), events };
	}

	WaitForEvent await_io_event(socket_raw_t socket_fd, IOEventManager::Event events) {
		return { socket_fd, events };
	}
}

std::shared_ptr<CoroClientSocket> CoroServerSocket::accept() const {
	if (!is_valid())
		throw SocketException("Invalid Socket Handle");

	sockaddr_in cli {};
	socklen_t cli_len = sizeof(cli);

	int fd = ::accept4(socket_fd, reinterpret_cast<sockaddr*>(&cli),
	                   &cli_len, SOCK_CLOEXEC | SOCK_NONBLOCK);
	if (fd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return nullptr;
		throw AcceptError("Error in Accept!");
	}

	auto result = std::make_shared<CoroClientSocket>(fd);
	result->set_internal_data(&cli);
	return result;
}

Task<std::shared_ptr<CoroClientSocket>> CoroServerSocket::__async_accept() {
	while (true) {
		auto client_socket = this->accept();
		if (client_socket) {
			co_return client_socket;
		}
		co_await await_io_event(socket_fd, IOEventManager::Event::MONITOR_READ);
	}
}

Task<ssize_t> CoroClientSocket::async_read(void* buffer, size_t buffer_size) {
	while (true) {
		ssize_t n = ClientSocket::read(buffer, buffer_size);
		if (n >= 0) {
			co_return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				co_await await_io_event(internal(),
				                        IOEventManager::Event::MONITOR_READ);
				continue;
			} else {
				co_return -1;
			}
		}
	}
}

Task<ssize_t> CoroClientSocket::async_write(const void* buffer, size_t buffer_size) {
	size_t sent = 0;
	while (sent < buffer_size) {
		ssize_t n = ClientSocket::write((const char*)buffer + sent, buffer_size - sent);

		if (n > 0) {
			sent += (size_t)n;
			continue;
		}
		if (n <= 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				co_await await_io_event(internal(),
				                        IOEventManager::Event::MONITOR_WRITE);
				continue;
			} else {
				co_return -1; // quit
			}
		}
	}
	co_return buffer_size;
}

Task<void> CoroServerSocket::__accept_loop(
    async_client_comming_callback_t callback) {
	while (true) {
		auto client_socket = co_await __async_accept();
		Scheduler::instance().spawn(callback(client_socket));
	}
}

void CoroServerSocket::run_server(async_client_comming_callback_t callback) {
	ServerSocket::listen(Sync::ASync); // force Async
	Scheduler::instance().spawn(
	    std::move(__accept_loop(callback)));
	Scheduler::run();
}
}
