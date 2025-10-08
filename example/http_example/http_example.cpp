#include "http_request.hpp"

Task<void> handle_client(std::shared_ptr<CNetUtils::CoroClientSocket> socket) {
	return coro_http::handle_connection(socket, coro_http::ServerConfig {});
}

int main() {
	CNetUtils::netport_t port = 7000;
	auto server_addr = CNetUtils::ServerAddress { port };
	auto server = std::make_shared<CNetUtils::CoroServerSocket>(server_addr);
	server->run_server(handle_client);
	server->close();
	return 0;
}