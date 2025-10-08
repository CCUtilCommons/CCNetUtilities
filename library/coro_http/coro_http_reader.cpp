#include "coro_http_reader.h"
#include "copy_helpers.hpp"
#include "http/http_defines.h"
#include "http/http_exceptions.h"
#include "http/http_request.h"
#include <format>

namespace CNetUtils::coro_http {
Task<bool> HttpReader::read_until_double_crlf() {
	while (true) {
		if (accum_.find(http::ALL_SEG_TERMINATE) != std::string::npos)
			co_return true; // OK, read this
		if (accum_.size() > cfg_.max_header_bytes)
			throw http::HttpHeaderTooLarge("headers too large");
		char buf[MAX_TEMP_READ_BUFFER];
		ssize_t n = co_await sock_->async_read(buf, cfg_.read_block);
		if (n <= 0)
			co_return !accum_.empty();
		accum_.append(buf, (size_t)n);
	}
}

Task<void> HttpReader::read_exact(std::string& dest, size_t need) {
	while (dest.size() < need) {
		size_t want = std::min(cfg_.read_block, need - dest.size());
		char buf[MAX_TEMP_READ_BUFFER];
		ssize_t n = co_await sock_->async_read(buf, want);
		if (n <= 0)
			throw http::HttpReaderBodyError("unexpected EOF while reading body");
		dest.append(buf, (size_t)n);
		if (dest.size() > cfg_.max_body_bytes)
			throw http::HttpReaderBodyError("body too large");
	}
}

Task<std::string> HttpReader::decode_chunked_body(std::string& prefix) {
	// resets and prepare sessions
	std::string out;
	std::string buffer = std::move(prefix);
	prefix.clear();

	auto read_more = [&](auto&& self) -> Task<void> {
		char tmp[4096];
		ssize_t n = co_await sock_->async_read(tmp, cfg_.read_block);
		if (n <= 0)
			throw http::HttpReaderBodyError("unexpected EOF in chunked body");
		buffer.append(tmp, (size_t)n);
		co_return;
	};

	while (true) {
		// ensure we have a full chunk-size line
		auto pos = buffer.find(http::TERMINATE);
		while (pos == std::string::npos) {
			co_await read_more(read_more); // Await for more...
			pos = buffer.find(http::TERMINATE); // adjust the positions
		}

		std::string szline = buffer.substr(0, pos);
		// remove chunk extensions if present: take until ';'
		auto semi = szline.find(';');
		if (semi != std::string::npos)
			szline.resize(semi);

		size_t chunk_size = 0;
		try {
			// find the size
			chunk_size = std::stoull(szline, nullptr, 16);
		} catch (...) {
			throw http::HttpChunkError("invalid chunk size");
		}

		buffer.erase(0, pos + 2); // remove size line + CRLF

		if (chunk_size == 0) {
			// final chunk. consume trailing CRLF after last chunk (could be followed by trailers, but we ignore)
			// ensure we have at least "\r\n"
			while (buffer.size() < 2)
				co_await read_more(read_more);
			if (buffer.rfind("\r\n", 0) == 0) {
				buffer.erase(0, 2);
			} else if (buffer.size() >= 2) {
				buffer.erase(0, 2);
			}
			break; // done
		}

		// ensure we have chunk_size + CRLF
		while (buffer.size() < chunk_size + 2) {
			co_await read_more(read_more);
		}

		out.append(buffer.data(), chunk_size);
		// remove chunk data + trailing CRLF
		buffer.erase(0, chunk_size + 2);
		if (out.size() > cfg_.max_body_bytes)
			throw http::HttpChunkError("chunked body too large");
	}
	co_return out; // Decode down
}

Task<std::optional<http::Request>> HttpReader::read_request() {
	bool ok = co_await read_until_double_crlf();
	if (!ok)
		co_return std::nullopt; // Not a valid!

	size_t header_end = accum_.find(http::ALL_SEG_TERMINATE);
	if (header_end == std::string::npos)
		co_return std::nullopt; // malformed

	std::string header_block = accum_.substr(0, header_end);
	std::string after = accum_.substr(header_end + 4); // might contain body prefix

	http::Request req { header_block };

	if (req.path.size() > cfg_.max_start_line)
		throw http::HttpRequestPathError(
		    std::format(
		        "request path too long, Get: {} > {}",
		        req.path.size(), cfg_.max_start_line));

	// Determine body strategy
	auto body_content_length = req.headers.get("content-length");
	auto transfer_encoding = req.headers.get("transfer-encoding");

	if (body_content_length.has_value()) {
		// parse the content length
		size_t content_len = 0;
		try {
			content_len = std::stoull(*body_content_length);
		} catch (...) {
			throw http::HttpReaderBodyError("invalid content-length");
		}

		if (content_len > cfg_.max_body_bytes)
			throw http::HttpReaderBodyError("content-length exceeds max_body_bytes");

		req.body = std::move(after);

		if (req.body.size() < content_len) {
			co_await read_exact(req.body, content_len);
		}
		if (req.body.size() > content_len)
			req.body.resize(content_len);

	} else if (
	    transfer_encoding.has_value()
	    && CNetUtils::to_lower_copy(*transfer_encoding) == "chunked") {
		// decode chunked using `after` as prefix
		req.body = co_await decode_chunked_body(after);
	} else {
		// No body or use connection: close to signal EOF for body.
		// For simplicity: GET/HEAD assumed no body. Others: read until close (dangerous) -> we avoid.
		req.body = std::move(after);
	}

	co_return req;
}
}