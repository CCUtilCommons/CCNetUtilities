#include "Task.hpp"
#include "coro_sys_socket.h"
#include "socket_address.h"
#include <cstring>
#include <format>
#include <iostream>
#include <memory>

Task<void> handle_client(std::shared_ptr<CNetUtils::CoroClientSocket> socket) {
	static constexpr const size_t BUFEFR_SIZE = 4096;
	std::cout << "OK, new client comes in!" << socket->dump_self().dump_self() << std::endl;
	const char* welcome = "Hello dude, press q <enter> to quit\n";
	co_await socket->async_write(welcome, strlen(welcome));

	char buf[BUFEFR_SIZE];
	while (true) {
		ssize_t n = co_await socket->async_read(buf, sizeof(buf));
		if (n == 2 && buf[0] == 'q') {
			std::cout << "Client missed away..." << std::endl;
			socket->close();
			co_return;
		}

		if (n > 0) {
			co_await socket->async_write(buf, n);
		} else {
			socket->close(); // no matter close or error, shutdown direct
			co_return;
		}
	}
}

int main() {
	CNetUtils::netport_t port = 7000;
	auto server_addr = CNetUtils::ServerAddress { port };
	auto server = std::make_shared<CNetUtils::CoroServerSocket>(server_addr);
	std::cout << std::format(
	    "Server will be accessed in {}\n", server_addr.dump_self());
	server->run_server(handle_client);
	server->close();
	return 0;
}
