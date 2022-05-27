#include <sstream>
#include <vector>
#include "istream_wrapper.hpp"
#include <neutrino/utils/strings/wchar.hh>

namespace bsw
{
	istream_wrapper_c::istream_wrapper_c(std::istream& is)
		: stream(&is),
		m_data(0),
		m_data_len(0),
		m_data_pos(0),
		m_is_owner (false)
	{
	}

	istream_wrapper_c::istream_wrapper_c(const char* data, std::size_t len)
		: stream(0),
		m_data(data),
		m_data_len(len),
		m_data_pos(0),
		m_is_owner(false)
	{

	}

	istream_wrapper_c::istream_wrapper_c(std::istream& is, std::size_t offs, std::size_t len)
		: stream (0),
		  m_data_len (len),
		  m_data_pos (0),
		  m_is_owner (true)
	{


		std::streampos fsize = 0;

		is.seekg(0, std::ios::end);
		if (is)
		{
			fsize = static_cast <uint64_t>(is.tellg()) - offs;
			is.seekg(offs, std::ios::beg);
		}
		if (!is || static_cast<std::size_t>(fsize) < len)
		{
			throw std::runtime_error("I/O error");
		}
		
		m_data = new char[len];
		is.read(const_cast <char*> (m_data), len);
	}

	istream_wrapper_c::~istream_wrapper_c()
	{
		if (m_is_owner)
		{
			delete[] m_data;
		}
	}


	std::streampos istream_wrapper_c::current_pos() const
	{
		if (m_data)
		{
			return m_data_pos;
		}

		return stream->tellg();
	}

	std::streamsize istream_wrapper_c::size_to_end() const
	{
		if (m_data)
		{
			return m_data_len - m_data_pos;
		}
		std::streampos fsize = 0;
		if (stream)
		{
			std::streampos cur_pos = stream->tellg();
			stream->seekg(0, std::ios::end);
			fsize = stream->tellg() - cur_pos;
			stream->seekg(cur_pos, std::ios::beg);
		}
		else
		{
			throw std::runtime_error("nullptr in stream");
		}

		if (!(*stream))
		{
			throw std::runtime_error("I/O error");
		}
		return fsize;
	}

	void istream_wrapper_c::advance(std::streampos delta)
	{
		if (m_data)
		{
			if (m_data_pos + delta > m_data_len)
			{
				throw std::runtime_error("I/O error");
			}
			m_data_pos += (std::size_t)delta;
		}
		else
		{
			stream->seekg(delta, std::ios::cur);
			if (!(*stream))
			{
				throw std::runtime_error("I/O error");
			}
		}
	}

	void istream_wrapper_c::_seek(std::streampos pos, bool truncate)
	{
		if (m_data)
		{
			if (static_cast<std::size_t>(pos) > m_data_len)
			{
				if (truncate)
				{
					m_data_pos = m_data_len;
					return;
				}
				throw std::runtime_error("I/O error");
			}
			m_data_pos = static_cast <std::size_t>(pos);
		}
		else
		{
			stream->seekg(pos, std::ios::beg);
			if (!(*stream))
			{
				if (truncate)
				{
					stream->seekg(0, std::ios::end);
					return;
				}
				throw std::runtime_error("I/O error");
			}
		}
	}

	void istream_wrapper_c::seek(std::streampos pos)
	{
		this->_seek(pos, false);
	}

	void istream_wrapper_c::read(char* buff, std::size_t size)
	{
		if (m_data)
		{
			if (m_data_pos + size <= m_data_len)
			{
				std::memcpy(buff, m_data + m_data_pos, size);
				m_data_pos += size;
			}
			else
			{
				throw std::runtime_error("I/O error");
			}
		}
		else
		{
			stream->read(buff, size);
			if (!(*stream))
			{
				throw std::runtime_error("I/O error");
			}
		}
	}

	void istream_wrapper_c::assert_word(uint16_t word)
	{
		uint16_t w;
		read((char*)&w, sizeof(w));
		if (w != word)
		{
			std::ostringstream os;
			os << __FUNCTION__ << " expected " << word << " actual " << w;
			throw std::runtime_error(os.str());
		}
	}

	void istream_wrapper_c::assert_dword(uint32_t word)
	{
		uint32_t w;
		read((char*)&w, sizeof(w));
		if (w != word)
		{
			std::ostringstream os;
			os << __FUNCTION__ << " expected " << word << " actual " << w;
			throw std::runtime_error(os.str());
		}
	}

	void istream_wrapper_c::assert_string(const wchar_t* s, bool align)
	{
		const std::size_t n = wcslen(s);
		std::vector <wchar_t> d(n + 1, 0);
		read((char*)d.data(), (n + 1)*sizeof(wchar_t));
		for (std::size_t i = 0; i <= n; i++)
		{
			const auto e = s[i];
			const auto a = d[i];
			if (a != e && a != (e ^ 0x20))
			{
				std::ostringstream os;
				os << __FUNCTION__ << " expected " << neutrino::utils::wstring_to_utf8 (s) << " actual " << neutrino::utils::wstring_to_utf8 (d.data ());
				throw std::runtime_error(os.str());
			}
		}
		if (align)
		{
			align4();
		}
	}

	bool istream_wrapper_c::check_string(const wchar_t* s, bool align)
	{
		uint64_t curr = current_pos();
		const std::size_t n = wcslen(s);
		std::vector <wchar_t> d(n + 1, 0);
		read((char*)d.data(), (n + 1)*sizeof(wchar_t));
		for (std::size_t i = 0; i <= n; i++)
		{
			const auto e = s[i];
			const auto a = d[i];
			if (a != e && a != (e ^ 0x20))
			{
				seek(curr);
				return false;
			}
		}
		if (align)
		{
			align4();
		}

		return true;
	}

	void istream_wrapper_c::align4()
	{
		uint64_t x = current_pos();
		x += 3;
		x &= ~(uint64_t)3;
		_seek(x, true);
	}

	void istream_wrapper_c::assert_space(uint64_t s) const
	{
		const uint64_t a = this->size_to_end();
		if (s > a)
		{
			std::ostringstream os;
			os << "Not enough space. requested " << s << " actual " << a;
			throw std::runtime_error(os.str());
		}
	}

	void istream_wrapper_c::read_string(std::wstring& result, std::size_t n, bool ensure_null)
	{
		if (n == 0)
		{
			return;
		}
		bool has_null = false;
		for (std::size_t i = 0; i < n; i++)
		{
			wchar_t x;
			read((char*)&x, sizeof(wchar_t));
			if (x)
			{
				result += x;
			}
			else
			{
				has_null = true;
				break;
			}
		}
		if (ensure_null && !has_null)
		{
			throw std::runtime_error("Bad string length");
		}
	}
} // ns spy
