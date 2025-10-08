#pragma once
#include "copy_helpers.hpp"
#include "library_utils.h"
#include <string>
namespace CNetUtils {
namespace http {

	CNETUTILS_FORCEINLINE bool
	content_type_contains(const std::string& ct, const std::string& sub) {
		std::string a = to_lower_copy(ct);
		std::string b = to_lower_copy(sub);
		return a.find(b) != std::string::npos;
	}

}
}
