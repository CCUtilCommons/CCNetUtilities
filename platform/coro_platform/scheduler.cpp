#include "scheduler.hpp"
#include "IOEventMonitor.h"
#include <thread>

int Scheduler::caculate_time_out() const noexcept {
	int timeout_ms = -1;
	if (!ready_coroutines.empty()) {
		timeout_ms = 0;
	} else if (!sleepys.empty()) {
		auto next = sleepys.top().sleep;
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
		                next - current())
		                .count();
		timeout_ms = (int)std::max<long long>(0, diff);
	}
	return timeout_ms;
}

void Scheduler::__run() {
	while (!ready_coroutines.empty() || !sleepys.empty() || IOEventManager::instance().has_watchers()) {
		// resume all ready ones first
		while (!ready_coroutines.empty()) {
			auto front_one = ready_coroutines.front();
			ready_coroutines.pop();
			front_one.resume();
		}

		// move expired sleepers to ready queue
		auto now = current();
		while (!sleepys.empty() && sleepys.top().sleep <= now) {
			ready_coroutines.push(sleepys.top().coro_handle);
			sleepys.pop();
		}

		// compute timeout for epoll (ms)
		int timeout_ms = caculate_time_out();

		// POLL IO and collect handles that should be resumed (ET: coroutine will re-register)
		std::vector<std::coroutine_handle<>> ready_from_io;
		IOEventManager::instance().poll(timeout_ms, ready_from_io);

		// push io-ready handles into ready_coroutines
		for (auto h : ready_from_io) {
			ready_coroutines.push(h);
		}

		// If still nothing ready and there are sleepers, sleep until next sleeper time
		if (ready_coroutines.empty() && !sleepys.empty()) {
			std::this_thread::sleep_until(sleepys.top().sleep);
		}
	}
}
