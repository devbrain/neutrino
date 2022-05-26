#include <iostream>
#include "spy/common/am/icons/load_icon.hpp"
#include "bsw/fs/istream_wrapper.hpp"
#include "bsw/byte_order.hpp"

namespace spy
{
    namespace am
    {

#define PNG_MAGIC 0x474e5089 

        struct bitmap_header_s
        {
            uint32_t size;
            int32_t width;
            int32_t height;
            uint16_t planes;
            uint16_t bit_count;
            uint32_t compression;
            uint32_t size_image;
            int32_t x_pels_per_meter;
            int32_t y_pels_per_meter;
            uint32_t clr_used;
            uint32_t clr_important;
        }; 

#define bitmap_header_size 40

        struct rgb_quad_s
        {
            uint8_t blue;
            uint8_t green;
            uint8_t red;
            uint8_t reserved;
        }; 

#define ROW_BYTES(bits) ((((bits) + 31) >> 5) << 2)

        inline uint32_t simple_vec (const std::vector <uint8_t>& data, uint32_t ofs, uint8_t size)
        {
            switch (size) 
            {
            case 1:
                return (data[ofs/8] >> (7 - ofs%8)) & 1;
            case 2:
                return (data[ofs/4] >> ((3 - ofs%4) << 1)) & 3;
            case 4:
                return (data[ofs/2] >> ((1 - ofs%2) << 2)) & 15;
            case 8:
                return data[ofs];
            case 16:
                return data[2*ofs] | data[2*ofs+1] << 8;
            case 24:
                return data[3*ofs] | data[3*ofs+1] << 8 | data[3*ofs+2] << 16;
            case 32:
                return data[4*ofs] | data[4*ofs+1] << 8 | data[4*ofs+2] << 16 | data[4*ofs+3] << 24;
            }

            return 0;
        }
        
        
        // ============================================================================================
		bool load_icon(std::vector <unsigned char>& out, unsigned& w, unsigned& h, std::streampos offs, uint32_t size, std::istream& stream)
		{
			if (size < bitmap_header_size)
			{
				return false;
			}
			bsw::istream_wrapper_c is(stream);
			return load_icon(out, w, h, offs, size, is);
		}


		

		bool load_icon_dimensions (unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size, bsw::istream_wrapper_c& is)
		{
			if (size < bitmap_header_size)
			{
				return false;
			}
			//std::streampos has_bytes = 0;

			is.seek(offs);
			bitmap_header_s bi;
			is >> bi.size;
			if (bi.size == PNG_MAGIC)
			{
				// Vista icons
				is.seek(offs);
				int32_t header[1 + 1 + 1 + 1];
				is.read((char*)header, sizeof (header));

				w = bsw::byte_order_c::fromNetwork(header[2]);
				h = bsw::byte_order_c::fromNetwork(header[3]);
				bpp = 24;
			}
			else
			{
				is >> bi.width >> bi.height >> bi.planes
					>> bi.bit_count >> bi.compression
					>> bi.size_image >> bi.x_pels_per_meter
					>> bi.y_pels_per_meter >> bi.clr_used >> bi.clr_important;
				bpp = bi.bit_count;
				if (bi.compression)
				{
					return false;
				}
				bool negative_order = bi.height < 0;
				const uint32_t width = bi.width;
				const uint32_t total_height = negative_order ? (-bi.height) : (bi.height);

				if (total_height & 1)
				{
					// total_height should be divisible by 2
					return false;
				}
				const uint32_t height = total_height / 2;

				w = width;
				h = height;
			}
			return true;
		}

