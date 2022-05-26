#ifndef __SPY_AM_ICONS_NE_IMAGE_HPP__
#define __SPY_AM_ICONS_NE_IMAGE_HPP__

#include <iosfwd>
#include "spy/common/am/icons/icons_container.hpp"

namespace bsw
{
	class istream_wrapper_c;
}

namespace spy
{
    namespace am
    {
        class ne_image_c
        {
        public:
            ne_image_c (std::istream& is, icons_container_c& container);
			ne_image_c (bsw::istream_wrapper_c& is, icons_container_c& container);
			ne_image_c (const char* data, std::size_t size, icons_container_c& container);
            ~ne_image_c ();
        private:
            icons_container_c& m_container;
		private:
			void _load(bsw::istream_wrapper_c& is);
        };
    } // ns am
} // ns spy


#endif
