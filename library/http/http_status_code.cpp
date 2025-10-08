#include "http_status_code.h"

namespace CNetUtils::http {

std::string_view reason_phrase(const HttpStatus s) noexcept {
	using enum HttpStatus;
	switch (s) {
	case OK:
		return "OK";
	case Created:
		return "Created";
	case NoContent:
		return "No Content";
	case BadRequest:
		return "Bad Request";
	case Unauthorized:
		return "Unauthorized";
	case Forbidden:
		return "Forbidden";
	case NotFound:
		return "Not Found";
	case MethodNotAllowed:
		return "Method Not Allowed";
	case RequestTimeout:
		return "Request Timeout";
	case LengthRequired:
		return "Length Required";
	case PayloadTooLarge:
		return "Payload Too Large";
	case URITooLong:
		return "URI Too Long";
	case UnsupportedMediaType:
		return "Unsupported Media Type";
	case TooManyRequests:
		return "Too Many Requests";
	case InternalServerError:
		return "Internal Server Error";
	case NotImplemented:
		return "Not Implemented";
	case BadGateway:
		return "Bad Gateway";
	case ServiceUnavailable:
		return "Service Unavailable";
	case GatewayTimeout:
		return "Gateway Timeout";
	default:
		return "Unknown Status";
	}
}

}
