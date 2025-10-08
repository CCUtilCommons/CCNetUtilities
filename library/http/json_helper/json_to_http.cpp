#include "json_to_http.h"
#include "library_utils.h"
#include <cstddef>
#include <nlohmann/json.hpp>

namespace CNetUtils::http {

namespace {
	CNETUTILS_FORCEINLINE bool is_bom_json(const std::string& json_string) {
		auto check_each = [&](
		                      size_t index,
		                      unsigned char target) -> bool {
			return static_cast<unsigned char>(json_string[index]) == target;
		};

		return check_each(
		           0, 0xEF)
		    && check_each(
		           1, 0xBB)
		    && check_each(
		           2, 0xBF);
	}

	void json_to_multimap(const nlohmann::json& j, Request::query_map_t& out, const std::string& prefix = "") {
		auto make_key = [&](const std::string& k) -> std::string {
			if (prefix.empty())
				return k;
			return prefix + "." + k;
		};

		if (j.is_object()) {
			for (auto it = j.begin(); it != j.end(); ++it) {
				const std::string key = it.key();
				const nlohmann::json& val = it.value();
				if (val.is_object()) {
					// 递归展开对象
					json_to_multimap(val, out, make_key(key));
				} else if (val.is_array()) {
					// 对数组：原子值逐项加入；复杂元素序列化后作为单个值
					for (size_t i = 0; i < val.size(); ++i) {
						const nlohmann::json& elem = val[i];
						if (elem.is_primitive()) {
							if (elem.is_string())
								out[make_key(key)].push_back(elem.get<std::string>());
							else if (elem.is_boolean())
								out[make_key(key)].push_back(elem.get<bool>() ? "true" : "false");
							else if (elem.is_number_integer() || elem.is_number_unsigned() || elem.is_number_float()) {
								std::ostringstream oss;
								oss << elem;
								out[make_key(key)].push_back(oss.str());
							} else if (elem.is_null()) {
								out[make_key(key)].push_back(std::string {}); // null -> empty string
							} else {
								out[make_key(key)].push_back(elem.dump());
							}
						} else {
							// 对象或嵌套数组：直接序列化为字符串
							out[make_key(key)].push_back(elem.dump());
						}
					}
				} else { // 原子类型
					if (val.is_string())
						out[make_key(key)].push_back(val.get<std::string>());
					else if (val.is_boolean())
						out[make_key(key)].push_back(val.get<bool>() ? "true" : "false");
					else if (val.is_number_integer() || val.is_number_unsigned() || val.is_number_float()) {
						std::ostringstream oss;
						oss << val;
						out[make_key(key)].push_back(oss.str());
					} else if (val.is_null()) {
						out[make_key(key)].push_back(std::string {});
					} else {
						out[make_key(key)].push_back(val.dump());
					}
				}
			}
		} else if (j.is_array()) {
			for (size_t i = 0; i < j.size(); ++i) {
				const nlohmann::json& elem = j[i];
				std::string key = prefix.empty() ? std::to_string(i) : (prefix + "[" + std::to_string(i) + "]");
				if (elem.is_primitive()) {
					if (elem.is_string())
						out[key].push_back(elem.get<std::string>());
					else if (elem.is_boolean())
						out[key].push_back(elem.get<bool>() ? "true" : "false");
					else {
						std::ostringstream oss;
						oss << elem;
						out[key].push_back(oss.str());
					}
				} else {
					out[key].push_back(elem.dump());
				}
			}
		} else {
			std::string key = prefix;
			if (j.is_string())
				out[key].push_back(j.get<std::string>());
			else if (j.is_boolean())
				out[key].push_back(j.get<bool>() ? "true" : "false");
			else {
				std::ostringstream oss;
				oss << j;
				out[key].push_back(oss.str());
			}
		}
	}

}

query_map_t from_json_string(const std::string json_string) {
	std::string s = json_string;
	if (s.size() >= 3 && is_bom_json(json_string)) {
		s.erase(0, 3);
	}

	try {
		nlohmann::json j = nlohmann::json::parse(s);
		query_map_t map;
		json_to_multimap(j, map);
		return map;
	} catch (const nlohmann::json::parse_error& e) {
		throw RequestJsonParseFailed(std::string("Third Party blame you for the json parsing: ") + e.what());
	} catch (const std::exception& e) {
		throw RequestJsonParseFailed(std::string("Unexpected error while parsing JSON form body: ") + e.what());
	}
}
}