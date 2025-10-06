#pragma once
#include "library_utils.h"
#ifndef SYNC_SOCKET_PREFER
#error "Dont include the implementary headers in your codes, "\
        "these is generated without plantable codes"
#endif

namespace CNetUtils {
using socket_raw_t = int;
/**
 * @brief INVALID_FD marks the common invalid fd in implements
 *
 */
static constexpr const socket_raw_t INVALID_FD = -1;

/**
 * @brief simple and common judge if the raw is valid
 *
 * @param raw_fd
 * @return constexpr CNETUTILS_FORCEINLINE const
 */
static constexpr CNETUTILS_FORCEINLINE const bool
is_valid_socket_raw(const socket_raw_t& raw_fd) {
	return raw_fd != INVALID_FD;
}
}
