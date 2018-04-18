#pragma once

#include <string>
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

	std::string hr_to_string(HRESULT hr);
	std::wstring hr_to_wstring(HRESULT hr);

	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
		std::string m_what;
	public:
		com_exception(HRESULT hr) : com_exception(hr, nullptr) {}
		com_exception(HRESULT hr, const std::string &what);
		com_exception(HRESULT hr, const char *what);

		const char* what() const override
		{
			return m_what.c_str();
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr, const char *what)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr, what);
		}
	}

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr, const std::string &what)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr, what);
		}
	}
}

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw OE::com_exception(hr);
		}
	}
}