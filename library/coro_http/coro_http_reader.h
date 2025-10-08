#include "Task.hpp"
#include "bytes_helper.hpp"
#include "coro_sys_socket.h"
#include "http/http_request.h"
#include "http/http_server_config.h"
#include <cstddef>
#include <memory>

namespace CNetUtils {
namespace coro_http {
	using namespace CNetUtils::bytes_literals;
	class HttpReader {
		/**
		 * @brief MAX_TEMP_READ_BUFFER helps one read
		 *
		 */
		static constexpr const size_t MAX_TEMP_READ_BUFFER = 4_KB;

	public:
		explicit HttpReader(
		    std::shared_ptr<CNetUtils::CoroClientSocket> sock,
		    const http::ServerConfig& cfg)
		    : sock_(sock)
		    , cfg_(cfg) { }

		Task<std::optional<http::Request>> read_request();

	private:
		std::shared_ptr<CNetUtils::CoroClientSocket> sock_;
		http::ServerConfig cfg_;
		std::string accum_;

	private:
		Task<bool> read_until_double_crlf();
		Task<void> read_exact(std::string& dest, size_t need);
		Task<std::string> decode_chunked_body(std::string& prefix);
	};

}
};
