#pragma once

#include "library_utils.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

/**
 * @brief Exception class derived from std::runtime_error specifically for socket operations.
 *
 * This exception encapsulates both a descriptive message and an optional system-level
 * error code (e.g., errno on POSIX or WSAGetLastError on Windows) for detailed diagnostics.
 */
class SocketException : public std::runtime_error {
public:
	/**
	 * @brief Constructs a SocketException with a message and an optional error code.
	 *
	 * @param message A descriptive error message.
	 * @param error_code The system-level error code (e.g., errno). Defaults to 0 (no code).
	 */
	SocketException(const std::string& message, int error_code = 0)
	    : std::runtime_error(build_message(message, error_code))
	    , native_error_code(error_code) { }

	/**
	 * @brief Gets the system error code associated with the exception.
	 *
	 * @return The error code, or 0 if no code was provided.
	 */
	CNETUTILS_FORCEINLINE int get_error_code() const noexcept {
		return native_error_code;
	}

private:
	int native_error_code;

	/**
	 * @brief Helper function to format the final exception message string.
	 *
	 * @param message The base message.
	 * @param error_code The system error code.
	 * @return The combined, formatted error string.
	 */
	static std::string build_message(const std::string& message, int error_code) {
		if (error_code == 0) {
			return message;
		}

		// Use std::system_category to get a human-readable description of the error code
		std::string system_error_desc;
		try {
			system_error_desc = std::system_category().message(error_code);
		} catch (...) {
			system_error_desc = "Unknown system error";
		}

		std::stringstream ss;
		ss << message
		   << " [Error Code: " << error_code
		   << " / Description: " << system_error_desc << "]";

		return ss.str();
	}
};

/**
 * @brief Base class for all specific socket operation errors.
 * * Note: These classes do not need to redefine the constructor or the 'what()' method,
 * as they automatically inherit the behavior of the SocketException base class.
 */

/**
 * @brief Exception thrown when the socket() function fails.
 */
class CreateError : public SocketException {
public:
	using SocketException::SocketException; // Inherit base class constructors
};

/**
 * @brief Exception thrown when the connect() function fails.
 */
class ConnectError : public SocketException {
public:
	using SocketException::SocketException;
};

/**
 * @brief Exception thrown when the bind() function fails.
 */
class BindError : public SocketException {
public:
	using SocketException::SocketException;
};

/**
 * @brief Exception thrown when the listen() function fails.
 */
class ListenError : public SocketException {
public:
	using SocketException::SocketException;
};

/**
 * @brief Exception thrown when the accept() function fails.
 */
class AcceptError : public SocketException {
public:
	using SocketException::SocketException;
};

/**
 * @brief Exception thrown when the send() or write() function fails.
 */
class SendError : public SocketException {
public:
	using SocketException::SocketException;
};

/**
 * @brief Exception thrown when the recv() or read() function fails.
 */
class ReceiveError : public SocketException {
public:
	using SocketException::SocketException;
};

/**
 * @brief Exception thrown when address resolution (e.g., getaddrinfo) fails.
 */
class AddressResolutionError : public SocketException {
public:
	using SocketException::SocketException;
};