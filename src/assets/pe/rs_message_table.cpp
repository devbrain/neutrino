#include "rs_message_table.hpp"
#include "wchar.hpp"

namespace pefile
{
	message_table_c::msg_table_t::const_iterator message_table_c::begin() const
	{
		return m_messages.begin();
	}
	// ---------------------------------------------------------------------
	message_table_c::msg_table_t::const_iterator message_table_c::end() const
	{
		return m_messages.end();
	}
	// ---------------------------------------------------------------------
	bool message_table_c::exists(uint16_t key) const
	{
		return m_messages.find(key) != m_messages.end();
	}
	// ---------------------------------------------------------------------
	std::wstring message_table_c::operator [] (uint16_t key) const
	{
		auto i = m_messages.find(key);
		if (i != m_messages.end())
		{
			return i->second;
		}
		return L"";
	}
	// ----------------------------------------------------------------------
	namespace
	{
		struct msg_block_s
		{
			uint32_t idlo;
			uint32_t idhi;
			uint32_t offs;
		};
	}

	void message_table_c::load(const file_c& file, const resource_c& rn, message_table_c& out)
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		auto offs = rn.offset_in_file(file);

		if (offs >= file_size)
		{
			return;
		}

		bsw::istream_wrapper_c is(file_data + offs, rn.size());


		uint32_t num_blocks;
		is >> num_blocks;

		std::vector <msg_block_s> blocks;
		blocks.reserve(num_blocks);
		for (uint32_t i = 0; i < num_blocks; i++)
		{
			msg_block_s b;
			is >> b.idlo >> b.idhi >> b.offs;
			if (b.idlo > b.idhi)
			{
				throw std::runtime_error("b.idlo > b.idhi");
			}
			blocks.push_back(b);
		}

		for (auto b : blocks)
		{
			is.seek(b.offs);
			
			uint16_t id = static_cast <uint16_t>(b.idlo);
			while (id < b.idhi)
			{
				if (is.size_to_end() < 4)
				{
					break;
				}
				uint16_t len;
				uint16_t isunicode;
				is >> len >> isunicode;
				if (len < 4)
				{
					throw std::runtime_error("Illegal message block length");
				}
				if (is.size_to_end() < len - 4)
				{
					break;
				}
				std::vector <char> text(len - 4);
				is.read(text.data(), text.size());
				is.align4();
				
				if (isunicode)
				{
					union
					{
						char* c;
						wchar_t* w;
					} u;
					u.c = text.data();
					std::wstring w = u.w;
					out.m_messages[id] = w;
					id++;

				}
				else
				{
					out.m_messages[id] = bsw::utf8_to_wstring(text.data());
					id++;
				}
			}
		}
	}
}
