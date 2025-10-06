#pragma once
#include "library_utils.h"
#ifndef IO_MANAFER_INCLUDE_PREFER
#error "Dont include the implementary headers in your codes, "\
        "these is generated without plantable codes"
#endif

namespace CNetUtils {
using IOEventManager_Internal_t = int;
/**
 * @brief INVALID_FD marks the common invalid fd in implements
 *
 */
static constexpr const IOEventManager_Internal_t INVALID_HANDLE = -1;

/**
 * @brief simple and common judge if the raw is valid
 *
 * @param raw_fd
 * @return constexpr CNETUTILS_FORCEINLINE const
 */
static constexpr CNETUTILS_FORCEINLINE const bool
is_valid_iohandle_raw(const IOEventManager_Internal_t& raw_fd) {
	return raw_fd != INVALID_HANDLE;
}
}
