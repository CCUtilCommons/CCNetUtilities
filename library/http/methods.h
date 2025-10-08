#pragma once
#include "library_utils.h"
#include <string_view>

namespace CNetUtils {

namespace http {

	/**
	 * @brief Enum class representing the standard HTTP Request Methods (or Verbs).
	 *
	 * This enumeration defines the type of action the client wishes to perform on the
	 * resource identified by the Request-URI (Uniform Resource Identifier).
	 * Each method is defined by HTTP standards (e.g., RFC 7231, RFC 5789) and carries
	 * specific semantics, especially regarding safety and idempotency.
	 *
	 * @note Using 'enum class' (C++11) provides strong typing and scope, preventing
	 * name collisions (e.g., always accessed as HttpMethod::GET).
	 */
	enum class HttpMethod {
		/**
		 * @brief The **GET** method requests a representation of the specified resource.
		 *
		 * GET requests should only be used for data retrieval and **must not** have
		 * any side effects other than those which are naturally harmless (e.g., logging).
		 * - **Safety**: Yes (Does not alter server state).
		 * - **Idempotency**: Yes (Repeat requests yield the same result).
		 * - **Typical Use**: Fetching a webpage, retrieving a resource's details.
		 */
		GET,

		/**
		 * @brief The **POST** method submits data to be processed to the identified resource.
		 *
		 * The data is included in the request body. POST often causes a state change or
		 * the creation of a new resource on the server.
		 * - **Safety**: No (Alters server state).
		 * - **Idempotency**: No (Repeat requests may create multiple resources).
		 * - **Typical Use**: Submitting form data, uploading a file, creating a new record.
		 */
		POST,

		/**
		 * @brief The **PUT** method requests that the enclosed entity be stored under
		 * the supplied Request-URI.
		 *
		 * It **replaces** all current representations of the target resource with the
		 * content provided in the request body.
		 * - **Safety**: No (Alters server state).
		 * - **Idempotency**: Yes (Repeat requests are harmless as they just overwrite
		 * the resource with the same data).
		 * - **Typical Use**: Updating an existing resource completely, or creating it
		 * if it doesn't exist at the given URI.
		 */
		PUT,

		/**
		 * @brief The **DELETE** method requests that the resource identified by the
		 * Request-URI be deleted.
		 *
		 * - **Naming Note**: The trailing underscore (`_`) is used here to avoid
		 * conflict with the C++ keyword `delete`.
		 * - **Safety**: No (Alters server state).
		 * - **Idempotency**: Yes (Deleting an already deleted resource results in the
		 * same state: the resource is gone).
		 * - **Typical Use**: Removing a database record or file.
		 */
		DELETE_,

		/**
		 * @brief The **HEAD** method is identical to GET, but **transfers the status line
		 * and header fields only, without the message body**.
		 *
		 * It's useful for retrieving metadata about a resource (like content type,
		 * content length) without transferring the entire resource.
		 * - **Safety**: Yes.
		 * - **Idempotency**: Yes.
		 * - **Typical Use**: Checking resource existence, checking content type before
		 * downloading, validating resource modification time.
		 */
		HEAD,

		/**
		 * @brief The **OPTIONS** method requests information about the communication
		 * options available for the target resource.
		 *
		 * The response typically includes an `Allow` header listing the methods
		 * the server supports for that resource (e.g., "Allow: GET, POST, HEAD").
		 * - **Safety**: Yes.
		 * - **Idempotency**: Yes.
		 * - **Typical Use**: Cross-Origin Resource Sharing (CORS) preflight requests.
		 */
		OPTIONS,

		/**
		 * @brief The **PATCH** method is used to apply partial modifications to a resource.
		 *
		 * Unlike PUT, which replaces the entire resource, PATCH only modifies the
		 * specific parts indicated in the request body. (Defined in RFC 5789).
		 * - **Safety**: No.
		 * - **Idempotency**: No (Unless implemented carefully by the server to be idempotent).
		 * - **Typical Use**: Updating a user's single field (e.g., changing only their email).
		 */
		PATCH,

		/**
		 * @brief Represents an unknown, unsupported, or invalid HTTP method.
		 *
		 * This value is typically used as a sentinel when parsing a request line
		 * where the method field does not match any standard or supported verb.
		 */
		UNKNOWN
	};

	/**
	 * @brief from http methods enum to string_view for light conversions
	 *
	 * @param m
	 * @return std::string_view
	 */
	std::string_view to_string(const HttpMethod m) noexcept;

	/**
	 * @brief parse_http_method parses a string to possible HttpMethod
	 *
	 * @param s
	 * @return HttpMethod
	 */
	HttpMethod parse_http_method(const std::string_view s) noexcept;

	CNETUTILS_FORCEINLINE bool is_unknown_method(const HttpMethod m) noexcept {
		return m == HttpMethod::UNKNOWN;
	}

	CNETUTILS_FORCEINLINE bool is_invalid_method(const HttpMethod m) noexcept {
		return is_unknown_method(m);
	}

}

}
