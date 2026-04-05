#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../deps/httplib.h"

namespace OutgoingError {
inline constexpr int NONE = 0;
inline constexpr int INVALID_URL = 1001;
inline constexpr int UNSUPPORTED_SCHEME = 1002;
inline constexpr int TLS_UNAVAILABLE = 1003;
inline constexpr int TLS_INVALID_CERTS = 1004;
inline constexpr int FILE_NOT_FOUND = 1005;
inline constexpr int EMPTY_FILE = 1006;
inline constexpr int CANCELLED = 1007;
inline constexpr int HTTP_STATUS = 1008;
inline constexpr int NETWORK = 1100;
inline constexpr int TIMEOUT = 1101;
inline constexpr int TLS_HANDSHAKE = 1102;
inline constexpr int UNKNOWN = 1199;
inline constexpr int JSON_PARSE = 1200;
inline constexpr int WEBSOCKET = 1201;
}

static std::string NormalizeSlashes(std::string s)
{
	std::replace(s.begin(), s.end(), '\\', '/');
	return s;
}

static std::string Trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

static std::string SanitizeFilename(const std::string& input)
{
	std::string filename = NormalizeSlashes(input);

	size_t lastSlash = filename.find_last_of('/');
	if (lastSlash != std::string::npos) {
		filename = filename.substr(lastSlash + 1);
	}

	if (filename.empty() || filename == "." || filename == "..")
		return "";

	std::string out;
	out.reserve(filename.size());

	for (unsigned char c : filename) {
		if (c < 32) continue;
		switch (c) {
			case '<': case '>': case ':': case '"':
			case '/': case '\\': case '|': case '?': case '*':
				out += '_';
				break;
			default:
				out += static_cast<char>(c);
				break;
		}
	}

	while (!out.empty() && (out.back() == ' ' || out.back() == '.'))
		out.pop_back();

	if (out.empty() || out == "." || out == "..")
		return "";

	return out;
}

static std::string SanitizeRelativeDir(const std::string& input)
{
	std::string path = NormalizeSlashes(Trim(input));
	if (path.empty()) return "";

	if (!path.empty() && path[0] == '/') return "";
	if (path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':') return "";

	std::stringstream ss(path);
	std::string segment;
	std::vector<std::string> parts;

	while (std::getline(ss, segment, '/')) {
		segment = Trim(segment);
		if (segment.empty() || segment == ".") continue;
		if (segment == "..") return "";
		parts.push_back(segment);
	}

	std::string out;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i) out += '/';
		out += parts[i];
	}
	if (!out.empty() && out.back() != '/') out += '/';
	return out;
}

static std::vector<std::string> SplitCSV(const std::string& s)
{
	std::vector<std::string> out;
	std::stringstream ss(s);
	std::string token;
	while (std::getline(ss, token, ',')) {
		token = Trim(token);
		if (!token.empty())
			out.push_back(token);
	}
	return out;
}

static std::vector<std::pair<std::string, std::string>> SplitHeaderPairs(const std::string& headerStr)
{
	std::vector<std::pair<std::string, std::string>> result;
	std::stringstream ss(headerStr);
	std::string pair;

	while (std::getline(ss, pair, '|')) {
		size_t colonPos = pair.find(':');
		if (colonPos != std::string::npos) {
			std::string key = Trim(pair.substr(0, colonPos));
			std::string val = Trim(pair.substr(colonPos + 1));
			if (!key.empty()) result.emplace_back(key, val);
		}
	}

	return result;
}

static std::unordered_map<std::string, std::string> ParseHeaderMap(const std::string& headerStr)
{
	std::unordered_map<std::string, std::string> headers;
	for (const auto& [key, value] : SplitHeaderPairs(headerStr)) {
		headers[key] = value;
	}
	return headers;
}

static void SetOrReplaceHeader(httplib::Headers& headers, const std::string& key, const std::string& value)
{
	auto existing = headers.equal_range(key);
	if (existing.first != existing.second) {
		headers.erase(existing.first, existing.second);
	}
	headers.emplace(key, value);
}

static std::string HeaderMapToString(const std::unordered_map<std::string, std::string>& headers)
{
	std::string output;
	bool first = true;
	for (const auto& [key, value] : headers) {
		if (!first) output += "|";
		output += key + ": " + value;
		first = false;
	}
	return output;
}

static std::string JoinUrlPath(const std::string& baseUrl, const std::string& path)
{
	if (path.empty()) return baseUrl;
	if (baseUrl.empty()) return path;
	if (path.find("://") != std::string::npos) return path;

	bool baseEndsWithSlash = !baseUrl.empty() && baseUrl.back() == '/';
	bool pathStartsWithSlash = !path.empty() && path.front() == '/';

	if (baseEndsWithSlash && pathStartsWithSlash) {
		return baseUrl + path.substr(1);
	}
	if (!baseEndsWithSlash && !pathStartsWithSlash) {
		return baseUrl + "/" + path;
	}
	return baseUrl + path;
}

static std::pair<int, std::string> ClassifyClientError(httplib::Error error)
{
	switch (error) {
		case httplib::Error::ConnectionTimeout:
		case httplib::Error::Timeout:
			return { OutgoingError::TIMEOUT, "timeout" };
		case httplib::Error::SSLConnection:
		case httplib::Error::SSLServerVerification:
		case httplib::Error::SSLServerHostnameVerification:
			return { OutgoingError::TLS_HANDSHAKE, "tls" };
		case httplib::Error::Connection:
		case httplib::Error::Read:
		case httplib::Error::Write:
		case httplib::Error::ConnectionClosed:
		case httplib::Error::ProxyConnection:
			return { OutgoingError::NETWORK, "network" };
		default:
			return { OutgoingError::UNKNOWN, "unknown" };
	}
}

