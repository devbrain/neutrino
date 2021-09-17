//
// Created by igor on 21/07/2021.
//

#include <iostream>
#include <pugixml.hpp>
#include <neutrino/utils/exception.hh>
#include "xml_reader.hh"

namespace neutrino::tiled::tmx {
  struct xml_reader::impl {
    explicit impl (pugi::xml_node elt)
        : m_node (elt), m_doc (nullptr) {
    }

    pugi::xml_node m_node;
    std::unique_ptr<pugi::xml_document> m_doc;
  };

  xml_reader xml_reader::load (const char* xml, std::size_t size, const char* root_elt_name) {
    std::unique_ptr<pugi::xml_document> doc = std::make_unique<pugi::xml_document> ();
    pugi::xml_parse_result result = doc->load_buffer (xml, size);
    if (!result) {
      RAISE_EX("Failed to load tmx :", result.description ());
    }
    auto root = doc->child (root_elt_name);
    if (!root) {
      RAISE_EX ("entry node <", root_elt_name, "> is missing");
    }
    xml_reader res{root};
    res.m_pimpl->m_doc = std::move (doc);
    return res;
  }

  xml_reader::xml_reader (const pugi::xml_node& node)
      : m_pimpl (spimpl::make_unique_impl<impl> (node)) {
  }

  xml_reader::~xml_reader () = default;

  bool xml_reader::has_child (const char* name) const {
    return m_pimpl->m_node.child (name);
  }

  std::optional<std::string> xml_reader::get_attribute_value (const char* name) const {
    const auto a = m_pimpl->m_node.attribute (name);
    if (a) {
      return a.as_string ();
    }
    return std::nullopt;
  }

  std::string xml_reader::get_text () const {
    const char* text = m_pimpl->m_node.text ().get ();
    return text ? text : "";
  }

  void xml_reader::parse_each_element (visitor_t func) const {
    auto child = m_pimpl->m_node.first_child ();

    while (child) {
      func (xml_reader (child));
      child = child.next_sibling ();
    }
  }

  void xml_reader::parse_many_elements (const char* name, visitor_t func) const {
    auto child = m_pimpl->m_node.child (name);
    while (child) {
      func (xml_reader (child));
      child = child.next_sibling (name);
    }
  }

  void xml_reader::parse_one_element (const char* name, visitor_t func) const {
    auto child = m_pimpl->m_node.child (name);

    if (!child) {
      return;
    }

    func (xml_reader (child));
    child = child.next_sibling (name);

    if (child != nullptr) {
      std::clog << "Multiple chidren where a single child was expected for element: " << name << '\n';
    }
  }
}
