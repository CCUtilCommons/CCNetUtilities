#pragma once

#include "bytes_helper.hpp"
#include <cstddef>
#include <utility>
namespace CNetUtils {
namespace http {

	using namespace CNetUtils::bytes_literals;
	/**
	 * @brief Server Configs will indicate the Parser Behaviours
	 * Like the length we can consume once, or the other sessions
	 */
	struct ServerConfig {
		std::size_t max_header_bytes = 64_KB; // max headers total
		size_t max_header_lines = 200; // max number of header lines
		size_t max_start_line = 4096; // max request line length
		size_t max_body_bytes = 16_MB; // max body size we'll accept in-memory
		size_t read_block = 4096;
		bool default_keep_alive_http11 = true; // HTTP/1.1 default

	private:
		friend class ServerConfigBuilder;
		ServerConfig() = default;
	};

	/**
	 * @brief Builder for ServerConfig
	 * Provides a fluent interface for setting configuration parameters.
	 */
	class ServerConfigBuilder {
	private:
		ServerConfig config_;

	public:
		ServerConfigBuilder() = default;

		/**
		 * @brief Sets the maximum total size of headers (in bytes).
		 */
		ServerConfigBuilder& setMaxHeaderBytes(std::size_t max_bytes) {
			config_.max_header_bytes = max_bytes;
			return *this;
		}

		/**
		 * @brief Sets the maximum number of header lines allowed.
		 */
		ServerConfigBuilder& setMaxHeaderLines(size_t max_lines) {
			config_.max_header_lines = max_lines;
			return *this;
		}

		/**
		 * @brief Sets the maximum length of the request line (e.g., "GET / HTTP/1.1").
		 */
		ServerConfigBuilder& setMaxStartLine(size_t max_line_len) {
			config_.max_start_line = max_line_len;
			return *this;
		}

		/**
		 * @brief Sets the maximum body size (in bytes) to accept in-memory.
		 */
		ServerConfigBuilder& setMaxBodyBytes(size_t max_bytes) {
			config_.max_body_bytes = max_bytes;
			return *this;
		}

		/**
		 * @brief Sets the size of the block read from the network in one go.
		 */
		ServerConfigBuilder& setReadBlock(size_t block_size) {
			config_.read_block = block_size;
			return *this;
		}

		/**
		 * @brief Sets whether HTTP/1.1 connections default to keep-alive.
		 */
		ServerConfigBuilder& setDefaultKeepAliveHttp11(bool keep_alive) {
			config_.default_keep_alive_http11 = keep_alive;
			return *this;
		}

		/**
		 * @brief Builds and returns the final ServerConfig object.
		 * @return ServerConfig The configured instance.
		 */
		operator ServerConfig() {
			return std::move(config_);
		}
	};
}
}
