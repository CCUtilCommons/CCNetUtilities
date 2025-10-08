#pragma once
#include <optional>
#include <string>
#include <unordered_map>

namespace CNetUtils {
namespace http {

	/**
	 * @brief Fast helpers for the headers computations
	 * */
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

	struct Headers {
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

}
}
