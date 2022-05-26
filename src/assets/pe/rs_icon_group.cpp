#include "rs_icon_group.hpp"

namespace pefile
{
	void icon_group_c::_add_group(uint32_t group_id, uint16_t ordinal)
	{
		m_grp.insert(icon_group_t::value_type(group_id, ordinal));
	}
	// ------------------------------------------------------------------
	icon_group_c::icon_group_t::const_iterator icon_group_c::begin() const
	{
		return m_grp.begin();
	}
	// ------------------------------------------------------------------
	icon_group_c::icon_group_t::const_iterator icon_group_c::end() const
	{
		return m_grp.end();
	}
	// ------------------------------------------------------------------
	namespace
	{
		struct icon_grp_header_s
		{
			icon_grp_header_s(bsw::istream_wrapper_c& is)
				: wReserved(0), wType(0), wCount(0)
			{
				is >> wReserved;
				is >> wType;
				is >> wCount;
			}
			uint16_t   wReserved;          /* Currently zero  */
			uint16_t   wType;              /* 1 for icons */
			uint16_t   wCount;             /* Number of components */
		};
	}
	// ------------------------------------------------------------------
	void icon_group_c::load(const file_c& file, const resource_c& rn, icon_group_c& out)
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		auto offs = rn.offset_in_file(file);

		if (offs >= file_size)
		{
			return;
		}
		bsw::istream_wrapper_c stream(file_data + offs, rn.size());
	
		icon_grp_header_s hdr(stream);
		if (hdr.wType != 1)
		{
			return;
		}
		for (uint16_t i = 0; i<hdr.wCount; i++)
		{
			stream.advance(4 * sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint32_t));
			uint16_t ordinal;
			stream >> ordinal;
			out._add_group(rn.name().id(), ordinal);
		}
	}
	// ===================================================================
	icon_info_s::icon_info_s(uint16_t id, uint64_t offs, uint32_t s)
		: ordinal(id),
		offset(offs),
		size(s)
	{
	}
	// ------------------------------------------------------------------
	icon_info_s::icon_info_s()
		: ordinal(0xFFFF),
		offset(0),
		size(0)
	{

	}
	// ------------------------------------------------------------------
	void icon_info_s::load(const file_c& file, const resource_c& rn, icon_info_s& out)
	{
		
		out.ordinal = 0xFFFF;
		if (rn.name().is_id())
		{
			out.ordinal = rn.name().id();
		}
		
		out.offset = rn.offset_in_file(file);
		out.size = rn.size();
	}
} // ns pefile
