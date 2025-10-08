#include "methods.h"
#include <unordered_map>

namespace CNetUtils::http {

namespace {
	static const std::unordered_map<std::string_view, HttpMethod> map = {
		{ "GET", HttpMethod::GET },
		{ "POST", HttpMethod::POST },
		{ "PUT", HttpMethod::PUT },
		{ "DELETE", HttpMethod::DELETE_ },
		{ "HEAD", HttpMethod::HEAD },
		{ "OPTIONS", HttpMethod::OPTIONS },
		{ "PATCH", HttpMethod::PATCH }
	};
}

/**
 * @brief from http methods enum to string_view for light conversions
 *
 * @param m
 * @return std::string_view
 */
std::string_view to_string(const HttpMethod m) noexcept {
	switch (m) {
	case HttpMethod::GET:
		return "GET";
	case HttpMethod::POST:
		return "POST";
	case HttpMethod::PUT:
		return "PUT";
	case HttpMethod::DELETE_:
		return "DELETE";
	case HttpMethod::HEAD:
		return "HEAD";
	case HttpMethod::OPTIONS:
		return "OPTIONS";
	case HttpMethod::PATCH:
		return "PATCH";
	default:
		return "UNKNOWN";
	}
}

/**
 * @brief parse_http_method parses a string to possible HttpMethod
 *
 * @param s
 * @return HttpMethod
 */
HttpMethod parse_http_method(const std::string_view s) noexcept {
	if (auto it = map.find(s); it != map.end())
		return it->second;
	return HttpMethod::UNKNOWN;
}

}
