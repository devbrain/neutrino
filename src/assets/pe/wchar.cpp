#include <iterator>
#include <cstring>
#include "wchar.hpp"
#include "utf8cpp/utf8.h"

namespace bsw
{
    // convert UTF-8 string to wstring
    std::wstring utf8_to_wstring (const std::string& str)
    {
		std::wstring r;
		if (!str.empty())
		{
			utf8::utf8to16(str.begin(), str.end(), std::back_inserter(r));
		}
		return r;

     // std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
     // return myconv.from_bytes(str);
    }

    // convert wstring to UTF-8 string
    std::string wstring_to_utf8 (const std::wstring& str)
    {
		std::string r;
		if (!str.empty())
		{
			utf8::utf16to8(str.begin(), str.end(), std::back_inserter(r));
		}
		return r;
    //    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
     //   return myconv.to_bytes(str);
    }
} // ns bsw


namespace bsw 
{
	std::wstring utf8_to_wstring(const char* const str)
	{
		if (str == 0)
		{
			return std::wstring();
		}
		return utf8_to_wstring(std::string(str));
	}

	std::string wstring_to_utf8(const wchar_t* const str)
	{
		if (str == 0)
		{
			return std::string();
		}
		return wstring_to_utf8(std::wstring(str));
	}

	void utf8_to_wstring(const std::string& str, std::vector <wchar_t>& out)
	{
		utf8::utf8to16(str.begin(), str.end(), std::back_inserter(out));
	}

	void utf8_to_wstring(const std::vector <char>& str, std::vector <wchar_t>& out)
	{
		utf8::utf8to16(str.begin(), str.end(), std::back_inserter(out));
	}

	void wstring_to_utf8(const std::vector <wchar_t>& str, std::vector <char>& out)
	{
		utf8::utf16to8(str.begin(), str.end(), std::back_inserter(out));
	}

	void wstring_to_utf8(const std::wstring& str, std::vector <char>& out)
	{
		utf8::utf16to8(str.begin(), str.end(), std::back_inserter(out));
	}

	void utf8_to_wstring(const char* const str, std::vector <wchar_t>& out)
	{
		if (!str)
		{
			return;
		}
		utf8::utf8to16(str, str + std::strlen (str), std::back_inserter(out));
	}

	void wstring_to_utf8(const wchar_t* const str, std::vector <char>& out)
	{
		if (!str)
		{
			return;
		}
		utf8::utf16to8(str, str + std::wcslen(str), std::back_inserter(out));
	}
}
