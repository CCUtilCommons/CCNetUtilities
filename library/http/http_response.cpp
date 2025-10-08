#include "http_response.hpp"
#include "http_defines.h"
#include <format>
#include <sstream>

namespace CNetUtils::http {

std::string Response::format_header() const {
	std::ostringstream os;
	os << std::format("{} {} {}{}",
	                  version_string(version),
	                  static_cast<int>(status),
	                  reason_phrase(status),
	                  TERMINATE);

	if (!use_chunked && !headers.get("content-length").has_value()) {
		os << std::format("Content-Length: {}{}", body.size(), TERMINATE);
	}

	for (auto& kv : headers.items)
		os << kv.first << ": " << kv.second << TERMINATE;
	os << TERMINATE;
	return os.str();
}

}
