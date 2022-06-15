//
// Created by igor on 09/06/2022.
//

#include "egahead.hh"
#include <boost/pfr.hpp>
#include <neutrino/utils/io/binary_reader.hh>

namespace neutrino::appogee::keen3 {
  std::ostream& operator << (std::ostream& os, const egahead::header& v) {
    os << "Latplansiz : " << v.Latplansiz << std::endl
       << "Sprplansiz : " << v.Sprplansiz << std::endl
       << "Bmpdatstart : " << v.Bmpdatstart << std::endl
       << "Sprdatstart : " << v.Sprdatstart << std::endl
       << "Fontnum : " << v.Fontnum << std::endl
       << "Fontloc : " << v.Fontloc << std::endl
       << "Unknum : " << v.Unknum << std::endl
       << "Unkloc : " << v.Unkloc << std::endl
       << "Tilenum : " << v.Tilenum << std::endl
       << "Tileloc : " << v.Tileloc << std::endl
       << "Bmpnum : " << v.Bmpnum << std::endl
       << "Bmploc : " << v.Bmploc << std::endl
       << "Spritenum : " << v.Spritenum << std::endl
       << "Spriteloc : " << v.Spriteloc << std::endl
       << "Compression : " << v.Compression << std::endl;
    return os;
  }

  std::ostream& operator << (std::ostream& os, const egahead::unmasked_data& v) {
    os << "size_h : " << v.size_h << std::endl
       << "size_w : " << v.size_w << std::endl
       << "loc : " << v.loc << std::endl
       << "name : " << v.name.data () << std::endl;
    return os;
  }

  std::ostream& operator << (std::ostream& os, const egahead::masked_data& v) {
    os << "width : " << v.width << std::endl
       << "height : " << v.height << std::endl
       << "loc_offset : " << v.loc_offset << std::endl
       << "location : " << v.location << std::endl
       << "hitbox_ul : " << v.hitbox_ul_h << " x " << v.hitbox_ul_v << std::endl
       << "Hitbox_br : " << v.hitbox_br_h << " x " << v.hitbox_br_v << std::endl
       << "Name : " << v.Name.data () << std::endl;
    return os;
  }

  template <typename T>
  static T load (std::istream& is) {
    neutrino::utils::io::binary_reader br (is, neutrino::utils::io::binary_reader::stream_byte_order::LITTLE_ENDIAN_BYTE_ORDER);
    T out{};
    boost::pfr::for_each_field (out, [&] (auto& v) {
      br >> v;
    });
    return out;
  }

  egahead::egahead (std::istream& is)
      : m_header (load<egahead::header> (is)) {
    std::cout << m_header << std::endl;
    is.seekg (m_header.Bmpdatstart, std::ios::beg);
    for (uint16_t i = 0; i < m_header.Bmpnum; i++) {
      m_unmasked_data.push_back (load<egahead::unmasked_data> (is));
    }
    is.seekg (m_header.Sprdatstart, std::ios::beg);
    for (uint16_t i = 0; i < m_header.Spritenum; i++) {
      m_masked_data.push_back (load<egahead::masked_data> (is));
    }
    std::cout << " ------------- UNMASKED ------------- " << std::endl;
    for (const auto& x: m_unmasked_data) {
      std::cout << x << std::endl;
    }

    std::cout << " ------------- MASKED ------------- " << std::endl;
    for (const auto& x: m_masked_data) {
      std::cout << x << std::endl;
    }
  }

}