#include "rs_string_table.hpp"

namespace pefile
{
	int string_table_c::number() const
	{
		return m_num;
	}
	// ------------------------------------------------------------------
	string_table_c::strings_map_t::const_iterator string_table_c::begin() const
	{
		return m_strings.begin();
	}
	// ------------------------------------------------------------------
	string_table_c::strings_map_t::const_iterator string_table_c::end() const
	{
		return m_strings.end();
	}
	// ------------------------------------------------------------------
	void string_table_c::_number(int x)
	{
		m_num = x;
	}
	// ------------------------------------------------------------------
	void string_table_c::_bind(int id, const std::wstring& s)
	{
		m_strings[id] = s;
	}
	// ------------------------------------------------------------------
	void string_table_c::load(const file_c& file, const resource_c& rn, string_table_c& out)
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		auto offs = rn.offset_in_file(file);

		if (offs >= file_size)
		{
			return;
		}

		bsw::istream_wrapper_c stream(file_data + offs, rn.size());

		uint16_t id = static_cast<uint16_t>(rn.name().id());

		out._number(id);
		int nums = 16 * (id - 1);
		std::size_t has_bytes = 0;
		std::size_t block_len = rn.size();
		while (has_bytes < block_len)
		{
			uint16_t len;
			stream >> len;
			has_bytes += sizeof(len);
			if (has_bytes + len > block_len)
			{
				break;
			}
			if (len != 0)
			{
				std::wstring w;
				stream.read_string(w, len, false);
				has_bytes += sizeof(wchar_t)*(len);
				out._bind(nums, w);
			}
			nums++;
		}

	}
}