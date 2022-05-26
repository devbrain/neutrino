#ifndef __SPY_AM_ICONS_ICO_IMAGE_HPP__
#define __SPY_AM_ICONS_ICO_IMAGE_HPP__

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
		void ico_image_loader(bsw::istream_wrapper_c& stream, icons_container_c& container);

        void ico_image_loader (std::istream& stream, icons_container_c& container);
    } // ns am
} // ns spy


#endif
