#pragma once

#include "library_utils.h"
#include "single_instance.hpp"
#include <coroutine>
#include <cstdint>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

#define IO_MANAFER_INCLUDE_PREFER
#include "coro_platform_impl.h"

#define SYNC_SOCKET_PREFER
#include "socket_impl.h"

class IOEventManager : public SingleInstance<IOEventManager> {
public:
	friend class SingleInstance<IOEventManager>;
	using socket_raw_t = CNetUtils::socket_raw_t;

	enum class Event {
		MONITOR_READ,
		MONITOR_WRITE
	};

	// register a waiter for (fd, events). If already registered, expand/replace.
	// void add_waiter(socket_raw_t fd, uint32_t events, std::coroutine_handle<> h);
	void add_waiter(socket_raw_t fd, Event events, std::coroutine_handle<> h);

	// remove a specific watcher
	void remove_waiter(socket_raw_t fd);

	// poll events, timeout in ms (-1 block)
	void poll(int timeout_ms, std::vector<std::coroutine_handle<>>& out_handles);

	// whether there are any watchers
	CNETUTILS_FORCEINLINE bool has_watchers() const noexcept {
		return !table.empty();
	}

private:
	IOEventManager();
	~IOEventManager();

	CNetUtils::IOEventManager_Internal_t epoll_fd { -1 };

	struct Waiter {
		uint32_t events;
		std::coroutine_handle<> handle;
	};

	std::unordered_map<int, Waiter> table;
};
