#pragma once

#include <string>

namespace OE {
	// Convert a wide Unicode string to an UTF8 string
	std::string utf8_encode(const std::wstring &wstr);

	// Convert an UTF8 string to a wide Unicode String
	std::wstring utf8_decode(const std::string &str);

	bool str_starts(const std::string &str, const std::string &prefix);
	bool str_ends(const std::string &str, const std::string &suffix);

	bool str_starts(const std::wstring &str, const std::wstring &prefix);
	bool str_ends(const std::wstring &str, const std::wstring &suffix);

	std::string str_replace_all(std::string str, const std::string& from, const std::string& to);

	std::string to_string(HRESULT hr);
	std::wstring to_wstring(HRESULT hr);
}