//
// Created by igor on 19/07/2021.
//

#ifndef NEUTRINO_COMPONENT_HH
#define NEUTRINO_COMPONENT_HH

#include <map>
#include <string>
#include <filesystem>
#include <variant>
#include <optional>
#include <strong_type/strong_type.hpp>

#include "reader.hh"
#include "color.hh"

namespace neutrino::tiled::tmx
{
    namespace detail {
        struct object_id_s;
    }
    using object_id = strong::type<int, detail::object_id_s, strong::ordered, strong::equality>;

    using property_t = std::variant<std::string, int64_t, bool, float, colori, std::filesystem::path, object_id>;

    class component
    {
    public:

       static void parse(component& obj, const reader& elt, const component* parent = nullptr);

        template <typename T>
        void add(const std::string& name, T&& v) {
            m_prop.template insert(std::make_pair(name, property_t{std::forward<T>(v)}));
        }

        [[nodiscard]] bool empty() const noexcept ;

        [[nodiscard]] bool contains(const std::string& name) const noexcept ;

        [[nodiscard]] std::optional<property_t> get(const std::string& name) const noexcept ;

    private:
        std::map<std::string, property_t> m_prop;
    };
}


#endif //NEUTRINO_COMPONENT_HH
