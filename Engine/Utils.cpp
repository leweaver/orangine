#include "pch.h"

#include "Utils.h"

#include <Stringapiset.h>
#include <comdef.h>

using namespace oe;
using namespace std::string_literals;

// Convert a wide Unicode string to an UTF8 string
std::string oe::utf8_encode(const std::wstring &wstr)
{
	if (wstr.empty()) return std::string();
	if (wstr.size() > static_cast<size_t>(INT_MAX)) return std::string();

	const auto wstrSize = static_cast<int>(wstr.size());
	const auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstrSize, nullptr, 0, nullptr, nullptr);
	std::string strTo(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstrSize, &strTo[0], sizeNeeded, nullptr, nullptr);
	return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring oe::utf8_decode(const std::string &str)
{
	if (str.empty()) return std::wstring();
	if (str.size() > static_cast<size_t>(INT_MAX)) return std::wstring();

	const auto strSize = static_cast<int>(str.size());
	const auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &str[0], strSize, nullptr, 0);
	std::wstring wstrTo(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], strSize, &wstrTo[0], sizeNeeded);
	return wstrTo;
}

bool oe::str_starts(const std::string &str, const std::string &prefix)
{
	if (prefix.size() > str.size()) return false;
	return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool oe::str_ends(const std::string &str, const std::string &suffix)
{
	if (suffix.size() > str.size()) return false;
	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

bool oe::str_starts(const std::wstring &str, const std::wstring &prefix)
{
	if (prefix.size() > str.size()) return false;
	return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool oe::str_ends(const std::wstring &str, const std::wstring &suffix)
{
	if (suffix.size() > str.size()) return false;
	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string oe::str_replace_all(std::string str, const std::string &from, const std::string &to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

std::wstring oe::hr_to_wstring(const HRESULT hr)
{
	_com_error err(hr);
	return err.ErrorMessage();
}

std::string oe::errno_to_str()
{
	char errmsg[94];
	strerror_s(errmsg, sizeof(errmsg), errno);
	return std::string(errmsg);
}

com_exception::com_exception(HRESULT hr)
	: com_exception(hr, hr_to_string(hr))
{
}

com_exception::com_exception(HRESULT hr, std::string_view what)
	: _result(hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(_result));
	_what = std::string(s_str);
	if (!what.empty())
	{
		_what.append(" (").append(what.data()).append(")");
	}
}

std::string oe::hr_to_string(HRESULT hr)
{
	_com_error err(hr);
	const auto errMsg = err.ErrorMessage();
	return utf8_encode(errMsg);
}
