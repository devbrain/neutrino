#ifndef __BSW_WCHAR_HPP__
#define __BSW_WCHAR_HPP__

#include <string>
#include <vector>


namespace bsw
{
    std::wstring utf8_to_wstring (const std::string& str);
    std::string wstring_to_utf8 (const std::wstring& str);

	std::wstring utf8_to_wstring(const char* const str);
	std::string wstring_to_utf8(const wchar_t* const str);

	void utf8_to_wstring(const std::string& str, std::vector <wchar_t>& out);
	void wstring_to_utf8(const std::wstring& str, std::vector <char>& out);

	void utf8_to_wstring(const std::vector <char>& str, std::vector <wchar_t>& out);
	void wstring_to_utf8(const std::vector <wchar_t>& str, std::vector <char>& out);

	std::wstring utf8_to_wstring(const char* const str);
	std::string wstring_to_utf8(const wchar_t* const str);

	void utf8_to_wstring(const char* const str, std::vector <wchar_t>& out);
	void wstring_to_utf8(const wchar_t* const str, std::vector <char>& out);

} // ns spy

#endif
