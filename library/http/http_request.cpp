#include "http_request.h"
#include "compare_helper.hpp"
#include "copy_helpers.hpp"
#include "http_defines.h"
#include "http_exceptions.h"
#include "http_version.hpp"
#include "json_helper/json_to_http.h"
#include "methods.h"
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <sys/types.h>

namespace {

// Helpers for the line consume
std::optional<std::string> consume_line(
    const std::string& header_block,
    size_t& pos, // OUT Assigned
    const size_t len) {
	if (pos >= len)
		return std::nullopt; // overflow sessions
	const size_t nl = header_block.find("\n", pos);

	std::string result;

	if (nl == std::string::npos) {
		// last line w/o trailing newline
		result.assign(header_block.data() + pos, header_block.data() + len);
		pos = len;
		return result;
	}

	// include characters up to nl (exclude '\n')
	size_t start = pos;
	size_t end = nl;
	if (end > start && header_block[end - 1] == '\r')
		--end; // strip CR if present
	result.assign(header_block.data() + start, header_block.data() + end);
	pos = nl + 1;
	return result;
}

/**
 * @brief Force to decode %XX and + -> space
 *
 * @param s
 * @return std::string
 */
std::string url_decode(const std::string& s) {
	std::string decoded_url;
	// pre-allocate the decodeds
	decoded_url.reserve(s.size());

	for (size_t i = 0; i < s.size(); ++i) {
		char c = s[i];
		if (c == '+') {
			decoded_url.push_back(' '); // emplace with ' '
		} else if (c == '%' && i + 2 < s.size()) {
			auto hs = s.substr(i + 1, 2);
			unsigned int val = 0;
			std::istringstream iss(hs);
			if (!(iss >> std::hex >> val)) {
				throw CNetUtils::http::UrlDecodingError("Decoding failed when attempting with " + s);
			}
			decoded_url.push_back(static_cast<char>(val));
			i += 2;
		} else {
			decoded_url.push_back(c);
		}
	}
	return decoded_url;
}

CNetUtils::http::Request::query_map_t
filled_params_query(const std::string& src) {
	using query_map_t = CNetUtils::http::Request::query_map_t;
	using request_key_t = CNetUtils::http::Request::request_key_t;
	using request_value_t = CNetUtils::http::Request::request_value_t;

	query_map_t query_pair;
	size_t position = 0;
	const size_t max_len = src.size();
	while (position < max_len) {
		auto amp = src.find('&', position);
		size_t end = (amp == std::string::npos) ? src.size() : amp;
		auto equal_splits = src.find('=', position);

		if (equal_splits != std::string::npos && equal_splits < end) {
			// Ok, these is valid
			request_key_t key = src.substr(position, equal_splits - position);
			request_value_t value = src.substr(equal_splits + 1, end - equal_splits - 1);
			query_pair[std::move(url_decode(key))].emplace_back(std::move(url_decode(value)));
		} else {
			// key without value
			request_key_t key = src.substr(position, end - position);
			query_pair[std::move(url_decode(key))].emplace_back(std::string {});
		}

		if (amp == std::string::npos)
			break;
		position = amp + 1;
	}

	return query_pair;
}

std::optional<CNetUtils::http::Request::query_map_t>
parse_multipart_formdata(const std::string& body_block,
                         const std::string& content_type_header) {
	CNetUtils::http::Request::query_map_t form;
	/**
	 * @brief We first need to find the boundary
	 *
	 */
	auto bpos = content_type_header.find("boundary=");
	if (bpos == std::string::npos)
		return std::nullopt; // invalid

	std::string boundary = "--" + content_type_header.substr(bpos + 9);
	size_t pos = 0;

	while (true) {
		size_t start = body_block.find(boundary, pos);
		if (start == std::string::npos)
			break;
		start += boundary.size();
		// handle final boundary that ends with "--"
		if (start + 2 <= body_block.size() && body_block.substr(start, 2) == "--")
			break;
		if (start + 2 <= body_block.size() && body_block.substr(start, 2) == "\r\n")
			start += 2;

		size_t end = body_block.find(boundary, start);
		if (end == std::string::npos)
			break;
		std::string part = body_block.substr(start, end - start);
		// split part headers / body by \r\n\r\n
		auto sep = part.find(CNetUtils::http::ALL_SEG_TERMINATE);
		if (sep == std::string::npos) {
			pos = end;
			continue;
		}
		std::string part_headers = part.substr(0, sep);
		std::string part_body = part.substr(sep + 4);
		// remove trailing CRLF if present
		if (part_body.size() >= 2 && part_body.substr(part_body.size() - 2) == "\r\n")
			part_body.resize(part_body.size() - 2);

		// find name="..."
		auto name_pos = part_headers.find("name=\"");
		if (name_pos != std::string::npos) {
			size_t name_start = name_pos + 6;
			auto name_end = part_headers.find('"', name_start);
			if (name_end != std::string::npos) {
				std::string name = part_headers.substr(name_start, name_end - name_start);
				form[name].push_back(part_body);
			}
		}
		pos = end;
	}
	return form;
}
};

