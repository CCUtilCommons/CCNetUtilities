#pragma once
#include "http_headers.hpp"
#include "http_status_code.h"
#include "http_version.hpp"

namespace CNetUtils {
namespace http {

	struct Response {
		HttpVersion version = HttpVersion::V1_1;
		HttpStatus status = HttpStatus::OK;
		Headers headers;
		std::string body;
		bool use_chunked = false;

		/**
		 * @brief format the response type
		 *
		 * @return std::string
		 */
		std::string format_header() const;
	};

}
}
