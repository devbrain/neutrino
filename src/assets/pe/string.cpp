#include <algorithm>
#include <locale>
#include <functional> 
#include <cctype>
#include <limits.h>
#include <errno.h>
#include "string.hpp"


template<typename charT>
struct my_equal 
{
	my_equal(const std::locale& loc) : loc_(loc) {}
	bool operator()(charT ch1, charT ch2) {
		return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
	}
private:
	const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
long ci_find_substr(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
	typename T::const_iterator it = std::search(str1.begin(), str1.end(),
		str2.begin(), str2.end(), my_equal<typename T::value_type>(loc));
	if (it != str1.end()) return static_cast<long>(it - str1.begin());
	else return -1; // not found
}

static void escape_for_xml(std::string& buffer, const std::string& data)
{
	buffer.reserve(data.size());
	for (size_t pos = 0; pos != data.size(); ++pos)
	{
		auto c = data[pos];
		switch (c) {
		case '&':  buffer.append("&amp;");       break;
		case '\"': buffer.append("&quot;");      break;
		case '\'': buffer.append("&apos;");      break;
		case '<':  buffer.append("&lt;");        break;
		case '>':  buffer.append("&gt;");        break;
		default:
			if (c<32)
			{
				buffer.append("&#");
				buffer.append(std::to_string(((unsigned int)c) & 0xFF));
				buffer.append(";");
			}
			else
			{
				buffer.append(&c, 1); break;
			}
		}
	}
}

template <typename STRING>
static
bool impl_is_number(const STRING& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), 
		[](typename STRING::value_type c) 
		{ 
			if (c < 0 || c> 255)
			{
				return true;
			}
			return !std::isdigit(c); 
		}) == s.end();
}

template <typename STRING, typename F>
static 
void do_split(F func, 
	const STRING& theString,
	const STRING& theDelimiter)
{
	size_t  start = 0, end = 0;

	while (end != STRING::npos)
	{
		end = theString.find(theDelimiter, start);

		// If at end, use length=maxLength.  Else use length=end-start.
		func (theString.substr(start, (end == STRING::npos) ? STRING::npos : end - start));

		// If at end, use start=maxSize.  Else use start=end+delimiter.
		start = ((end > (STRING::npos - theDelimiter.size())) ? STRING::npos : end + theDelimiter.size());
	}
}

static wchar_t to_lower_wch(wchar_t in)
{
	if (in <= L'Z' && in >= L'A')
		return in - (L'Z' - L'z');
	return in;
}

static char to_lower_ch(char in)
{
	if (in <= 'Z' && in >= 'A')
		return static_cast <char>(in - ('Z' - 'z'));
	return in;
}



// trim from start
template <typename STRING>
static inline STRING &ltrim(STRING &s)
{
	auto nsps = [](int x) {
		if (x < 0 || x > 255)
		{
			return true;
		}
		return std::isspace(x) == 0;
	};
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), nsps));
	return s;
}

// trim from end
template <typename STRING>
static inline STRING &rtrim(STRING &s)
{
	auto nsps = [](int x) {
		if (x < 0 || x > 255)
		{
			return true;
		}
		return std::isspace(x) == 0;
	};
	s.erase(std::find_if(s.rbegin(), s.rend(), nsps).base(), s.end());
	return s;
}

template <typename STRING>
static inline bool impl_ends_with(STRING const &fullString, STRING const &ending)
{
	if (fullString.length() >= ending.length()) 
	{
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	
	return false;
	
}

namespace bsw
{
	
	wchar_t to_lower(wchar_t in)
	{
		return to_lower_wch(in);
	}

	char to_lower(char in)
	{
		return to_lower_ch (in);
	}

	std::string& to_lower(std::string& data)
	{
		std::transform(data.begin(), data.end(), data.begin(), to_lower_ch);
		return data;
	}

	std::wstring& to_lower(std::wstring& data)
	{
		std::transform(data.begin(), data.end(), data.begin(), to_lower_wch);
		return data;
	}

	std::string& replace_all (std::string& str, const std::string& from, const std::string& to) 
	{
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) 
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}

	std::wstring& replace_all(std::wstring& str, const std::wstring& from, const std::wstring& to)
	{
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::wstring::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}

	std::string& trim(std::string& s)
	{
		return ltrim(rtrim(s));
	}

	std::wstring& trim(std::wstring& s)
	{
		return ltrim(rtrim(s));
	}

	void split(std::vector<std::string>& result,
		const std::string& string,
		const std::string& delimiter)
	{
		do_split([&result](const std::string&x) {
			result.push_back(x);
		}, string, delimiter);
	}

	void split(std::vector<std::wstring>& result,
		const std::wstring& string,
		const std::wstring& delimiter)
	{
		do_split([&result](const std::wstring&x) {
			result.push_back(x);
		}, string, delimiter);
	}

	bool is_number(const std::string& s)
	{
		return impl_is_number(s);
	}

	bool is_number(const std::wstring& s)
	{
		return impl_is_number(s);
	}
	// ==========================================================
	bool ends_with(std::string const &fullString, std::string const &ending)
	{
		return impl_ends_with(fullString, ending);
	}
	// ------------------------------------------------------------------------
	bool ends_with(std::wstring const &fullString, std::wstring const &ending)
	{
		return impl_ends_with(fullString, ending);
	}
	// ========================================================================
	void escape_for_xml (std::string& data) 
	{
		std::string buffer;
		::escape_for_xml(buffer, data);
		data.swap(buffer);
	}

	std::string escape_for_xml(const std::string& data)
	{
		std::string buffer;
		buffer.reserve(data.size());
		::escape_for_xml(buffer, data);
		return buffer;
	}
	// ========================================================================
	long find_substr_ci(const std::string& s1, const std::string& s2)
	{
		return ci_find_substr(s1, s2);
	}
	// ------------------------------------------------------------------------
	long find_substr_ci(const std::wstring& s1, const std::wstring& s2)
	{
		return ci_find_substr(s1, s2);
	}
	// ------------------------------------------------------------------------
	bool string_is_empty(const char* s)
	{
		return (s == nullptr) || (s[0] == 0);
	}
	// ------------------------------------------------------------------------
	bool string_is_empty(const wchar_t* s)
	{
		return (s == nullptr) || (s[0] == 0);
	}
	// ------------------------------------------------------------------------
	str2int_t str2int(int &i, char const *s, int base)
	{
		char *end;
		long  l;
		errno = 0;
		l = strtol(s, &end, base);
		if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) 
		{
			return str2int_t::eOVERFLOW;
		}
		if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) 
		{
			return str2int_t::eUNDERFLOW;
		}
		if (*s == '\0' || *end != '\0') 
		{
			return str2int_t::eINCONVERTIBLE;
		}
		i = l;
		return str2int_t::eSUCCESS;
	}
} // ns bsw
