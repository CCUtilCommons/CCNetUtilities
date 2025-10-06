#include "IOEventMonitor.h"
#include "IOEvent_Exception.hpp"
#include <cerrno>
#include <sys/epoll.h>
#include <unistd.h>

IOEventManager::IOEventManager() {
	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd < 0) {
		throw EpollCreateError("epoll_create1 failed", errno);
	}
}

IOEventManager::~IOEventManager() {
	if (epoll_fd >= 0)
		close(epoll_fd);
}

void IOEventManager::add_waiter(int fd, Event events, std::coroutine_handle<> h) {
	if (fd < 0) {
		throw InvalidFDException("Attempted to add waiter for invalid FD");
	}
	uint32_t __adapt = 0;
	switch (events) {
	case Event::MONITOR_READ:
		__adapt = EPOLLIN;
		break;
	case Event::MONITOR_WRITE:
		__adapt = EPOLLOUT;
		break;
	default:
		throw EpollUnsupportiveEvent("Unsupported operations");
	}

	uint32_t new_events = __adapt | EPOLLET;
	epoll_event ev {};
	ev.events = new_events;
	ev.data.fd = fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0) {
		if (errno == EEXIST) {
			if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) != 0) {
				throw EpollCtlError("epoll_ctl MOD failed", errno);
			}
		} else {
			throw EpollCtlError("epoll_ctl ADD failed", errno);
		}
	}

	table[fd] = Waiter { new_events, h };
}

void IOEventManager::remove_waiter(int fd) {
	auto it = table.find(fd);
	if (it == table.end())
		return;
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
	table.erase(it);
}

void IOEventManager::poll(int timeout_ms, std::vector<std::coroutine_handle<>>& out_handles) {
	const int MAX_EVENTS = 64;
	epoll_event events[MAX_EVENTS];
	int n = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout_ms);
	if (n < 0) {
		if (errno == EINTR)
			return;
		// std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
		return;
	}
	for (int i = 0; i < n; ++i) {
		int fd = events[i].data.fd;
		auto it = table.find(fd);
		if (it != table.end()) {
			auto handle = it->second.handle;
			// ET semantics: remove registration and let coroutine re-register if needed
			table.erase(it);
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
			if (handle)
				out_handles.push_back(handle);
		}
	}
}