		bool load_icon_dimensions(unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size, std::istream& stream)
		{
			if (size < bitmap_header_size)
			{
				return false;
			}
			bsw::istream_wrapper_c is(stream);
			return load_icon_dimensions(w, h, bpp, offs, size, is);
		}
		bool load_icon_dimensions(unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size, const char* data, std::size_t data_size)
		{
			if (size < bitmap_header_size)
			{
				return false;
			}
			bsw::istream_wrapper_c is(data, data_size);
			return load_icon_dimensions(w, h, bpp, offs, size, is);
		}
        bool load_icon (std::vector <unsigned char>& out, unsigned& w, unsigned& h,  
            std::streampos offs, uint32_t size, bsw::istream_wrapper_c& is)
        {
            if (size < bitmap_header_size)
            {
                return false;
            }
            std::streampos has_bytes = 0;
            
            is.seek (offs);
            bitmap_header_s bi;
            is >> bi.size;
            if (bi.size == PNG_MAGIC)
            {
                // Vista icons
                is.seek (offs);
                out.resize (size);
                is.read (reinterpret_cast <char*> (out.data ()), size);
                w = 0;
                h = 0;
            }
            else
            {
                is >> bi.width >> bi.height >> bi.planes 
                    >> bi.bit_count >> bi.compression
                    >> bi.size_image >> bi.x_pels_per_meter 
                    >> bi.y_pels_per_meter >> bi.clr_used >> bi.clr_important;
                if (bi.compression)
                {
                    return false;
                }
                bool negative_order = bi.height < 0;
                const uint32_t width = bi.width;
                const uint32_t total_height = negative_order ? (-bi.height) : (bi.height); 

                if (total_height & 1)
                {
                    // total_height should be divisible by 2
                    return false;
                }
                const uint32_t height = total_height / 2;

                w = width;
                h = height;

                has_bytes += bitmap_header_size;
                if (bi.size != bitmap_header_size)
                {
                    if (bi.size <  bitmap_header_size )
                    {
                        return false;
                    }
                    const std::streampos delta = bi.size - bitmap_header_size;
                    if (delta + has_bytes > size)
                    {
                        return false;
                    }
                    is.advance (bi.size - bitmap_header_size);
                    has_bytes += delta;
                }

                rgb_quad_s pal [256];
                unsigned int palette_count = 0;
                if (bi.clr_used != 0 || bi.bit_count < 24) 
                {
                    palette_count = (bi.clr_used != 0 ? bi.clr_used : 1 << bi.bit_count);
                    if (palette_count > 256)
                    {
                        return false;
                    }
                    const std::streampos to_read = palette_count*sizeof (rgb_quad_s);
                    if (has_bytes + to_read > size)
                    {
                        return false;
                    }
                    is.read (reinterpret_cast <char*>(pal), static_cast <size_t>(to_read));	
                    has_bytes += to_read;
                } 

                const uint32_t row_size = ROW_BYTES(width * bi.bit_count);
                const uint32_t image_size = height * row_size;
                const uint32_t mask_size = height * ROW_BYTES(width); 
                has_bytes += (static_cast <std::streampos>(image_size) + static_cast <std::streampos>(mask_size));
                if (has_bytes > size)
                {
                    return false;
                }
                out.resize (width * height * 4);


                std::vector <uint8_t> image_data (image_size);
                std::vector <uint8_t> mask_data (mask_size);
                is.read ((char*)image_data.data (), image_data.size ());
                is.read ((char*)mask_data.data (), mask_data.size ());

                uint8_t* row = (uint8_t*)out.data ();

                for (uint32_t d = 0; d < height; d++) 
                {
                    uint32_t x;
                    uint32_t y = (bi.height < 0 ? d : height - d - 1);
                    uint32_t imod = y * (image_size / height) * 8 / bi.bit_count;
                    uint32_t mmod = y * (mask_size / height) * 8;

                    for (x = 0; x < width; x++) 
                    {
                        uint32_t color = simple_vec (image_data, x + imod, static_cast <uint8_t>(bi.bit_count));

                        if (bi.bit_count <= 16) 
                        {
                            if (color >= palette_count) 
                            {
                                return false;
                            }
                            row [4*x+0] = pal [color].red;
                            row [4*x+1] = pal [color].green;
                            row [4*x+2] = pal [color].blue;
                        } 
                        else 
                        {
                            row [4*x+0] = (color >> 16) & 0xFF;
                            row [4*x+1] = (color >>  8) & 0xFF;
                            row [4*x+2] = (color >>  0) & 0xFF;
                        }
                        if (bi.bit_count == 32)
                        {
			  row [4*x+3] = static_cast<uint8_t>((color >> 24) & 0xFF);
                        }
                        else
                        {
                            row [4*x+3] = simple_vec (mask_data, x + mmod, 1) ? 0 : 0xFF;
                        }
                    }
                    row += (4*width);
                } 
            }
            return true;
        }
        // ==========================================================================================
#if defined(_MSC_VER)
        struct icon_info_guard_s : public ICONINFO 
        {
            icon_info_guard_s () 
            {
                hbmColor = NULL;
                hbmMask = NULL;
            }

            ~icon_info_guard_s() 
            {
                if (hbmColor)
                {
                    ::DeleteObject (hbmColor);
                }
                if (hbmMask)
                {
                    ::DeleteObject (hbmMask);
                }
            }
        };
        // ----------------------------------------------------------------------------------
        struct hdc_guard_s
        {
            hdc_guard_s (HDC& hdc)
                : m_hdc (hdc)
            {
            }
            ~hdc_guard_s ()
            {
                if (m_hdc != NULL)
                {
                    ReleaseDC (NULL, m_hdc);
                }
            }
            HDC& m_hdc;
        };
        // ----------------------------------------------------------------------------------
        static bool get_pixels (HBITMAP& bmp, std::vector <unsigned char>& pixels, int& w, int& h, uint16_t& bit_count, bool is_xor_mask)
        {
            BITMAPINFO info;
            ZeroMemory (&info, sizeof (BITMAPINFO));  
            info.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);