namespace CNetUtils::http {

Request::Request(const std::string& header_block) {
	std::istringstream is(header_block);
	std::string first; // consume
	size_t pos = 0;
	const size_t len = header_block.size();

	// OK, consume the first line
	auto start_line = consume_line(header_block, pos, len);
	// which lines like POST /echo HTTP/1.1
	// including the sessions of:
	// [Methods] [Requesting Path] [Version Strings]
	// thats what we should do by using istringstream!
	if (!start_line.has_value() || start_line->empty()) {
		// if the start_line is invalid or empty, get out!
		throw HttpRequestParseError(
		    "Failed to find the start line! or its empty! "
		    "Request Provided is not valid!"); // viewing as NULL Request
	} else {
		// OK, we should start parsing the line...
		std::string method_string, request_string, version_string;
		std::istringstream first_line_parser(*start_line);
		if (!(first_line_parser >> method_string
		      >> request_string >> version_string)) {
			// oh sucks!
			throw HttpRequestParseError(
			    "Can not parse method/request/versions for the first line");
		}

		// OK, the method_string, request_string and version_string is all valid
		this->method = parse_http_method(method_string);
		if (is_invalid_method(this->method)) {
			throw HttpRequestParseError(
			    "Can not parse method, method is unsupported: " + method_string);
		}

		auto qpos = request_string.find('?');
		if (qpos != std::string::npos) {
			// OK we owns the keys
			std::string qs = request_string.substr(qpos + 1);
			this->path = request_string.substr(0, qpos);
			this->from_url_params = std::move(filled_params_query(qs));
		} else {
			// ok we dont :)
			this->path = request_string;
		}

		this->version = from_string(version_string);
		if (is_invalid_http_version(this->version)) {
			throw HttpRequestParseError(
			    "Can not parse version, version is unknown: " + version_string);
		}
	}

	// OK, first line is parsed OK
	// then parse headers; allow continuation lines that start with SP or TAB
	std::string last_key;
	size_t header_count = 0;

	while (header_count <= RequestParseLimitations::MAX_LINE) {
		// OK, at first we need to add once, to make it
		// possibilities to leak the add sessions
		header_count++;
		// OK, we can consume a line
		auto this_line = consume_line(header_block, pos, len);
		if (!this_line.has_value() || this_line->empty()) {
			// OK, these is the end
			break;
		}

		// OK, we can safely parse out the sessions
		std::string line = std::move(*this_line);

		// continuation?
		if ((line[0] == ' ' || line[0] == '\t') && !last_key.empty()) {
			// append to previous header value (single space between)

			auto existing = headers.get(last_key);
			std::string cont = trim_copy(line);
			if (existing.has_value()) {
				std::string merged = *existing + " " + cont;
				headers.set(last_key, std::move(merged));
			} else {
				headers.set(last_key, cont);
			}
			// we are fetching the values,
			// so continue and get the next line
			continue;
		}

		auto colon = line.find(':');
		if (colon == std::string::npos) {
			// ignore malformed header line (or log)
			continue;
		}

		std::string key = to_lower_copy(trim_copy(line.substr(0, colon)));
		std::string val = trim_copy(line.substr(colon + 1));

		// combine multiple headers: most headers allow comma-joined values
		auto possible_exsiting_key = headers.get(key);

		// if the key is exsiting...
		if (possible_exsiting_key.has_value()) {
			// special-case Set-Cookie: don't join with commas
			std::string comb = *possible_exsiting_key + ", " + val;
			headers.set(key, std::move(comb));
		} else {
			headers.set(std::move(key), std::move(val));
		}

		last_key = key; // assigned the key to make up a value

		// OK, go for the next loop
	} // while loop ends here

	// for what reason do we quit then?
	if (header_count > RequestParseLimitations::MAX_LINE) {
		// oh, client throws us a shit!
		throw HttpRequestParseError("Client has sent us too many lines!");
	}

	// further settings
	auto is_long_connections = headers.get("connection");
	if (is_long_connections.has_value()) {
		std::string c = CNetUtils::to_lower_copy(*is_long_connections);
		this->isKeepAlive = (c == "keep-alive");
	} else {
		// fallbacks, if not HttpVersion::V1_0
		// we default opens the keep alive
		this->isKeepAlive = (this->version != HttpVersion::V1_0);
	}
}

void Request::consume_form_body(const std::string& form_body) {
	auto content_type = headers.get("content-type");
	if (!content_type.has_value())
		return; // we dont need to consume the body
	auto ct_value = *content_type;
	if (content_type_contains(ct_value, "application/x-www-form-urlencoded")) {
		// Parse as kv pairs
		this->from_form = std::move(filled_params_query(form_body));
	} else if (content_type_contains(ct_value, "multipart/form-data")) {
		auto result = parse_multipart_formdata(form_body, ct_value);
		if (!result.has_value()) {
			throw FailedParseForm("Failed to parse forms, these might be failed to find boundary key...");
		} else {
			this->from_form = std::move(*result);
		}
	} else if (content_type_contains(ct_value, "application/json")) {
		this->from_form = std::move(from_json_string(form_body));
	}
}

}
