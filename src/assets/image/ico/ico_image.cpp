#include <stdexcept>
#include "spy/common/am/icons/ico_image.hpp"
#include "bsw/fs/istream_wrapper.hpp"

namespace spy
{
    namespace am
    {

        struct icon_dir_s
        {
            uint16_t idReserved;
            uint16_t idType;
            uint16_t idCount;
        };

#define icon_dir_size (sizeof(uint16_t)*3)

        static bsw::istream_wrapper_c& operator >> (bsw::istream_wrapper_c& is, icon_dir_s& x)
        {
            is >> x.idReserved >> x.idType >> x.idCount;
            return is;
        }

        struct group_icon_dir_entry_s
        {
            uint8_t    bWidth;
            uint8_t    bHeight;
            uint8_t    bColorCount;
            uint8_t    bReserved;
            uint16_t   wPlanes;
            uint16_t   wBitCount;
            uint32_t   dwBytesInRes;
            uint32_t   dwImageOffset;
        };


#define bitmap_info_header_size 40
        void repair (group_icon_dir_entry_s& entry)
        {
            // If there is missing information in the header... lets try to calculate it
            if (entry.wBitCount == 0)
            {
                int stride, CLSSize, palette; 
                int bmpSize  = ((uint16_t) entry.dwBytesInRes - bitmap_info_header_size);
                int BWStride = ((entry.bWidth * 1 + 31) & ~31) >> 3;
                int BWSize   = BWStride * entry.bHeight;
                bmpSize     -= BWSize;

                // Lets find the value;
                uint8_t bpp [] = {1, 4, 8, 16, 24, 32};
                int j=0;
                while(j<=5)
                {
                    stride   = ((entry.bWidth * bpp[j] + 31) & ~31) >> 3;
                    CLSSize  = entry.bHeight * stride ;
                    palette  = bpp[j]<=8 ? ((int) (1 << bpp[j]) * 4) : 0;
                    if (palette + CLSSize == bmpSize)
                    {
                        entry.wBitCount = bpp[j];
                        break;
                    }
                    j++;
                }
            }

            if (entry.wBitCount < 8 && entry.bColorCount == 0)
            {
                entry.bColorCount = (uint8_t) (1 << entry.wBitCount);
            }
            if (entry.wPlanes == 0) 
            {
                entry.wPlanes = 1;
            }
        }

        static bsw::istream_wrapper_c& operator >> (bsw::istream_wrapper_c& is, group_icon_dir_entry_s& x)
        {
            is >> x.bWidth >> x.bHeight >> x.bColorCount >> x.bReserved >> x.wPlanes 
                >> x.wBitCount >> x.dwBytesInRes >> x.dwImageOffset;
            return is;
        }

		void ico_image_loader(bsw::istream_wrapper_c& is, icons_container_c& container)
		{
			std::streamsize len = is.size_to_end();

			icon_dir_s icon_dir;
			is >> icon_dir;

			if (icon_dir.idReserved != 0 || icon_dir.idType != 1)
			{
				throw std::runtime_error ("Not an ICO file");
			}
			uint16_t ord = 0;
			for (uint16_t i = 0; i<icon_dir.idCount; i++)
			{
				group_icon_dir_entry_s e;
				is >> e;
				repair(e);
				if (e.wPlanes != 0 && e.wPlanes != 1)
				{
					continue;
				}
				
				bool ok = false;
				
				switch (e.wBitCount)
				{
				case 1:
				case 4:
				case 8:
					
				case 16:
				case 24:
				case 32:
					ok = true;
					break;
				default:
					ok = false;
				}
				if (!ok)
				{
					continue;
				}
				
				if (e.dwImageOffset + e.dwBytesInRes < len)
				{
					container.add_ordinal(ord, e.dwImageOffset, static_cast <uint32_t> (len - e.dwImageOffset));
					container.add_group(0, ord);
					ord++;
				}
			}
		}

        void ico_image_loader (std::istream& stream, icons_container_c& container)
        {
            bsw::istream_wrapper_c is (stream);
			ico_image_loader(is, container);
            
        }

    } // ns am
} // ns spy
