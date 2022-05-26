#include "rs_version.hpp"
#include "wchar.hpp"

namespace pefile
{
	// ------------------------------------------------------------------
	void version_c::_bind(const std::wstring& k, const std::wstring& b)
	{
		m_kv_map[bsw::wstring_to_utf8(k)] = bsw::wstring_to_utf8(b);
	}
	// ------------------------------------------------------------------
	uint32_t version_c::operator [] (fields_t f) const
	{
		return m_fields[f];
	}
	// ------------------------------------------------------------------
	std::string version_c::operator [] (const std::string& v) const
	{
		auto i = m_kv_map.find(v);
		if (i == m_kv_map.end())
		{
			return "";
		}
		return i->second;
	}
	// ------------------------------------------------------------------
	version_c::kv_map_t::const_iterator version_c::begin() const
	{
		return m_kv_map.begin();
	}
	// ------------------------------------------------------------------
	version_c::kv_map_t::const_iterator version_c::end() const
	{
		return m_kv_map.end();
	}
	// ------------------------------------------------------------------
	std::size_t version_c::size() const
	{
		return m_kv_map.size();
	}
	// ------------------------------------------------------------------
	version_c::translations_t::const_iterator version_c::translations_begin() const
	{
		return m_translations.begin();
	}
	// ------------------------------------------------------------------
	version_c::translations_t::const_iterator version_c::translations_end() const
	{
		return m_translations.end();
	}
	// ------------------------------------------------------------------
	std::size_t version_c::translations_size() const
	{
		return m_translations.size();
	}
	// ------------------------------------------------------------------
	void version_c::_add_translation(uint16_t x)
	{
		m_translations.push_back(x);
	}
	// ====================================================================
	namespace
	{
		struct node_s
		{
			node_s(bsw::istream_wrapper_c& is)
				: cbNode(0),
				cbData(0),
				wType(0),
				pos(0)
			{
				pos = is.current_pos();
			}

			void unread(bsw::istream_wrapper_c& is)
			{
				is.seek(pos);
			}

			uint16_t cbNode;
			uint16_t cbData;
			uint16_t wType;
			uint64_t pos;


		};

		struct vs_version_info_s : public node_s
		{
			uint16_t Size;
			static const uint16_t version_sec_size = 52;

			vs_version_info_s(bsw::istream_wrapper_c& is, uint16_t expected_size)
				: node_s(is), Size(0)
			{
				is >> Size;


				is.assert_word(version_sec_size);
				is.assert_word(0);
				is.assert_string(L"VS_VERSION_INFO");

				cbNode = expected_size;
				cbData = version_sec_size;
				wType = 0;
			}
		};

		struct named_node_s : public node_s
		{
			named_node_s(bsw::istream_wrapper_c& is)
				: node_s(is)
			{
				is >> cbNode;
				is >> cbData;
				is >> wType;
				const auto max_read = 1 + (cbNode / sizeof(wchar_t));
				if (cbNode)
				{
					is.read_string(name, max_read, true);
					is.align4();
				}
			}
			std::wstring name;

		};

		struct text_node_s : public node_s
		{
			text_node_s(bsw::istream_wrapper_c& is)
				: node_s(is)
			{
				is >> cbNode;
				is >> cbData;
				is >> wType;
			}
		};
	}
	// ------------------------------------------------------------------------
	static std::vector <uint16_t> parse_ver_translations(bsw::istream_wrapper_c& is, std::size_t& to_skip)
	{
		std::vector <uint16_t> out;
		named_node_s tr(is);
		to_skip = tr.cbNode;
		if (tr.name == L"Translation")
		{
			uint16_t num = tr.cbData / 2;
			out.reserve(num);
			for (uint16_t i = 0; i < num; i++)
			{
				uint16_t trid;
				is >> trid;
				out.push_back(trid);
			}
		}
		return out;
	}
	// -------------------------------------------------------------------
	void version_c::load(const file_c& file, const resource_c& rn, version_c& out)
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		auto offs = rn.offset_in_file(file);

		if (offs >= file_size)
		{
			return;
		}

		bsw::istream_wrapper_c stream(file_data + offs, rn.size ());

		vs_version_info_s  vs_version_info(stream, static_cast<uint16_t>(rn.size()));

		union
		{
			char* bytes;
			uint32_t* w;
		} u;
		
		u.w = &out.m_fields[0];
		stream.read(u.bytes, sizeof(uint32_t)* version_c::MAX_FIELD);

		if (out.m_fields[version_c::dwSignature] != 0xfeef04bd)
		{
			throw std::runtime_error("Bad VERSION_INFO signature");
		}
		//bool zero_ver = (out.m_fields[version_c::dwStrucVersion] == 0);
		if (out.m_fields[version_c::dwStrucVersion] > 0x00010000)
		{
			throw std::runtime_error("Bad VERSION_INFO version");
		}
		stream.align4();

		uint64_t has_bytes = stream.current_pos();

		while (has_bytes + 6 < vs_version_info.Size)
		{
			const uint16_t string_bytes = static_cast <uint16_t>(stream.current_pos());

			named_node_s nm(stream);

			if (nm.name == L"StringFileInfo")
			{
				named_node_s lang(stream);
				if (lang.name.size() != 8)
				{
					throw std::runtime_error("Bad language code");
				}
				stream.align4();

				const uint16_t string_table_len = nm.cbNode;
				while (((uint16_t)stream.current_pos() - string_bytes) < string_table_len)
				{
					auto enter = stream.current_pos();
					text_node_s tn(stream);
					if (tn.cbNode == 0)
					{
						tn.unread(stream);
						break;
					}
					
					std::wstring key;
					std::size_t max_key_len = (tn.cbNode - 6)/ sizeof(wchar_t);
					stream.read_string(key, max_key_len + 1, true);
					stream.align4();

					if (key == L"VarFileInfo")
					{
						std::size_t sz;
						for (auto tr : parse_ver_translations(stream, sz))
						{
							out._add_translation(tr);
						}
					}
					else
					{
						std::wstring value;
						if (tn.cbData)
						{
							auto now = stream.current_pos();
							std::size_t delta = static_cast<std::size_t>(now - enter);
							max_key_len = tn.cbData + 1;
							if (delta < tn.cbNode)
							{
								max_key_len = tn.cbNode - delta;
							}
							if (max_key_len > stream.size_to_end())
							{
								max_key_len = static_cast<std::size_t>(stream.size_to_end());
							}
							stream.read_string(value, max_key_len, false);
							
							if (tn.pos + tn.cbNode >= stream.size())
							{
								break;
							}
							stream.seek(tn.pos + tn.cbNode);
							if (stream.size_to_end() < 4)
							{
								break;
							}
							stream.align4();
						}
						out._bind(key, value);
					}
				}
			}
			else
			{
				if (nm.name == L"VarFileInfo")
				{
					std::size_t sz = 0;
					for (auto tr : parse_ver_translations(stream, sz))
					{
						out._add_translation(tr);
					}

				}
			}

			has_bytes = stream.current_pos();

		}
		
	}
} // ns pefile
