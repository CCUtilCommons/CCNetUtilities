#pragma once

namespace CNetUtils {

using bytes_count_t = unsigned long long;

constexpr bytes_count_t __B = 1;
constexpr bytes_count_t __KB = __B * 1024;
constexpr bytes_count_t __MB = __KB * 1024;
constexpr bytes_count_t __GB = __MB * 1024;

namespace bytes_literals {
	constexpr bytes_count_t operator"" _B(unsigned long long bs) {
		return bs * __B;
	}

	constexpr bytes_count_t operator"" _KB(unsigned long long kbs) {
		return kbs * __KB;
	}

	constexpr bytes_count_t operator"" _MB(unsigned long long mbs) {
		return mbs * __MB;
	}

	constexpr bytes_count_t operator"" _GB(unsigned long long gbs) {
		return gbs * __GB;
	}
}

}
