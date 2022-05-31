//
// Created by igor on 30/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_ID_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_ID_HH

#include <string>
#include <iosfwd>
#include "string_id.hpp"

namespace neutrino::assets {
  class resource_id {
    public:
      using hash_t = foonathan::string_id::hash_type;
    public:
      resource_id();
      explicit resource_id(const std::string& name);
      explicit resource_id(const char* name);

      template <std::size_t N>
      inline
      explicit constexpr resource_id(const char(& value)[N])
          : m_id (value, get_database())
      {
      }

      resource_id(const resource_id&) = default;
      resource_id& operator = (const resource_id&) = default;

      [[nodiscard]] hash_t hash() const noexcept;
      [[nodiscard]] std::string name() const noexcept;

      [[nodiscard]] bool empty() const noexcept;

      explicit operator bool () const {
        return !empty();
      }

      friend bool operator==(const resource_id& a, const resource_id& b) noexcept
      {
        return a.m_id == b.m_id;
      }

      friend bool operator!=(const resource_id& a, const resource_id& b) noexcept
      {
        return !(a == b);
      }
    private:
      static foonathan::string_id::basic_database& get_database();
    private:
      foonathan::string_id::string_id m_id;
  };

  std::ostream& operator << (std::ostream& os, const resource_id& rid);
}

namespace std
{
  template <>
  struct hash<neutrino::assets::resource_id>
  {
    using argument_type =  neutrino::assets::resource_id;
    using result_type = std::size_t;

    result_type operator()(const argument_type &arg) const noexcept
    {
      return static_cast<result_type>(arg.hash());
    }
  };
} // namspace std

#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_RESOURCE_ID_HH
