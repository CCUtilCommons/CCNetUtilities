#include "socket_address.h"
#include <format>

std::string CNetUtils::ServerAddress::dump_self() const {
	return std::format("[localhost: {}]", port);
}

std::string CNetUtils::FullAddress::dump_self() const {
	return std::format("[{}: {}]", address, port);
}