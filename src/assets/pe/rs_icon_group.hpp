#ifndef __PEFILE_RS_ICON_GROUP_HPP__
#define __PEFILE_RS_ICON_GROUP_HPP__

#include <map>
#include "resource_directory.hpp"
#include "pefile.hpp"
namespace pefile
{
	class icon_group_c
	{
	public:
		typedef std::multimap <uint32_t, uint16_t> icon_group_t;
	public:
		static constexpr int resource_id()
		{
			return 14;
		}

		static constexpr bool singleton()
		{
			return false;
		}

		icon_group_t::const_iterator begin() const;
		icon_group_t::const_iterator end() const;
		static void load(const file_c& file, const resource_c& rn, icon_group_c& out);
	private:
		void _add_group(uint32_t group_id, uint16_t ordinal);
	private:
		icon_group_t m_grp;
	};

	struct icon_info_s
	{
		static constexpr int resource_id()
		{
			return 3;
		}

		static constexpr bool singleton()
		{
			return false;
		}

		icon_info_s(uint16_t id, uint64_t offs, uint32_t s);
		icon_info_s();
		uint16_t ordinal;
		uint64_t offset;
		uint32_t size;

		static void load(const file_c& file, const resource_c& rn, icon_info_s& out);
	};

} // ns pefile

#endif
