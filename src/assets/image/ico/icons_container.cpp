#include <algorithm>
#include "spy/common/am/icons/icons_container.hpp"

namespace spy
{
    namespace am
    {
        icons_container_c::icons_container_c ()
        {
        }

        icons_container_c::~icons_container_c ()
        {
        }

        void icons_container_c::add_group (uint32_t group_id, uint16_t ordinal)
        {
            m_icon_group.insert (icon_group_t::value_type (group_id, ordinal));

            group_names_t::const_iterator itr = std::lower_bound (m_group_names.begin (), m_group_names.end (), group_id);
            if (itr == m_group_names.end ())
            {
                m_group_names.push_back (group_id);
            }
            else
            {
                const std::size_t pos = itr - m_group_names.begin ();
                if (*itr != group_id)
                {
                    const std::size_t rng = m_group_names.size ();

                    m_group_names.resize (rng + 1);
                    std::copy_backward (m_group_names.begin () + pos, m_group_names.begin () + rng, m_group_names.end ());
                    m_group_names [pos] = group_id;


                }

            } 
        }
        // ---------------------------------------------------------------------------------------------
        void icons_container_c::add_ordinal (uint16_t ordinal, std::streampos offs, uint32_t size)
        {
            m_icons_map.insert (icon_map_t::value_type (ordinal, icon_info_s (offs, size)));
        }
        // ---------------------------------------------------------------------------------------------
        bool icons_container_c::get_by_id (int id, std::streampos& offset, uint32_t& size) const
        {
            uint16_t group_name;
            if (id < 0)
            {
                group_name = static_cast <uint16_t> (-id);        
            }
            else
            {
                if ((unsigned int)id > m_group_names.size ())
                {
                    return false;
                }
                if (static_cast<std::size_t>(id) == m_group_names.size ())
                {
                    if (m_group_names.empty ())
                    {
                        return false;
                    }
                    group_name = static_cast<uint16_t>(m_group_names [id-1]);
                }
                else
                {
		  group_name = static_cast<uint16_t>(m_group_names [id]);
                }
            }
            std::pair <icon_group_t::const_iterator, icon_group_t::const_iterator> itr = 
                m_icon_group.equal_range (group_name);
            icon_group_t::const_iterator p = std::min_element (itr.first, itr.second);
            if (p == itr.second)
            {
                return false;
            }
            uint16_t ordinal = p->second;
            return get_by_ordinal (ordinal, offset, size);
        }
        // -----------------------------------------------------------------------------
        bool icons_container_c::get_by_ordinal (uint16_t ordinal, std::streampos& offset, uint32_t& size) const
        {
            icon_map_t::const_iterator i = m_icons_map.find (ordinal);
            if (i == m_icons_map.end ())
            {
                return false;
            }
            offset = i->second.offset;
            size = i->second.size;
            return true;
        }
        // -----------------------------------------------------------------------------
        icons_container_c::group_iterator icons_container_c::groups_begin () const
        {
            return m_icon_group.begin ();
        }
        // -----------------------------------------------------------------------------
        icons_container_c::group_iterator icons_container_c::groups_end   () const
        {
            return m_icon_group.end ();
        }
        // -----------------------------------------------------------------------------
        bool icons_container_c::groups_empty () const
        {
            return m_icon_group.empty ();
        }
		// -----------------------------------------------------------------------------
		std::pair<icons_container_c::group_iterator, icons_container_c::group_iterator> icons_container_c::group_range(int g) const
		{
			return m_icon_group.equal_range(g);
		}
        // -----------------------------------------------------------------------------
        icons_container_c::icon_iterator icons_container_c::icons_begin () const
        {
            return m_icons_map.begin ();
        }
        // -----------------------------------------------------------------------------
        icons_container_c::icon_iterator icons_container_c::icons_end () const
        {
            return m_icons_map.end ();
        }
        // -----------------------------------------------------------------------------
        bool icons_container_c::icons_empty () const
        {
            return m_icons_map.empty ();
        }
        // -----------------------------------------------------------------------------
        bool icons_container_c::empty () const
        {
            return icons_empty () && groups_empty ();
        }
    } // ns am
} // ns spy
