#ifndef __SPY_AM_ICONS_LOAD_ICON_HPP__
#define __SPY_AM_ICONS_LOAD_ICON_HPP__

#include <iosfwd>
#include <vector>
#include <stdint.h>

#if defined(_MSC_VER)
#include <Windows.h>
#endif

namespace bsw
{
	class istream_wrapper_c;
} // ns bsw

namespace spy
{ 
    namespace am
    {
        // if w = h = 0, then out is png data
        bool load_icon (std::vector <unsigned char>& out, unsigned& w, unsigned& h, std::streampos offs, uint32_t size, std::istream& stream);
		bool load_icon (std::vector <unsigned char>& out, unsigned& w, unsigned& h, std::streampos offs, uint32_t size, bsw::istream_wrapper_c& wrapper);
		bool load_icon_dimensions(unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size, std::istream& stream);
		bool load_icon_dimensions(unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size, const char* data, std::size_t data_size);
		
#if defined(_MSC_VER)
        bool load_icon (HICON hicon, std::vector <unsigned char>& out, unsigned& w, unsigned& h);
#endif
    } // ns am
} // ns spy

#endif
