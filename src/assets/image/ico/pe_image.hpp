#ifndef __SPY_AM_ICONS_PE_IMAGE_HPP__
#define __SPY_AM_ICONS_PE_IMAGE_HPP__

#include <iosfwd>
#include <vector>
#include <string>
#include "spy/common/am/icons/icons_container.hpp"

namespace spy
{
    namespace am
    {
        class pe_image_c
        {
        public:
            pe_image_c (const std::string& path, icons_container_c& container);
			pe_image_c(const std::wstring& path, icons_container_c& container);
			pe_image_c (const char* data, std::size_t size, icons_container_c& container);
            ~pe_image_c () = default;

			pe_image_c(const pe_image_c&) = delete;
			pe_image_c& operator = (const pe_image_c&) = delete;
     
        };
    } // ns am
} // ns icons

#endif
