#pragma once
#include "http_headers.hpp"
#include "http_version.hpp"
#include "methods.h"
#include <cstddef>

namespace CNetUtils {
namespace http {

	namespace RequestParseLimitations {
		static constexpr const size_t MAX_LINE = 10000; // can not be too long dude
	}

	struct Request {
		HttpMethod method { HttpMethod::UNKNOWN };
		std::string path {};
		HttpVersion version {};
		Headers headers;
		std::string body {};
		bool isKeepAlive { true };

		Request() = default;
		/**
		 * @brief 	Construct a new Request object
		 *			by given a build_block strings
		 *
		 * @param header_block
		 */
		Request(const std::string& header_block);
	};

}
}
