#include "coro_helper.h"
#include "IOEventMonitor.h"
#include "scheduler.hpp"
#include "sys_socket.h"
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

	Task<std::shared_ptr<ClientSocket>>
	__async_accept(std::shared_ptr<ServerSocket> server_socket) {
		while (true) {
			auto client_socket = server_socket->accept();
			if (client_socket) {
				co_return client_socket;
			}
			co_await await_io_event(server_socket,
			                        IOEventManager::Event::MONITOR_READ);
		}
	}

	Task<void> __accept_loop(
	    std::shared_ptr<ServerSocket> server_socket,
	    client_comming_callback_t callback) {
		while (true) {
			auto client_socket = co_await __async_accept(server_socket);
			Scheduler::spawn(callback(client_socket));
		}
	}

} // for privates and unexported

void run_server(
    std::shared_ptr<ServerSocket> server_socket,
    client_comming_callback_t callback) {
	Scheduler::spawn(
	    std::move(__accept_loop(server_socket, callback)));
	Scheduler::run();
}

Task<ssize_t> async_read(
    std::shared_ptr<ClientSocket> socket,
    void* buffer, size_t buffer_size) {
	while (true) {
		ssize_t n = socket->read(buffer, buffer_size);
		if (n >= 0) {
			co_return n;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				co_await await_io_event(socket, IOEventManager::Event::MONITOR_READ);
				continue;
			} else {
				co_return -1;
			}
		}
	}
}

Task<ssize_t> async_write(
    std::shared_ptr<ClientSocket> socket,
    const void* buffer, size_t buffer_size) {
	size_t sent = 0;
	while (sent < buffer_size) {
		ssize_t n = socket->write((const char*)buffer + sent, buffer_size - sent);

		if (n > 0) {
			sent += (size_t)n;
			continue;
		}
		if (n <= 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				co_await await_io_event(socket,
				                        IOEventManager::Event::MONITOR_WRITE);
				continue;
			} else {
				co_return -1; // 其他错误直接退出
			}
		}
	}
	co_return buffer_size;
}

}