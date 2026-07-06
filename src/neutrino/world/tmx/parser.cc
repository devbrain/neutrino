//
// Created by igor on 05/07/2026.
//

#include "world/tmx/parser.hh"

#include "world/tmx/node.hh"
#include "world/tmx/world_parser.hh"

#include <string>

namespace neutrino::world_tmx {
    namespace {
        [[nodiscard]] node_view map_root_from_json(const json& document) {
            const json* root = &document;
            if (document.is_object()) {
                if (const auto it = document.find("map"); it != document.end()) {
                    root = &*it;
                }
            }
            return {node_kind::json, {}, root};
        }
    }

    world parse_document(std::string_view document, const tmx_path_resolver& resolver) {
        const auto clean = utils::trim_document(document);
        if (clean.empty()) {
            fail("document is empty");
        }

        if (clean.front() == '<') {
            pugi::xml_document doc;
            const auto status = doc.load_buffer(clean.data(), clean.size());
            if (!status) {
                fail(std::string("failed to parse XML: ") + status.description());
            }
            const auto root = doc.child("map");
            if (!root) {
                fail("XML document has no <map> root");
            }
            return parse_world(node_view{node_kind::xml, root, nullptr}, resolver);
        }

        if (clean.front() == '{' || clean.front() == '[') {
            try {
                json doc = json::parse(clean);
                return parse_world(map_root_from_json(doc), resolver);
            } catch (const json::exception& e) {
                fail(std::string("failed to parse JSON: ") + e.what());
            }
        }

        fail("unknown world document type");
    }
}
