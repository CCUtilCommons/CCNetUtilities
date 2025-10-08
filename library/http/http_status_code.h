#pragma once
#include <string_view>

namespace CNetUtils {
namespace http {

	enum class HttpStatus : int {
		OK = 200,
		Created = 201,
		NoContent = 204,
		BadRequest = 400,
		Unauthorized = 401,
		Forbidden = 403,
		NotFound = 404,
		MethodNotAllowed = 405,
		RequestTimeout = 408,
		LengthRequired = 411,
		PayloadTooLarge = 413,
		URITooLong = 414,
		UnsupportedMediaType = 415,
		TooManyRequests = 429,
		InternalServerError = 500,
		NotImplemented = 501,
		BadGateway = 502,
		ServiceUnavailable = 503,
		GatewayTimeout = 504,
	};

	std::string_view reason_phrase(const HttpStatus s) noexcept;

}
}
