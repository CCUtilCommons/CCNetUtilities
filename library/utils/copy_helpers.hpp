#pragma once
#include "library_utils.h"
#include <algorithm>
#include <string>
namespace CNetUtils {

CNETUTILS_FORCEINLINE std::string trim_copy(const std::string& s) {
	size_t a = 0, b = s.size();
	while (a < b && std::isspace((unsigned char)s[a]))
		++a;
	while (b > a && std::isspace((unsigned char)s[b - 1]))
		--b;
	return s.substr(a, b - a);
}

CNETUTILS_FORCEINLINE std::string to_lower_copy(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

}
