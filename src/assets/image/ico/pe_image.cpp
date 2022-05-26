#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "spy/common/am/icons/pe_image.hpp"
#include "bsw/fs/istream_wrapper.hpp"
#include "exefile.hpp"

namespace spy
{
    namespace am
    {
		static void load(pefile::exe_file_c& exe, icons_container_c& container)
		{
			
			std::vector<pefile::icon_group_c> group;
			exe.load_resource(group);
			for (const auto& g : group)
			{
				for (auto j : g)
				{
					container.add_group(j.first, j.second);
				}
			}

			std::vector<pefile::icon_info_s> icons;
			exe.load_resource(icons);
			for (const auto& ii : icons)
			{
				container.add_ordinal(ii.ordinal, ii.offset, ii.size);
			}
		}

        pe_image_c::pe_image_c (const std::string& path, icons_container_c& container)
        {
			pefile::exe_file_c exe(path);
			load(exe, container);
        }
		// ----------------------------------------------------------
		pe_image_c::pe_image_c(const std::wstring& path, icons_container_c& container)
		{
			pefile::exe_file_c exe(path);
			load(exe, container);
		}
        // ----------------------------------------------------------
		pe_image_c::pe_image_c(const char* data, std::size_t size, icons_container_c& container)
		{
			pefile::exe_file_c exe(data, size);
			load(exe, container);
		}
    } // ns am
} // ns spy
