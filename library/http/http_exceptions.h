#pragma once
#include "library_utils.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace CNetUtils {
namespace http {

	class HttpException : public std::runtime_error {
	public:
		/**
		 * @brief Constructs a SocketException with a message and an optional error code.
		 *
		 * @param message A descriptive error message.
		 * @param error_code The system-level error code (e.g., errno). Defaults to 0 (no code).
		 */
		HttpException(const std::string& message, int error_code = 0)
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

#define DECLEAR_DEFAULT_EXCEPTIONS(ExceptionType) \
	class ExceptionType : public HttpException {  \
	public:                                       \
		using HttpException::HttpException;       \
	}

	/**
	 * @brief   HttpRequestParseError damns happens when in
	 *          parsing the http request parsing sessions
	 */
	DECLEAR_DEFAULT_EXCEPTIONS(HttpRequestParseError);

	/**
	 * @brief HttpHeaderTooLarge throws when the header incomes overflow
	 *
	 */
	DECLEAR_DEFAULT_EXCEPTIONS(HttpHeaderTooLarge);

	/**
	 * @brief HttpReaderBodyError throws when the body parse fatals
	 *
	 */
	DECLEAR_DEFAULT_EXCEPTIONS(HttpReaderBodyError);

	/**
	 * @brief When in chunks modes
	 *
	 */
	DECLEAR_DEFAULT_EXCEPTIONS(HttpChunkError);

	/**
	 * @brief HttpRequestPathError hanpens in parsing request path
	 *
	 */
	DECLEAR_DEFAULT_EXCEPTIONS(HttpRequestPathError);

	/**
	 * @brief UrlDecodingError happens when decodings fatal
	 *
	 */
	DECLEAR_DEFAULT_EXCEPTIONS(UrlDecodingError);

	DECLEAR_DEFAULT_EXCEPTIONS(FailedParseForm);

}
}
