#pragma once
#include "Task.hpp"
#include "coro_sys_socket.h"
#include "http/http_response.hpp"
#include "http/http_server_config.h"

namespace CNetUtils {
namespace coro_http {
	class HttpWriter {
	public:
		explicit HttpWriter(
		    std::shared_ptr<CNetUtils::CoroClientSocket> sock,
		    const http::ServerConfig& cfg)
		    : sock_(std::move(sock))
		    , cfg_(cfg) { }

		Task<void> write_response(const http::Response& resp);

	private:
		std::shared_ptr<CNetUtils::CoroClientSocket> sock_;
		const http::ServerConfig& cfg_;

	private:
		/**
		 * @brief Write the http clients with no chunked
		 *
		 * @param resp
		 * @return Task<void>
		 */
		Task<void> write_nonchunked(const http::Response& resp);
		/**
		 * @brief Write the http clients with chunked
		 *
		 * @param resp
		 * @return Task<void>
		 */
		Task<void> write_chunked(const http::Response& resp);
	};

}
}