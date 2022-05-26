#ifndef __BSW_STRING_HPP__
#define __BSW_STRING_HPP__

#include <string>
#include <vector>


namespace bsw
{
	wchar_t to_lower(wchar_t ch);
	char to_lower(char ch);
	std::string& to_lower(std::string& s);
	std::wstring& to_lower(std::wstring& s);

	std::string& replace_all(std::string& s, const std::string& a, const std::string& b);
	std::wstring& replace_all(std::wstring& s, const std::wstring& a, const std::wstring& b);

	std::string& trim(std::string& s);
	std::wstring& trim(std::wstring& s);

	void split(std::vector<std::string>& result,
		const std::string& string,
		const std::string& delimiter);

	void split(std::vector<std::wstring>& result,
		const std::wstring& string,
		const std::wstring& delimiter);

	bool is_number(const std::string& s);
	bool is_number(const std::wstring& s);

	bool ends_with(std::string const &fullString, std::string const &ending);
	bool ends_with(std::wstring const &fullString, std::wstring const &ending);

	void escape_for_xml(std::string& text);
	std::string escape_for_xml(const std::string& text);

	long find_substr_ci(const std::string& s1, const std::string& s2);
	long find_substr_ci(const std::wstring& s1, const std::wstring& s2);

	bool string_is_empty(const char* s);
	bool string_is_empty(const wchar_t* s);

	enum class str2int_t
	{
		eSUCCESS, eOVERFLOW, eUNDERFLOW, eINCONVERTIBLE
	};

	str2int_t str2int(int &i, char const *s, int base = 10);
} // ns bsw 

#endif
