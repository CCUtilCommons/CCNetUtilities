#pragma once

#pragma once
#include "scheduler.hpp"
#include <coroutine>
#include <utility>

/**
 * @brief This is the Task Type for wrapping the C++ Corotines
 *
 * @tparam T
 */
template <typename T>
class Task {
public:
	friend class Scheduler; // for the Schedular access
	struct promise_type;
	using coro_handle = std::coroutine_handle<promise_type>;

	Task(coro_handle h)
	    : coroutine_handle(h) {
	}

	~Task() {
		if (coroutine_handle) {
			coroutine_handle.destroy();
		}
	}

	Task(Task&& o)
	    : coroutine_handle(o.coroutine_handle) {
		o.coroutine_handle = nullptr;
	}

	Task& operator=(Task&& o) {
		coroutine_handle = std::move(o.coroutine_handle);
		o.coroutine_handle = nullptr;
		return *this;
	}

	// concept requires
	struct promise_type {
		T cached_value;
		std::coroutine_handle<> parent_coroutine;
		Task get_return_object() {
			return { coro_handle::from_promise(*this) };
		}
		// we dont need suspend when first suspend
		std::suspend_always initial_suspend() {

			return {};
		}
		// suspend always for the Task clean ups
		std::suspend_always final_suspend() noexcept {
			if (parent_coroutine) {
				Scheduler::instance().internal_spawn(parent_coroutine);
			}
			return {};
		}

		void return_value(T value) {
			cached_value = std::move(value);
		}

		void unhandled_exception() {
			// process notings
		}
	};

	bool await_ready() {
		return false; // always need suspend
	}

	void await_suspend(std::coroutine_handle<> h) {
		// Should never be here
		coroutine_handle.promise().parent_coroutine = h;
		Scheduler::instance().internal_spawn(coroutine_handle);
	}

	T await_resume() {
		return coroutine_handle.promise().cached_value;
	}

private:
	coro_handle coroutine_handle;

private:
	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;
};

template <>
class Task<void> {
public:
	friend class Scheduler;
	struct promise_type;
	using coro_handle = std::coroutine_handle<promise_type>;

	Task(coro_handle h)
	    : coroutine_handle(h) {
	}

	~Task() {
		if (coroutine_handle) {
			coroutine_handle.destroy();
		}
	}

	Task(Task&& o)
	    : coroutine_handle(o.coroutine_handle) {
		o.coroutine_handle = nullptr;
	}

	Task& operator=(Task&& o) {
		coroutine_handle = std::move(o.coroutine_handle);
		o.coroutine_handle = nullptr;
		return *this;
	}

	bool await_ready() {
		return false; // always need suspend
	}

	void await_suspend(std::coroutine_handle<> h) {
		// Should never be here
		coroutine_handle.promise().parent_coroutine = h;
		Scheduler::instance().internal_spawn(coroutine_handle);
	}

	void await_resume() {
	}

	// concept requires
	struct promise_type {
		std::coroutine_handle<> parent_coroutine;
		Task get_return_object() {
			return { coro_handle::from_promise(*this) };
		}
		// we need suspend when first suspend
		std::suspend_always initial_suspend() {
			return {};
		}
		// suspend always for the Task clean ups
		std::suspend_always final_suspend() noexcept {
			if (parent_coroutine) {
				Scheduler::instance().internal_spawn(parent_coroutine);
			}
			return {};
		}
		void return_void() {
		}
		void unhandled_exception() {
			// process notings
		}
	};

private:
	coro_handle coroutine_handle;

private:
	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;
};