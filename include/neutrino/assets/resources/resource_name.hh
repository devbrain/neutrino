//
// Created by igor on 29/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_NAME_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_NAME_HH

#include <neutrino/utils/id.hh>
#include <string_view>
#include <string>

namespace neutrino::assets {
  class resource_name {
    public:
      template <std::size_t N>
      inline
      explicit constexpr resource_name(const char(& value)[N])
      : m_name (value),
        m_id (utils::id(value))
      {
      }

      [[nodiscard]] unsigned int id () const noexcept {
        return m_id;
      }

      [[nodiscard]] std::string name() const noexcept {
        return {m_name.begin(), m_name.end()};
      }

      bool operator==(const resource_name &other) const
      {
        return (m_id == other.m_id);
      }

      bool operator < (const resource_name &other) const
      {
        return (m_name < other.m_name);
      }
    private:
      std::string_view m_name;
      unsigned int m_id;
  };
}

namespace std {

  template <>
  struct hash<neutrino::assets::resource_name>
  {
    std::size_t operator()(const neutrino::assets::resource_name& k) const
    {
      return k.id();
    }
  };

}


#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_NAME_HH