static void Assert(bool condition, const char* message)
{
	if (!condition) {
		std::fprintf(stderr, "Assertion failed: %s\n", message);
		std::exit(1);
	}
}

int main()
{
	Assert(NormalizeSlashes("a\\b\\c") == "a/b/c", "NormalizeSlashes should replace backslashes");
	Assert(Trim(" \t hello \n") == "hello", "Trim should strip whitespace");
	Assert(Trim(" \n\t ") == "", "Trim should return empty for all-whitespace");

	Assert(SanitizeFilename("../evil.txt") == "evil.txt", "SanitizeFilename should keep basename");
	Assert(SanitizeFilename("a<b>:c?.txt") == "a_b__c_.txt", "SanitizeFilename should replace invalid chars");
	Assert(SanitizeFilename("file. ") == "file", "SanitizeFilename should trim trailing dots/spaces");
	Assert(SanitizeFilename("..") == "", "SanitizeFilename should reject dot segments");
	Assert(SanitizeFilename("dir\\name.txt") == "name.txt", "SanitizeFilename should normalize slashes before basename");

	Assert(SanitizeRelativeDir("folder/sub") == "folder/sub/", "SanitizeRelativeDir should append trailing slash");
	Assert(SanitizeRelativeDir(" /abs/path ") == "", "SanitizeRelativeDir should reject absolute paths");
	Assert(SanitizeRelativeDir("C:/windows/path") == "", "SanitizeRelativeDir should reject drive-prefixed paths");
	Assert(SanitizeRelativeDir("a/../b") == "", "SanitizeRelativeDir should reject parent traversal");
	Assert(SanitizeRelativeDir("a//./b/") == "a/b/", "SanitizeRelativeDir should normalize separators and dots");

	{
		auto csv = SplitCSV(" .txt, .json , , .cfg ");
		Assert(csv.size() == 3, "SplitCSV should skip empty items");
		Assert(csv[0] == ".txt" && csv[1] == ".json" && csv[2] == ".cfg", "SplitCSV should trim tokens");
	}

	{
		auto pairs = SplitHeaderPairs("A: 1|B:2|NoColon| :x|C: value:with:colons");
		Assert(pairs.size() == 3, "SplitHeaderPairs should only include valid key:value entries");
		Assert(pairs[0] == std::make_pair(std::string("A"), std::string("1")), "SplitHeaderPairs should parse first pair");
		Assert(pairs[1] == std::make_pair(std::string("B"), std::string("2")), "SplitHeaderPairs should parse second pair");
		Assert(pairs[2] == std::make_pair(std::string("C"), std::string("value:with:colons")), "SplitHeaderPairs should preserve value colons");
	}

	{
		auto map = ParseHeaderMap("A:1|A:2|B:3");
		Assert(map.size() == 2, "ParseHeaderMap should collapse duplicate keys");
		Assert(map["A"] == "2" && map["B"] == "3", "ParseHeaderMap should keep last value");
	}

	{
		httplib::Headers headers;
		headers.emplace("X-Test", "old");
		headers.emplace("X-Test", "older");
		SetOrReplaceHeader(headers, "X-Test", "new");
		Assert(headers.count("X-Test") == 1, "SetOrReplaceHeader should keep one key");
		auto range = headers.equal_range("X-Test");
		Assert(range.first != range.second && range.first->second == "new", "SetOrReplaceHeader should set new value");
	}

	{
		std::unordered_map<std::string, std::string> h = {{"Auth", "token"}, {"Content-Type", "application/json"}};
		std::string s = HeaderMapToString(h);
		Assert(s.find("Auth: token") != std::string::npos, "HeaderMapToString should include Auth header");
		Assert(s.find("Content-Type: application/json") != std::string::npos, "HeaderMapToString should include Content-Type header");
		Assert(s.find('|') != std::string::npos, "HeaderMapToString should separate pairs");
	}

	Assert(JoinUrlPath("https://api.test", "v1/users") == "https://api.test/v1/users", "JoinUrlPath should add slash");
	Assert(JoinUrlPath("https://api.test/", "/v1/users") == "https://api.test/v1/users", "JoinUrlPath should avoid double slash");
	Assert(JoinUrlPath("https://api.test", "/v1/users") == "https://api.test/v1/users", "JoinUrlPath should preserve explicit leading slash");
	Assert(JoinUrlPath("", "/x") == "/x", "JoinUrlPath should handle empty base");
	Assert(JoinUrlPath("https://api.test", "http://other/path") == "http://other/path", "JoinUrlPath should return absolute path URL");

	{
		auto [code1, type1] = ClassifyClientError(httplib::Error::Timeout);
		auto [code2, type2] = ClassifyClientError(httplib::Error::SSLConnection);
		auto [code3, type3] = ClassifyClientError(httplib::Error::ConnectionClosed);
		auto [code4, type4] = ClassifyClientError(httplib::Error::Success);
		Assert(code1 == OutgoingError::TIMEOUT && type1 == "timeout", "ClassifyClientError should map timeout");
		Assert(code2 == OutgoingError::TLS_HANDSHAKE && type2 == "tls", "ClassifyClientError should map tls errors");
		Assert(code3 == OutgoingError::NETWORK && type3 == "network", "ClassifyClientError should map network errors");
		Assert(code4 == OutgoingError::UNKNOWN && type4 == "unknown", "ClassifyClientError should map unknown/default");
	}

	return 0;
}
