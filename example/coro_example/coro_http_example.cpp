#include "Task.hpp"
#include "coro_http/coro_http_reader.h"
#include "coro_http/coro_http_writer.h"
#include "coro_sys_socket.h"
#include "http/http_request.h"
#include "http/http_response.hpp"
#include "http/http_status_code.h"
#include "http/methods.h"
#include <iostream>
#include <memory>

using namespace CNetUtils;

// High-level connection handler. Accepts a socket and services multiple requests if keep-alive.
Task<void> handle_connection(
    std::shared_ptr<CNetUtils::CoroClientSocket> sock,
    CNetUtils::http::ServerConfig&& cfg) {
	CNetUtils::http::ServerConfig config = std::move(cfg);

	auto make_response = [&](http::Request& req, CNetUtils::http::HttpStatus status, std::string body, bool chunked = false) {
		http::Response resp;
		resp.status = status;
		resp.body = std::move(body);
		resp.use_chunked = chunked;
		resp.headers.set("server", "coro-http/0.1");
		resp.headers.set("content-type", "text/plain; charset=utf-8");
		resp.headers.set("connection", req.isKeepAlive ? "keep-alive" : "close");
		return resp;
	};

	try {
		while (true) {
			coro_http::HttpReader reader(sock, config);
			auto maybe_req = co_await reader.read_request();
			if (!maybe_req.has_value())
				break;

			http::Request req = std::move(*maybe_req);
			coro_http::HttpWriter writer(sock, config);
			http::Response resp;

			if (req.method == CNetUtils::http::HttpMethod::GET) {
				if (req.path == "/" || req.path == "/index") {
					resp = make_response(req, CNetUtils::http::HttpStatus::OK, "Hello from coroutine HTTP server!\n");
				} else if (req.path == "/stream") {
					std::string big;
					for (int i = 0; i < 1000; ++i)
						big += std::format("line {}\n", i);
					resp = make_response(req, CNetUtils::http::HttpStatus::OK, std::move(big), true);
				} else {
					resp = make_response(req, CNetUtils::http::HttpStatus::NotFound,
					                     std::format("Path {} not found\n", req.path));
				}
			} else if (req.method == CNetUtils::http::HttpMethod::POST && req.path == "/echo") {
				bool use_chunked = req.body.size() > cfg.read_block;
				resp = make_response(req, CNetUtils::http::HttpStatus::OK, std::move(req.body), use_chunked);
			} else {
				resp = make_response(req, CNetUtils::http::HttpStatus::NotFound,
				                     std::format("Path {} not found\n", req.path));
			}

			co_await writer.write_response(resp);

			if (!req.isKeepAlive)
				break;
		}
	} catch (const std::exception& e) {
		std::cerr << "Connection handler error: " << e.what() << std::endl;
	}

	try {
		sock->close();
	} catch (...) { }
	co_return;
}

Task<void> handle_client(std::shared_ptr<CNetUtils::CoroClientSocket> socket) {
	return handle_connection(socket, http::ServerConfigBuilder());
}

int main() {
	CNetUtils::netport_t port = 7000;
	auto server_addr = CNetUtils::ServerAddress { port };
	auto server = std::make_shared<CNetUtils::CoroServerSocket>(server_addr);
	server->run_server(handle_client);
	server->close();
	return 0;
}