#pragma once
#include <string>
namespace CNetUtils {
using netport_t = unsigned short;

struct SocketAddress {
	virtual ~SocketAddress() = default;
	virtual std::string dump_self() const = 0;
};

struct ServerAddress : public SocketAddress {
	netport_t port;
	ServerAddress(const netport_t port)
	    : port(port) { }
	std::string dump_self() const override;
};

struct FullAddress : public SocketAddress {
	std::string address;
	netport_t port;
	FullAddress(const std::string& ip, netport_t port)
	    : address(ip)
	    , port(port) { }
	FullAddress(std::string&& ip, netport_t port)
	    : address(std::move(ip))
	    , port(port) { }
	std::string dump_self() const override;
};

}
