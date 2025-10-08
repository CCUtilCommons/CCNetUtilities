#include "coro_http_writer.h"
#include "http/http_defines.h"
#include "http/http_response.hpp"
namespace CNetUtils::coro_http {

Task<void> HttpWriter::write_nonchunked(const http::Response& resp) {
	http::Response r = resp;
	if (!r.headers.has("content-length"))
		r.headers.set("content-length", std::to_string(r.body.size()));
	if (!r.headers.has("connection"))
		r.headers.set("connection", "close");
	std::string out = r.format_header() + r.body;

	size_t sent = 0;
	while (sent < out.size()) {
		ssize_t n = co_await sock_->async_write(out.data() + sent, out.size() - sent);
		if (n <= 0)
			break;
		sent += (size_t)n;
	}
}

Task<void> HttpWriter::write_chunked(const http::Response& resp) {
	http::Response r = resp;
	r.headers.erase("content-length");
	r.headers.set("transfer-encoding", "chunked");
	r.headers.set("connection", "keep-alive");

	std::string head = r.format_header();
	ssize_t sent = co_await sock_->async_write(head.data(), head.size());
	if (sent <= 0)
		co_return;

	size_t pos = 0;
	while (pos < r.body.size()) {
		size_t chunk = std::min(cfg_.read_block, r.body.size() - pos);
		std::string chunk_hdr = std::format("{:x}{}", chunk, http::TERMINATE);
		co_await sock_->async_write(chunk_hdr.data(), chunk_hdr.size());
		co_await sock_->async_write(r.body.data() + pos, chunk);
		co_await sock_->async_write(http::TERMINATE, 2);
		pos += chunk;
	}

	co_await sock_->async_write("0\r\n\r\n", 5);
}

Task<void> HttpWriter::write_response(const http::Response& resp) {
	if (resp.use_chunked) {
		co_await write_chunked(resp);
	} else {
		co_await write_nonchunked(resp);
	}
}

}
