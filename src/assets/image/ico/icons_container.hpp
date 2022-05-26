#ifndef __SPY_AM_ICONS_ICONS_CONTAINER_HPP__
#define __SPY_AM_ICONS_ICONS_CONTAINER_HPP__


#include <iosfwd>
#include <map>
#include <vector>
#include <iterator>
#include <stdint.h>

namespace spy
{
    namespace am
    {

        class icons_container_c
        {
        public:
            typedef std::multimap <uint32_t, uint16_t> icon_group_t;
            typedef icon_group_t::const_iterator group_iterator;

            typedef std::vector <uint32_t> group_names_t;
            struct icon_info_s
            {
                icon_info_s (std::streampos offs, uint32_t s)
                    : offset (offs),
                    size (s)
                {
                }
                std::streampos offset;
                uint32_t size;
            };
            typedef std::map <uint16_t, icon_info_s> icon_map_t;
            typedef icon_map_t::const_iterator icon_iterator;
        public:
            icons_container_c ();
            ~icons_container_c ();

            void add_group   (uint32_t group_id, uint16_t ordinal);
            void add_ordinal (uint16_t ordinal, std::streampos offs, uint32_t size);

            bool get_by_id (int id, std::streampos& offset, uint32_t& size) const;
            bool get_by_ordinal (uint16_t ordinal, std::streampos& offset, uint32_t& size) const;



            group_iterator groups_begin () const;
            group_iterator groups_end   () const;

			std::pair<group_iterator, group_iterator> group_range(int g) const;
            bool groups_empty () const;

            icon_iterator icons_begin () const;
            icon_iterator icons_end () const;
            bool icons_empty () const;

            bool empty () const;
        private:
            icons_container_c (const icons_container_c&);
            icons_container_c& operator = (const icons_container_c&);
        private:
            icon_group_t  m_icon_group;
            group_names_t m_group_names;
            icon_map_t    m_icons_map;
        };

    } // ns am
} // ns spy
#endif