            //Get the size of the bitmap
            HDC hdc = GetDC(NULL);  
            if (hdc == NULL)
            {
                return false;
            }
            hdc_guard_s hdcg (hdc);

            UINT dib_usage = is_xor_mask ? DIB_PAL_COLORS : DIB_RGB_COLORS;
            if (0 == GetDIBits (hdc, bmp, 0, 0, NULL, &info, dib_usage))
            {
                return false;
            }

            

            //get pixel data in 24bit format
            info.bmiHeader.biSize = sizeof (info.bmiHeader);
            if (!is_xor_mask)
            {
                info.bmiHeader.biBitCount = 24;
                info.bmiHeader.biCompression = BI_RGB;
            }
            else
            {
                info.bmiHeader.biBitCount = 1;
                info.bmiHeader.biCompression = BI_RGB;
            }

            w = info.bmiHeader.biWidth;
            h = info.bmiHeader.biHeight;
            bit_count = info.bmiHeader.biBitCount;

            bool negative = (info.bmiHeader.biHeight < 0);
            info.bmiHeader.biHeight = (info.bmiHeader.biHeight < 0) ? (-info.bmiHeader.biHeight) : (info.bmiHeader.biHeight);  // correct the bottom-up ordering of lines
            /*
            const std::size_t bit_size = info.bmiHeader.biWidth*info.bmiHeader.biBitCount;
            std::size_t byte_size = bit_size / 8;
            if (bit_size % 8) 
            {
                byte_size++;
            }
            */
            const std::size_t byte_size = info.bmiHeader.biHeight * ROW_BYTES(info.bmiHeader.biWidth*info.bmiHeader.biBitCount);

            pixels.resize (byte_size);

            if (int rc = GetDIBits(hdc, bmp, 0, info.bmiHeader.biHeight, (LPVOID)pixels.data (), &info, dib_usage) == 0)
            {
                
                return false;
            }

           
            return true;
        }
        // ----------------------------------------------------------------------------------
        bool load_icon (HICON hicon, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
        {
            icon_info_guard_s icon_info;
            if (!::GetIconInfo (hicon, &icon_info))
            {
                return false;
            }

            if (!icon_info.fIcon)
            {
                return false;
            }
            std::vector <unsigned char> raw_data;
            int rw, rh;
            uint16_t bpp;
            if (icon_info.hbmColor)
            {
                if (!get_pixels (icon_info.hbmColor, raw_data, rw, rh, bpp, false))
                {
                    return 0;
                }
                
            }
            if (raw_data.empty ())
            {
                return false;
            }
            std::vector <unsigned char> xor_data;
            int xw, xh;
            uint16_t xbpp;
            if (icon_info.hbmMask && bpp != 32)
            {
                get_pixels (icon_info.hbmMask, xor_data, xw, xh, xbpp, true);
            }

            w = rw;
            h = rh < 0 ? -rh : rh;

            const uint32_t row_size = ROW_BYTES(rw * bpp);
            const uint32_t image_size = h * row_size;
            const uint32_t mask_size = h * ROW_BYTES(rw); 
            //const uint32_t mask_size = xor_data.size (); 
            out.resize (w*h*4);

            uint8_t* row = (uint8_t*)out.data ();

            for (uint32_t d = 0; d < h; d++) 
                {
                    uint32_t x;
                    uint32_t y = (rh < 0 ? d : h - d - 1);
                    uint32_t imod = y * (image_size / h) * 8 / bpp;
                    uint32_t mmod = y * (mask_size / h) * 8;

                    for (x = 0; x < w; x++) 
                    {
                        uint32_t color = simple_vec (raw_data, x + imod, static_cast <uint8_t>(bpp));
                        row [4*x+0] = (color >> 16) & 0xFF;
                        row [4*x+1] = (color >>  8) & 0xFF;
                        row [4*x+2] = (color >>  0) & 0xFF;
                        if (!xor_data.empty ())
                        {
                           row [4*x+3] = simple_vec (xor_data, x + mmod, 1) ? 0 : 0xFF;
                        }
                        else
                        {
                            row [4*x+3] = 0xFF;
                        }
                    }
                     row += (4*w);
            }
            return true;
        }
#endif
    } // ns am
} // ns spy


