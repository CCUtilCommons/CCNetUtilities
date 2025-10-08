#pragma once
#include "library_utils.h"
#include <string_view>
namespace CNetUtils {
namespace http {

	enum class HttpVersion {
		V1_0,
		V1_1,
		V_UNKNOWN
	};

	CNETUTILS_FORCEINLINE std::string_view version_string(HttpVersion v) noexcept {
		switch (v) {
		case HttpVersion::V1_0:
			return "HTTP/1.0";
		case HttpVersion::V1_1:
			return "HTTP/1.1";
		default:
			return "HTTP/?";
		}
	}

	CNETUTILS_FORCEINLINE HttpVersion from_string(std::string_view str) {
		if (str == "HTTP/1.0")
			return HttpVersion::V1_0;
		if (str == "HTTP/1.1")
			return HttpVersion::V1_1;
		return HttpVersion::V_UNKNOWN;
	}

	CNETUTILS_FORCEINLINE bool is_invalid_http_version(const HttpVersion v) {
		return v == HttpVersion::V_UNKNOWN;
	}

}
}
