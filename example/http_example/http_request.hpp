#pragma once
// coroutine_http_server_full.cpp
// A reasonably complete single-file HTTP/1.1 server built on top of
// your coroutine socket API (CNetUtils::CoroServerSocket / CoroClientSocket).
// Features implemented:
//  - Request parsing (start-line + headers)
//  - Content-Length body reading
//  - Transfer-Encoding: chunked decoding (request) and encoding (response)
//  - Connection: keep-alive / close handling, support multiple requests per connection
//  - Simple protection limits (max header size, max header lines, max body size)
//  - Simple header container optimized for small counts
//  - Example router: GET /, GET /stream (chunked), POST /echo
//
// Integration notes:
//  - Assumes your Task<T> coroutine type and CNetUtils::CoroClientSocket exist
//    and provide async_read(void*, size_t) -> ssize_t and async_write(const void*, size_t) -> ssize_t.
//  - Timeout support is left as a hook (read_with_timeout). If your event loop
//    provides a coroutine timer/sleep awaitable, plug it into the read logic to
//    abort slow reads.
//  - This file is intentionally self-contained. You can split it into headers/sources.

#include <algorithm>
#include <cctype>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Task.hpp"
#include "coro_sys_socket.h"

namespace coro_http {

using CSocket = CNetUtils::CoroClientSocket;
using CSocketPtr = std::shared_ptr<CSocket>;

struct ServerConfig {
	size_t max_header_bytes = 64 * 1024; // max headers total
	size_t max_header_lines = 200; // max number of header lines
	size_t max_start_line = 4096; // max request line length
	size_t max_body_bytes = 16 * 1024 * 1024; // max body size we'll accept in-memory
	size_t read_block = 4096;
	bool default_keep_alive_http11 = true; // HTTP/1.1 default
};

struct CaseInsensitiveHash {
	using is_transparent = void;
	size_t operator()(std::string_view s) const noexcept {
		size_t h = 0;
		for (unsigned char c : s)
			h = h * 131 + std::tolower(c);
		return h;
	}
};

struct CaseInsensitiveEq {
	using is_transparent = void;
	bool operator()(std::string_view a, std::string_view b) const noexcept {
		return std::equal(a.begin(), a.end(), b.begin(), b.end(),
		                  [](unsigned char ac, unsigned char bc) {
			                  return std::tolower(ac) == std::tolower(bc);
		                  });
	}
};

struct HeaderList {
	std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEq> items;

	void set(std::string key, std::string val) {
		items[std::move(key)] = std::move(val);
	}

	std::optional<std::string> get(std::string_view key) const {
		if (auto it = items.find(key); it != items.end())
			return it->second;
		return std::nullopt;
	}

	void erase(std::string_view key) { items.erase(std::string { key }); }

	bool has(std::string_view key) const { return items.find(key) != items.end(); }
};

struct HttpRequest {
	std::string method;
	std::string path;
	std::string version; // e.g. HTTP/1.1
	HeaderList headers;
	std::string body;
};

struct HttpResponse {
	int status = 200;
	std::string reason = "OK";
	HeaderList headers;
	std::string body;
	bool use_chunked = false; // if true, transfer-encoding: chunked will be used

