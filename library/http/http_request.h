#pragma once
#include "http_headers.hpp"
#include "http_version.hpp"
#include "library_utils.h"
#include "methods.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace CNetUtils {
namespace http {

	namespace RequestParseLimitations {
		static constexpr const size_t MAX_LINE = 10000; // can not be too long dude
	}

	struct Request {
		using request_value_t = std::string;
		using request_key_t = std::string;
		using request_value_list_t = std::vector<request_value_t>;
		using query_map_t = std::unordered_map<request_key_t,
		                                       request_value_list_t>;

		HttpMethod method { HttpMethod::UNKNOWN };
		std::string path {};
		HttpVersion version {};
		Headers headers;
		std::string body {};
		bool isKeepAlive { true };

		// key values

		Request() = default;
		/**
		 * @brief 	Construct a new Request object
		 *			by given a build_block strings
		 *
		 * @param header_block
		 */
		Request(const std::string& header_block);

		/**
		 * @brief Query the
		 *
		 * @param key
		 * @return std::optional<std::string>
		 */
		std::optional<std::string>
		query_first(const std::string& key) const {
			auto it = from_url_params.find(key);
			if (it == from_url_params.end() || it->second.empty())
				return std::nullopt;
			return it->second.front();
		}

		std::vector<std::string> query_all(const std::string& key) const {
			auto it = from_url_params.find(key);
			if (it == from_url_params.end() || it->second.empty())
				return {};
			return it->second;
		}

		std::optional<std::string> form_first(const std::string& key) const {
			auto it = from_form.find(key);
			if (it == from_form.end() || it->second.empty())
				return std::nullopt;
			return it->second.front();
		}
		std::vector<std::string> form_all(const std::string& key) const {
			auto it = from_form.find(key);
			if (it == from_form.end())
				return {};
			return it->second;
		}

		CNETUTILS_FORCEINLINE bool request_check_for_form_body() const {
			return headers.get("content-type").has_value();
		}

		void consume_form_body(const std::string& form_body);

	private:
		query_map_t from_url_params;
		query_map_t from_form;
	};

}
}
