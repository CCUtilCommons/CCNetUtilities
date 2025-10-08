#pragma once
#include "http/http_exceptions.h"
#include "http/http_request.h"
namespace CNetUtils::http {
using query_map_t = Request::query_map_t;

DECLEAR_DEFAULT_EXCEPTIONS(RequestJsonParseFailed);

/**
 * @brief consume the json string to the query_map_t
 *
 * @param json_string
 * @return query_map_t
 */
query_map_t from_json_string(const std::string json_string);

}
