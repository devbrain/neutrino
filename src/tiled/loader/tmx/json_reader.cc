//
// Created by igor on 06/08/2021.
//

#include <sstream>
#include "json_reader.hh"
#define INCLUDE_NLOHMANN_JSON_FWD_HPP_
#include <nlohmann/json.hpp>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    struct json_reader::impl {
        explicit impl(nlohmann::json node)
        : m_node(std::move(node)) {}

        nlohmann::json m_node;
    };

    void json_reader::assert_type(const char* type, const reader& elt) {
        if (dynamic_cast<const json_reader*>(&elt)) {
            if (elt.get_string_attribute("type") != type) {
                RAISE_EX("map type expected");
            }
        }
    }

    json_reader json_reader::load(const char* json, std::size_t size, const char* root_elt_name) {
        auto obj = nlohmann::json::parse(json, json+size);
        if (root_elt_name) {
            auto itr = obj.find(root_elt_name);
            if (itr == obj.end()) {
                RAISE_EX("Can not find element <", root_elt_name, ">");
            }
            return json_reader{*itr};
        }
        return json_reader{obj};
    }

    json_reader::json_reader(const nlohmann::json& node)
    : m_pimpl(spimpl::make_unique_impl<impl>(node))
    {

    }

    json_reader::~json_reader() = default;

    void json_reader::parse_each_element(visitor_t func) const
    {
        for (const auto& obj : m_pimpl->m_node) {
            func(json_reader(obj));
        }
    }
    // -------------------------------------------------------------------------------------------
    void json_reader::iterate_data_array(std::function<void(uint32_t  v)> f, const char* name) const {
        auto itr = m_pimpl->m_node.find(name);
        if (itr == m_pimpl->m_node.end()) {
            RAISE_EX("Can not find", name, "array");
        }
        if (!itr->is_array()) {
            RAISE_EX(name, "element is not array");
        }
        for (const auto& x : *itr) {
            f(x.get<uint32_t>());
        }
    }
    // -------------------------------------------------------------------------------------------
    void json_reader::parse_many_elements(const char* name, visitor_t func) const
    {
        auto itr = m_pimpl->m_node.find(name);
        if (itr != m_pimpl->m_node.end()) {
            if (itr->is_object() || itr->is_array()) {
                const auto& child = *itr;
                for (const auto& obj : child) {
                    func(json_reader(obj));
                }
            } else {
                RAISE_EX("json field <", name, "> should be object or array");
            }
        }
    }
    // -------------------------------------------------------------------------------------------
    void json_reader::parse_one_element(const char* name, visitor_t func) const
    {
        auto itr = m_pimpl->m_node.find(name);
        if (itr != m_pimpl->m_node.end()) {
            const auto& child = *itr;
               
                if (child.is_object()) {
                    func(json_reader(child));
                } else {
                    if (child.is_array()) {
                        for (const auto& e : child) {
                            func(json_reader(e));
                        }
                    } else {
                        RAISE_EX("Non iterable element ", name);
                    }
                }

        }
    }
    // -------------------------------------------------------------------------------------------
    bool json_reader::has_element(const char* name) const noexcept {
        return m_pimpl->m_node.find(name) != m_pimpl->m_node.end();
    }
    // -------------------------------------------------------------------------------------------
    bool json_reader::has_child(const char* name) const
    {
        auto itr = m_pimpl->m_node.find(name);
        if (itr != m_pimpl->m_node.end()) {
            return itr->is_object() || itr->is_array();
        }
        return false;
    }
    // -------------------------------------------------------------------------------------------
    void json_reader::parse_each_element_of(const char* name, visitor_t func) const {
        auto itr = m_pimpl->m_node.find(name);
        if (itr != m_pimpl->m_node.end()) {
            if (itr->is_array()) {
                for (const auto& obj : *itr) {
                    func(json_reader(obj));
                }
            }
            else {
                RAISE_EX(name, "is expected to be an array");
            }
        }
    }
    // -------------------------------------------------------------------------------------------
    std::vector<math::point2f> json_reader::parse_points(const char* name) const {
        auto j = m_pimpl->m_node.find(name);
        if (j == m_pimpl->m_node.end()) {
            RAISE_EX("Can not find element", name);
        }
        const auto& node = *j;
        if (!node.is_array()) {
            RAISE_EX("points elements is expected to be an array");
        }
        std::vector<math::point2f> ret;
        for (const auto& obj : node) {
            if (!obj.is_object()) {
                RAISE_EX("points should be an array of objects");
            }
            auto itr = obj.find("x");
            if (itr == obj.end()) {
                RAISE_EX("x attribute is missing for points object");
            }
            const float x = itr->get<float>();
            itr = obj.find("y");
            if (itr == obj.end()) {
                RAISE_EX("y attribute is missing for points object");
            }
            const float y = itr->get<float>();
            ret.emplace_back(x, y);
        }
        return ret;
    }
    // -------------------------------------------------------------------------------------------
    std::optional<std::string> json_reader::get_attribute_value(const char* name) const
    {

        auto itr = m_pimpl->m_node.find(name);
        if (itr != m_pimpl->m_node.end()) {
            if (!itr->is_object() && !itr->is_array()) {
                if (!itr->is_string()) {
                    std::ostringstream os;
                    os << *itr;
                    return os.str();
                } else {
                    return itr->get<std::string>();
                }
            }
        }
        return std::nullopt;
    }
}
