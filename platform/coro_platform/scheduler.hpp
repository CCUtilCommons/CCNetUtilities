#pragma once

#include "library_utils.h"
#include "single_instance.hpp"
#include <chrono>
#include <coroutine>
#include <queue>

#define IO_MANAFER_INCLUDE_PREFER
#include "coro_platform_impl.h"

template <typename T>
class Task;
struct AwaitableSleep;

/**
 * @brief   Scheduler will Schedule the co-routines for the sessions
 *          So, IO Manager will
 *
 */
class Scheduler : public SingleInstance<Scheduler> {
public:
	using sch_tp_t = std::chrono::steady_clock::time_point;
	using coro_handle_t = std::coroutine_handle<>;
	friend SingleInstance<Scheduler>; // for friend sessions
	friend class AwaitableSleep; // for sleep call
	template <typename T>
	friend class Task; // Task can access the internals for conv

	CNETUTILS_FORCEINLINE static void run() { instance().__run(); }

	template <typename Task_RType>
	static void spawn(Task<Task_RType>&& task);

	~Scheduler() override {
		run();
	}

private:
	/**
	 * @brief 	To support Sleep, we need to defines a convient types
	 *			for the Schedular to decide which one we need sleep
	 *
	 */
	struct SleepItem {
		SleepItem(coro_handle_t h,
		          sch_tp_t tp);

		std::chrono::steady_clock::time_point sleep;
		std::coroutine_handle<> coro_handle;

		CNETUTILS_FORCEINLINE bool operator<(
		    const SleepItem& other) const noexcept {
			return sleep > other.sleep;
		}
	};
	std::queue<std::coroutine_handle<>> ready_coroutines;
	std::priority_queue<SleepItem> sleepys;

private:
	Scheduler() = default;
	CNETUTILS_FORCEINLINE sch_tp_t
	current() const noexcept { return std::chrono::steady_clock::now(); }
	/**
	 * @brief Helpers for find the max sleep await out
	 *
	 * @return int
	 */
	int caculate_time_out() const noexcept;

	/**
	 * @brief Spawn internal calls
	 *
	 * @param h
	 * @return CNETUTILS_FORCEINLINE
	 */
	CNETUTILS_FORCEINLINE void internal_spawn(coro_handle_t h) {
		ready_coroutines.push(h);
	}

	/**
	 * @brief Task helpers for the sleeps, not open as
	 *		  user is expected to call await sleep(ms)
	 *
	 * @param which
	 * @param until_when
	 * @return CNETUTILS_FORCEINLINE
	 */
	CNETUTILS_FORCEINLINE void sleep_until(
	    coro_handle_t which, sch_tp_t until_when) { sleepys.emplace(which, until_when); }

	/**
	 * @brief 	Actual Run and de-wrapper from instance of
	 *			run() call
	 */
	void __run();

	/**
	 * @brief in_spawn a type for the schedular
	 *
	 * @tparam Task_RType
	 * @param task
	 */
	template <typename Task_RType>
	void __spawn(Task<Task_RType>&& task);
};

/**
 * @brief Sleep call awaitable
 *
 */
struct AwaitableSleep {
	AwaitableSleep(std::chrono::milliseconds how_long)
	    : duration(how_long)
	    , wake_time(std::chrono::steady_clock::now() + how_long) { }

	/**
	 * @brief await_ready always lets the sessions sleep!
	 *
	 */
	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> h) {
		Scheduler::instance().sleep_until(h, wake_time);
	}

	void await_resume() { }

private:
	std::chrono::milliseconds duration;
	std::chrono::steady_clock::time_point wake_time;
};

CNETUTILS_FORCEINLINE AwaitableSleep sleep(std::chrono::milliseconds s) {
	return { s };
}

#include "Task.hpp"
template <typename T>
inline void Scheduler::__spawn(Task<T>&& task) {
	internal_spawn(task.coroutine_handle);
	task.coroutine_handle = nullptr;
}

template <typename Task_RType>
inline void Scheduler::spawn(Task<Task_RType>&& task) {
	instance().__spawn(std::move(task));
}