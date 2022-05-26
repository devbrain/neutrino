#ifndef __BSW_FS_ISTREAM_WRAPPER_HPP__
#define __BSW_FS_ISTREAM_WRAPPER_HPP__

#include <iostream>
#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <stdint.h>
#include <type_traits>


namespace bsw
{
	class istream_wrapper_c;

	template <typename T>
	istream_wrapper_c& operator >> (istream_wrapper_c& is, T& x);

	class istream_wrapper_c
	{
		template <typename T>
		friend istream_wrapper_c& operator >> (istream_wrapper_c& is, T& x);
	public:
		istream_wrapper_c(std::istream& is);

		istream_wrapper_c(std::istream& is, std::size_t offs, std::size_t len);

		istream_wrapper_c(const char* data, std::size_t len);

		~istream_wrapper_c();

		std::streampos current_pos() const;
		std::streamsize size_to_end() const;
		std::size_t size() const
		{
			return m_data_len;
		}
		const char* data() const
		{
			return m_data;
		}
		const char* rd_ptr() const
		{
			return m_data + m_data_pos;
		}
		void advance(std::streampos delta);
		void seek(std::streampos pos);
		void read(char* buff, std::size_t size);

		void assert_word(uint16_t word);
		void assert_dword(uint32_t word);
		void assert_string(const wchar_t* s, bool align = true);
		
		bool check_string(const wchar_t* s, bool align = true);

		void assert_space(uint64_t s) const;

		void align4();

		void read_string(std::wstring& result, std::size_t n, bool ensure_null);
	private:
		void _seek(std::streampos pos, bool truncate);
	private:
		std::istream* stream;

		const char* m_data;
		std::size_t m_data_len;
		std::size_t m_data_pos;

		bool m_is_owner;

	};

	template <typename T>
	inline
	istream_wrapper_c& operator >> (istream_wrapper_c& is, T& x)
	{
		if (is.m_data)
		{
			if (is.m_data_pos + sizeof(T) <= is.m_data_len)
			{
				union
				{
					const char* bytes;
					const T* words;
				} u;
				u.bytes = is.m_data + is.m_data_pos;
				x = *u.words;
				//std::memcpy(&x, is.m_data + is.m_data_pos, sizeof(T));
				is.m_data_pos += sizeof(T);
			}
			else
			{
				throw std::runtime_error("I/O error");
			}
		}
		else
		{
			is.stream->read(reinterpret_cast<char*> (&x), sizeof(x));
			if (!(*is.stream))
			{
				throw std::runtime_error("I/O error");
			}
		}
		return is;
	}

	template <typename T>
	static const T* load_struct(istream_wrapper_c& is)
	{
		union
		{
			const char* bytes;
			const T* data;
		} u;
		u.bytes = is.rd_ptr();
		is.advance(sizeof(T));
		return u.data;
	}
} // ns bsw
#endif