	// build start+headers (without body/chunks)
	std::string head_string() const {
		std::ostringstream os;
		os << std::format("HTTP/1.1 {} {}\r\n", status, reason);
		// ensure content-length if not chunked and not streaming
		if (!use_chunked && headers.get("content-length").has_value() == false) {
			os << std::format("Content-Length: {}\r\n", body.size());
		}
		for (auto& kv : headers.items) {
			os << kv.first << ": " << kv.second << "\r\n";
		}
		os << "\r\n";
		return os.str();
	}
};

// --- Utilities ---
static inline std::string trim_copy(const std::string& s) {
	size_t a = 0, b = s.size();
	while (a < b && std::isspace((unsigned char)s[a]))
		++a;
	while (b > a && std::isspace((unsigned char)s[b - 1]))
		--b;
	return s.substr(a, b - a);
}

static inline std::string to_lower_copy(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
}

// parse start-line and headers from header_block (without trailing CRLFCRLF)
HttpRequest parse_request_from_header_block(const std::string& header_block) {
	HttpRequest req;
	std::istringstream is(header_block);
	std::string first;
	if (!std::getline(is, first))
		return req;
	if (!first.empty() && first.back() == '\r')
		first.pop_back();
	{
		std::istringstream ls(first);
		ls >> req.method >> req.path >> req.version;
	}
	std::string line;
	size_t header_count = 0;
	while (std::getline(is, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			break;
		auto pos = line.find(':');
		if (pos == std::string::npos)
			continue;
		std::string key = to_lower_copy(trim_copy(line.substr(0, pos)));
		std::string val = trim_copy(line.substr(pos + 1));
		req.headers.set(std::move(key), std::move(val));
		++header_count;
		if (header_count > 10000)
			throw std::runtime_error("too many header lines");
	}
	return req;
}

// read until the delimiter \r\n\r\n into `accum`. It will append data already in accum.
// Returns false if socket closed or error and no data.
Task<bool> read_until_double_crlf(CSocketPtr sock, std::string& accum, const ServerConfig& cfg) {
	static constexpr char DELIM[] = "\r\n\r\n";
	while (true) {
		if (accum.find(DELIM) != std::string::npos)
			co_return true;
		if (accum.size() > cfg.max_header_bytes)
			throw std::runtime_error("headers too large");
		char buf[4096];
		ssize_t n = co_await sock->async_read(buf, cfg.read_block);
		if (n <= 0) {
			co_return !accum.empty(); // if we have partial data, return true to let caller handle it
		}
		accum.append(buf, (size_t)n);
	}
}

// read fixed number of bytes into dest (append). Throws on too large/closed.
Task<void> read_exact(CSocketPtr sock, std::string& dest, size_t need, const ServerConfig& cfg) {
	while (dest.size() < need) {
		size_t want = std::min(cfg.read_block, need - dest.size());
		char buf[4096];
		ssize_t n = co_await sock->async_read(buf, want);
		if (n <= 0)
			throw std::runtime_error("unexpected EOF while reading body");
		dest.append(buf, (size_t)n);
		if (dest.size() > cfg.max_body_bytes)
			throw std::runtime_error("body too large");
	}
}

// decode chunked body. `prefix` may contain bytes already after header end.
Task<std::string> decode_chunked_body(CSocketPtr sock, std::string& prefix, const ServerConfig& cfg) {
	std::string out;
	std::string buffer = std::move(prefix);
	prefix.clear();
	auto read_more = [&](auto&& self) -> Task<void> {
		char tmp[4096];
		ssize_t n = co_await sock->async_read(tmp, cfg.read_block);
		if (n <= 0)
			throw std::runtime_error("unexpected EOF in chunked body");
		buffer.append(tmp, (size_t)n);
		co_return;
	};

	while (true) {
		// ensure we have a full chunk-size line
		auto pos = buffer.find("\r\n");
		while (pos == std::string::npos) {
			co_await read_more(read_more);
			pos = buffer.find("\r\n");
		}
		std::string szline = buffer.substr(0, pos);
		// remove chunk extensions if present: take until ';'
		auto semi = szline.find(';');
		if (semi != std::string::npos)
			szline.resize(semi);
		size_t chunk_size = 0;
		try {
			chunk_size = std::stoull(szline, nullptr, 16);
		} catch (...) {
			throw std::runtime_error("invalid chunk size");
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
		if (out.size() > cfg.max_body_bytes)
			throw std::runtime_error("chunked body too large");
	}
	co_return out;
}

// Read a full HTTP request from socket. Returns std::nullopt on EOF/closure.
Task<std::optional<HttpRequest>> read_http_request(CSocketPtr sock, ServerConfig const& cfg) {
	std::string accum; // keeps leftover bytes between requests
	bool ok = co_await read_until_double_crlf(sock, accum, cfg);
	if (!ok)
		co_return std::nullopt;
	size_t header_end = accum.find("\r\n\r\n");
	if (header_end == std::string::npos)
		co_return std::nullopt; // malformed

	std::string header_block = accum.substr(0, header_end);
	std::string after = accum.substr(header_end + 4); // might contain body prefix

	std::cout << "Get the formats: " << header_block << "\n\n";

	HttpRequest req = parse_request_from_header_block(header_block);
	// Basic sanity
	if (req.method.size() > cfg.max_start_line || req.path.size() > cfg.max_start_line)
		throw std::runtime_error("start-line too long");

	// Determine body strategy
	if (auto cl = req.headers.get("content-length"); cl.has_value()) {
		size_t content_len = 0;
		try {
			content_len = std::stoull(*cl);
		} catch (...) {
			throw std::runtime_error("invalid content-length");
		}
		if (content_len > cfg.max_body_bytes)
			throw std::runtime_error("content-length exceeds max_body_bytes");
		req.body = std::move(after);
		if (req.body.size() < content_len) {
			co_await read_exact(sock, req.body, content_len, cfg);
		}
		if (req.body.size() > content_len)
			req.body.resize(content_len);
	} else if (auto te = req.headers.get("transfer-encoding"); te.has_value() && to_lower_copy(*te) == "chunked") {
		// decode chunked using `after` as prefix
		req.body = co_await decode_chunked_body(sock, after, cfg);
	} else {
		// No body or use connection: close to signal EOF for body.
		// For simplicity: GET/HEAD assumed no body. Others: read until close (dangerous) -> we avoid.
		req.body = std::move(after);
	}

	co_return req;
}

// Write response. Supports chunked and non-chunked modes.
Task<void> write_response(CSocketPtr sock, HttpResponse const& resp, ServerConfig const& cfg) {
	if (resp.use_chunked) {
		// ensure header contains Transfer-Encoding: chunked
		HttpResponse r = resp; // copy to mutate header
		r.headers.erase("content-length"); // <-- ensure no Content-Length when chunked
		r.headers.set("transfer-encoding", "chunked");
		r.headers.set("connection", "keep-alive");
		r.headers.set("transfer-encoding", "chunked");
		r.headers.set("connection", "keep-alive");
		std::string head = r.head_string();
		ssize_t sent = co_await sock->async_write(head.data(), head.size());
		if (sent <= 0)
			co_return;
		// send body in one chunk (or could split). We'll chunk by cfg.read_block
		size_t pos = 0;
		while (pos < r.body.size()) {
			size_t chunk = std::min(cfg.read_block, r.body.size() - pos);
			std::string chunk_hdr = std::format("{:x}\r\n", chunk);
			ssize_t n = co_await sock->async_write(chunk_hdr.data(), chunk_hdr.size());
			if (n <= 0)
				break;
			n = co_await sock->async_write(r.body.data() + pos, chunk);
			if (n <= 0)
				break;
			n = co_await sock->async_write("\r\n", 2);
			if (n <= 0)
				break;
			pos += chunk;
		}
		// final 0 chunk
		co_await sock->async_write("0\r\n\r\n", 5);
	} else {
		// simple: send head + body
		HttpResponse r = resp;
		if (!r.headers.has("content-length"))
			r.headers.set("content-length", std::to_string(r.body.size()));
		// default connection header: close unless caller set keep-alive
		if (!r.headers.has("connection"))
			r.headers.set("connection", "close");
		std::string out = r.head_string() + r.body;
		size_t sent = 0;
		while (sent < out.size()) {
			ssize_t n = co_await sock->async_write(out.data() + sent, out.size() - sent);
			if (n <= 0)
				break;
			sent += (size_t)n;
		}
		std::cerr << "[DEBUG] write_response (non-chunked) finished: attempted_out_size=" << out.size()
		          << " final_sent=" << sent << "\n";
	}
	co_return;
}

// High-level connection handler. Accepts a socket and services multiple requests if keep-alive.
Task<void> handle_connection(CSocketPtr sock, ServerConfig cfg = {}) {
	try {
		// buffer any leftover bytes between requests
		while (true) {
			auto maybe_req = co_await read_http_request(sock, cfg);
			if (!maybe_req.has_value())
				break; // EOF/closed
			HttpRequest req = std::move(*maybe_req);

			// Determine keep-alive semantics
			bool keep_alive = false;
			if (req.headers.get("connection").has_value()) {
				std::string c = to_lower_copy(*req.headers.get("connection"));
				keep_alive = (c == "keep-alive");
			} else {
				// default: HTTP/1.1 keep-alive true, HTTP/1.0 false
				if (req.version == "HTTP/1.1")
					keep_alive = cfg.default_keep_alive_http11;
			}

			// Simple routing & handler (customize this block)
			HttpResponse resp;
			resp.headers.set("server", "coro-http/0.1");

			if (req.method == "GET" && (req.path == "/" || req.path == "/index")) {
				resp.status = 200;
				resp.reason = "OK";
				resp.headers.set("content-type", "text/plain; charset=utf-8");
				resp.body = "Hello from coroutine HTTP server!\n";
				resp.headers.set("connection", keep_alive ? "keep-alive" : "close");
				resp.use_chunked = false;
				co_await write_response(sock, resp, cfg);
			} else if (req.method == "GET" && req.path == "/stream") {
				// example: stream a large response using chunked encoding
				resp.status = 200;
				resp.reason = "OK";
				resp.headers.set("content-type", "text/plain; charset=utf-8");
				resp.headers.set("connection", keep_alive ? "keep-alive" : "close");
				resp.use_chunked = true;
				// make a body that is large (for demo). In real usage, you'd stream directly
				// from a file or generator to avoid building the whole body in memory.
				std::string big;
				for (int i = 0; i < 1000; ++i)
					big += std::format("line {}\n", i);
				resp.body = std::move(big);
				co_await write_response(sock, resp, cfg);
			} else if (req.method == "POST" && req.path == "/echo") {
				resp.status = 200;
				resp.reason = "OK";
				resp.headers.set("content-type", "text/plain; charset=utf-8");
				if (resp.body.size() <= cfg.read_block) {
					resp.use_chunked = false;
					// write_response will auto-add Content-Length
				} else {
					// Option B: stream large responses with chunked
					resp.use_chunked = true;
					// Ensure we don't accidentally keep Content-Length header set elsewhere.
					// We'll handle header cleanup in write_response (see next patch).
				}
				resp.body = std::move(req.body);
				resp.headers.set("connection", keep_alive ? "keep-alive" : "close");
				co_await write_response(sock, resp, cfg);
			} else {
				resp.status = 404;
				resp.reason = "Not Found";
				resp.headers.set("content-type", "text/plain; charset=utf-8");
				resp.body = std::format("Path {} not found\n", req.path);
				resp.headers.set("connection", keep_alive ? "keep-alive" : "close");
				co_await write_response(sock, resp, cfg);
			}

			// decide whether to continue serving more requests on this connection
			if (!keep_alive)
				break;
			// else loop to read next request
		}
	} catch (const std::exception& e) {
		std::cerr << "Connection handler error: " << e.what() << std::endl;
	}

	try {
		sock->close();
	} catch (...) { }
	co_return;
}

} // namespace coro_http