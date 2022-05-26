#include <stdexcept>

#include "spy/common/am/icons/ne_image.hpp"
#include "bsw/fs/istream_wrapper.hpp"



#define IMAGE_DOS_SIGNATURE                 0x5A4D 
#define IMAGE_NE_SIGNATURE                  0x454E
#define ID_MASK                             0x7FFF
#define TYPE_MASK                           0xFF

#define RES_GROUP_ICON 14
#define RES_ICON 3

namespace spy
{
    namespace am
    {

        struct tname_info_s
        {
            uint16_t rnOffset;
            uint16_t rnLength;
            uint16_t rnFlags;
            uint16_t rnID;
            uint16_t rnHandle;
            uint16_t rnUsage;
        };

        static
            bsw::istream_wrapper_c& operator >> (bsw::istream_wrapper_c& is, tname_info_s& x)
        {
            is >> x.rnOffset >> x.rnLength >> x.rnFlags >> x.rnID >> x.rnHandle >> x.rnUsage;
            return is;
        }

        struct type_info_s
        {
            uint16_t      rtTypeID;
            uint16_t      rtResourceCount;
            uint32_t      rtReserved;
            std::vector <tname_info_s> rtNameInfo;
        };

        static
            bsw::istream_wrapper_c& operator >> (bsw::istream_wrapper_c& is, type_info_s& x)
        {
            is >> x.rtTypeID;
            if (x.rtTypeID == 0)
            {
                return is;
            }
            is >> x.rtResourceCount >> x.rtReserved;
            x.rtNameInfo.resize (x.rtResourceCount);
            for (uint16_t i=0; i<x.rtResourceCount; i++)
            {
                is >> x.rtNameInfo [i];
            }
            return is;
        }

        struct group_icon_dir_entry_s
        {
            uint8_t     bWidth;               // Width, in pixels, of the image
            uint8_t     bHeight;              // Height, in pixels, of the image
            uint8_t     bColorCount;          // Number of colors in image (0 if >=8bpp)
            uint8_t     bReserved;            // Reserved
            uint16_t    wPlanes;              // Color Planes
            uint16_t    wBitCount;            // Bits per pixel
            uint32_t    dwBytesInRes;         // how many bytes in this resource?
            uint16_t    nID;                  // the ID
        };

        static bsw::istream_wrapper_c& operator >> (bsw::istream_wrapper_c& is, group_icon_dir_entry_s& x)
        {
            is >> x.bWidth >> x.bHeight >> x.bColorCount >> x.bReserved >> x.wPlanes 
                >> x.wBitCount >> x.dwBytesInRes >> x.nID;
            return is;
        }


        struct group_icon_dir_s
        {
            uint16_t               idReserved;
            uint16_t               idType;
            uint16_t               idCount;
            std::vector <group_icon_dir_entry_s> idEntries;
        };

        static bsw::istream_wrapper_c& operator >> (bsw::istream_wrapper_c& is, group_icon_dir_s& x)
        {
            is >> x.idReserved >> x.idType >> x.idCount;
            x.idEntries.resize (x.idCount);
            for (uint16_t i=0; i<x.idCount; i++)
            {
                is >> x.idEntries [i];
            }
            return is;
        }

        inline uint16_t translate_id (uint16_t x)
        {
            if (x > 0x8000) 
            {
                return x & ID_MASK;
            }
            return x;
        }

        // -------------------------------------------------------------------------

        ne_image_c::ne_image_c (std::istream& stream, icons_container_c& container)
            : m_container (container)
        {
            bsw::istream_wrapper_c is (stream);
			_load(is);
        }
		// --------------------------------------------------------------------------------
		ne_image_c::ne_image_c(bsw::istream_wrapper_c& is, icons_container_c& container)
			: m_container(container)
		{
			_load(is);
		}
		// --------------------------------------------------------------------------------
		ne_image_c::ne_image_c(const char* data, std::size_t size, icons_container_c& container)
			: m_container(container)
		{
			bsw::istream_wrapper_c is(data, size);
			_load(is);
		}
		// -------------------------------------------------------------------------
		void ne_image_c::_load(bsw::istream_wrapper_c& is)
		{
			std::streampos fsize = is.size_to_end();


			uint16_t old_dos_magic;
			is >> old_dos_magic;
			if (old_dos_magic != IMAGE_DOS_SIGNATURE)
			{
				throw std::runtime_error("Not a MZ file");
			}
			is.advance(26 + 32);
			int32_t lfanew;
			is >> lfanew;
			if (lfanew < 0 || lfanew > fsize)
			{
				throw std::runtime_error("Not a PE file");
			}
			// read coff magic
			is.seek(lfanew);
			uint16_t ne_magic;
			is >> ne_magic;
			if (ne_magic != IMAGE_NE_SIGNATURE)
			{
				throw std::runtime_error("Not a PE file");
			}

			is.advance(34);
			uint16_t resident_tab;
			uint16_t res_table_offset;
			is >> res_table_offset >> resident_tab;
			if (res_table_offset == resident_tab)
			{
				throw std::runtime_error("No resources found");
			}
			is.seek(lfanew + res_table_offset);
			uint16_t align_shift;
			is >> align_shift;

			std::vector <type_info_s> type_info;
			while (true)
			{
				type_info_s ti;
				is >> ti;
				if (ti.rtTypeID == 0)
				{
					break;
				}
				else
				{
					type_info.push_back(ti);
				}
			}
			std::vector <uint8_t> names;
			while (true)
			{
				uint8_t x;
				is >> x;
				if (x == 0)
				{
					break;
				}
				else
				{
					names.push_back(x);
				}
			}
			const std::size_t n = type_info.size();
			uint32_t grp_id = 0;
			for (std::size_t i = 0; i<n; i++)
			{
				const type_info_s& ti = type_info[i];
				if ((ti.rtTypeID & TYPE_MASK) == RES_ICON)
				{
					for (std::size_t j = 0; j<ti.rtNameInfo.size(); j++)
					{
						const tname_info_s& ni = ti.rtNameInfo[j];
						uint32_t offs = (1 << align_shift) * ni.rnOffset;
						uint16_t ordinal = translate_id(ni.rnID);
						uint32_t size = (1 << align_shift) * ni.rnLength;
						m_container.add_ordinal(ordinal, offs, size);
					}
				}
				else
				{
					if ((ti.rtTypeID & TYPE_MASK) == RES_GROUP_ICON)
					{
						for (std::size_t j = 0; j<ti.rtNameInfo.size(); j++)
						{
							const tname_info_s& ni = ti.rtNameInfo[j];
							std::streampos offs = (1 << align_shift) * ni.rnOffset;
							group_icon_dir_s d;
							is.seek(offs);
							is >> d;
							for (uint16_t k = 0; k<d.idCount; k++)
							{
								m_container.add_group(grp_id, translate_id(d.idEntries[k].nID));
							}
							grp_id++;
						}
					}
				}
			}
		}
        // -------------------------------------------------------------------------
        ne_image_c::~ne_image_c ()
        {
        }
    } // ns am
} // ns spy
