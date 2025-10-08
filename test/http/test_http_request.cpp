
#include "http_request.h"
#include "methods.h"
#include <iostream>
#include <string>
using CNetUtils::http::Request;

int main() {
	// header-only example
	std::string headers_only = "GET /search?q=one&q=two&lang=zh HTTP/1.1\r\n"
	                           "Host: example.com\r\n"
	                           "Connection: keep-alive\r\n"
	                           "\r\n";
	try {
		Request req(headers_only);
		std::cout << "Method: " << to_string(req.method) << "\n";
		std::cout << "Path: " << req.path << "\n";
		auto qs = req.query_all("q");
		std::cout << "q values:";
		for (auto& v : qs)
			std::cout << " [" << v << "]";
		std::cout << "\n";
		std::cout << "keepalive: " << req.isKeepAlive << "\n";
	} catch (const std::exception& e) {
		std::cerr << "parse error: " << e.what() << "\n";
	}

	// header + body example (form)
	std::string headers_with_body = "POST /submit HTTP/1.1\r\n"
	                                "Host: example.com\r\n"
	                                "Content-Type: application/x-www-form-urlencoded\r\n"
	                                "Content-Length: 27\r\n"
	                                "\r\n";
	std::string body = "a=1&b=2&b=3&name=hello+world";
	try {
		Request req2(headers_with_body);
		req2.consume_form_body(body);
		std::cout << "Form b values:";
		for (auto& v : req2.form_all("b"))
			std::cout << " [" << v << "]";
		std::cout << "\n";
		std::cout << "Form name: " << req2.form_all("name")[0] << "\n";
	} catch (const std::exception& e) {
		std::cerr << "parse error: " << e.what() << "\n";
	}

	// ---------- header + body example (json -> flattened form) ----------
	std::string body_json = R"({"user":{"name":"alice","age":30},"tags":["x","y"],"active":true})";
	std::string headers_with_json = std::string("POST /api HTTP/1.1\r\n")
	    + "Host: example.com\r\n"
	    + "Content-Type: application/json\r\n"
	    + "Content-Length: " + std::to_string(body_json.size()) + "\r\n"
	    + "\r\n";
	try {
		// 如果你的 Request 支持 constructor(header, body)
		Request req3(headers_with_json);

		req3.consume_form_body(body_json);
		// 打印检查扁平化结果（key 形式示例： "user.name", "user.age", "tags", "active"）
		auto uname_vals = req3.form_all("user.name");
		std::cout << "user.name:";
		for (auto& v : uname_vals)
			std::cout << " [" << v << "]";
		std::cout << "\n";

		auto uage_vals = req3.form_all("user.age");
		std::cout << "user.age:";
		for (auto& v : uage_vals)
			std::cout << " [" << v << "]";
		std::cout << "\n";

		auto tags_vals = req3.form_all("tags");
		std::cout << "tags:";
		for (auto& v : tags_vals)
			std::cout << " [" << v << "]";
		std::cout << "\n";

		auto active_vals = req3.form_all("active");
		std::cout << "active:";
		for (auto& v : active_vals)
			std::cout << " [" << v << "]";
		std::cout << "\n";

		// 简单断言：检查我们期望的值存在
		bool ok3 = false;
		auto maybe_name = req3.form_first("user.name");
		auto maybe_age = req3.form_first("user.age");
		if (maybe_name.has_value() && maybe_age.has_value()) {
			ok3 = (maybe_name.value() == "alice") && (maybe_age.value() == "30")
			    && (req3.form_all("tags").size() == 2)
			    && (req3.form_first("active").value_or("") == "true");
		}
		std::cout << "json-body test: " << (ok3 ? "PASS" : "FAIL") << "\n";
	} catch (const std::exception& e) {
		std::cerr << "json parse/form error: " << e.what() << "\n";
	}

	return 0;
}