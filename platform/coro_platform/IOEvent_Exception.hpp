#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <system_error>
/**
 * @file io_exceptions.hpp
 * @brief Exception hierarchy for epoll/poll/kqueue based I/O management errors.
 *
 * This file defines a hierarchy of exceptions used by IOManager and related
 * asynchronous I/O components. Each exception includes an optional system-level
 * error code (e.g., errno) for detailed diagnostics and debugging.
 */

class IOManagerException : public std::runtime_error {
public:
	/**
	 * @brief Construct an IOManagerException with a message and optional system error code.
	 *
	 * @param message Descriptive error message.
	 * @param error_code System-level error code (e.g., errno). Defaults to 0.
	 */
	explicit IOManagerException(const std::string& message, int error_code = 0)
	    : std::runtime_error(build_message(message, error_code))
	    , native_error_code(error_code) { }

	/**
	 * @brief Retrieve the native error code.
	 */
	int get_error_code() const noexcept { return native_error_code; }

protected:
	int native_error_code;

private:
	static std::string build_message(const std::string& message, int error_code) {
		if (error_code == 0)
			return message;

		std::string system_msg;
		try {
			system_msg = std::system_category().message(error_code);
		} catch (...) {
			system_msg = "Unknown system error";
		}
		return std::format("{} [error_code = {} : {}]", message, error_code, system_msg);
	}
};

// ========================== 具体异常类型 ===============================

/**
 * @brief Raised when epoll_create1() fails.
 */
class EpollCreateError : public IOManagerException {
public:
	using IOManagerException::IOManagerException;
};

/**
 * @brief Raised when epoll_ctl() fails for ADD/MOD/DEL operations.
 */
class EpollCtlError : public IOManagerException {
public:
	using IOManagerException::IOManagerException;
};

/**
 * @brief Raised when epoll_wait() fails unexpectedly.
 */
class EpollWaitError : public IOManagerException {
public:
	using IOManagerException::IOManagerException;
};

/**
 * @brief Raised when you pass a non support Event.
 *
 */
class EpollUnsupportiveEvent : public IOManagerException {
public:
	using IOManagerException::IOManagerException;
};

/**
 * @brief Raised when attempting to operate on an invalid or closed file descriptor.
 */
class InvalidFDException : public IOManagerException {
public:
	using IOManagerException::IOManagerException;
};
