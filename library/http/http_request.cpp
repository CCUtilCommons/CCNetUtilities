#include "http_request.h"
#include "copy_helpers.hpp"
#include "http_exceptions.h"
#include "http_version.hpp"
#include "methods.h"
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>

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

}

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

		this->path = request_string; // request string assigned
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
	}
}

}
