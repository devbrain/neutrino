//
// Created by igor on 8/27/24.
//

#include <pugixml.hpp>
#include <fstream>
#include <sstream>
#include <bsw/exception.hh>
#include <sdlpp/video/surface.hh>
#include <neutrino/modules/video/world/world_serializer.hh>
#include <neutrino/modules/video/texture_atlas.hh>
#include <neutrino/modules/video/world/world_model.hh>
#include <bsw/override.hh>

namespace neutrino {
    namespace {
        void create_tsx(const std::filesystem::path& out_dir, texture_id_t tid,
            const std::string& img_file,
            pugi::xml_node& root,
            const std::vector<sdl::rect>& rects, unsigned fgid) {
            std::ostringstream os;
            os << "tileset-" << tid;
            auto area = rects.front().area();
            pugi::xml_document doc;
            pugi::xml_node node = doc.append_child("tileset");
            node.append_attribute("firstgid").set_value(fgid);
            node.append_attribute("name").set_value(os.str().c_str());
            node.append_attribute("tilewidth").set_value(area.w);
            node.append_attribute("tileheight").set_value(area.h);
            node.append_attribute("tilecount").set_value(rects.size());

            for (std::size_t i = 0; i < rects.size(); ++i) {
                auto tl = node.append_child("tile");
                tl.append_attribute("x").set_value(rects[i].x);
                tl.append_attribute("y").set_value(rects[i].y);
                tl.append_attribute("width").set_value(rects[i].w);
                tl.append_attribute("height").set_value(rects[i].h);
                tl.append_attribute("id").set_value(i);
                tl.append_child("image").append_attribute("source").set_value(img_file.c_str());
            }
            os << ".tsx";
            std::ofstream ofs(out_dir / os.str());

            auto ts = root.append_child("tileset");
            ts.append_attribute("firstgid").set_value(fgid);
            ts.append_attribute("source").set_value(os.str().c_str());

            doc.print(ofs);
        }
    }
    // ----------------------------------------------------------------------------------------------------
    void add_layer(pugi::xml_node& root, const tiled::tiles_layer& tl, int id, const std::map<texture_id_t, unsigned>& fgid_map) {
        pugi::xml_node layer = root.append_child("layer");
        layer.append_attribute("id").set_value(id);
        std::ostringstream os;
        os << "layer-" << id;
        layer.append_attribute("name").set_value(os.str().c_str());
        layer.append_attribute("width").set_value(tl.get_width());
        layer.append_attribute("height").set_value(tl.get_height());
        pugi::xml_node data = layer.append_child("data");
        data.append_attribute("encoding").set_value("csv");
        os.str("<-- DATA --> \n");
        for (tile_coord_t r=0; r<tl.get_height(); ++r) {
            for (tile_coord_t c=0; c<tl.get_width(); ++c) {
                if (c>0) {
                    os <<", ";
                }
                auto value = tl.at(c, r);
                unsigned gid = 0;
                if (value.m_id == EMPTY_TILE_VALUE) {
                    gid = 0;
                } else {
                    auto itr = fgid_map.find(value.m_texture);
                    if (itr == fgid_map.end()) {
                        ENFORCE (itr != fgid_map.end());
                    }
                    gid = itr->second + value.m_id.value_of();
                }
                os << gid;
            }
            if (r < tl.get_height()-1) {
                os << ",";
            }
            os << "\n";
        }
        data.append_child(pugi::node_pcdata).set_value(os.str().c_str());
    }
    // ----------------------------------------------------------------------------------------------------
    std::tuple<tile_coord_t, tile_coord_t, tile_coord_t, tile_coord_t> get_map_size(const tiled::world_model& model) {
        tile_coord_t w = 0;
        tile_coord_t h = 0;
        tile_coord_t tl_w = 0;
        tile_coord_t tl_h = 0;
        for (const auto& layer : model) {
            std::visit(
                bsw::overload(
                    []([[maybe_unused]] const tiled::image_layer& il) {

                    },
                    []([[maybe_unused]] const tiled::color_layer& cl) {
                    },
                    [&](const tiled::tiles_layer& tl) {
                        w = std::max(w, tl.get_width());
                        h = std::max(h, tl.get_height());
                        tl_w = tl.get_tile_width();
                        tl_h = tl.get_tile_height();
                    },
                    []([[maybe_unused]] const tiled::objects_layer& ol) {
                    }
                ),
                layer
            );
        }
        return {w,h, tl_w, tl_h};
    }


    void serialize_tmx(const std::filesystem::path& out_dir,
                       const tiled::world_model& model,
                       const texture_atlas& atlas,
                       const sdl::renderer& renderer) {
        if (exists(out_dir)) {
            if (!is_directory(out_dir)) {
                RAISE_EX("Output ", out_dir.c_str(), " exists and is not directory");
            }
        } else {
            if (!std::filesystem::create_directories(out_dir)) {
                RAISE_EX("Failed to create directory ", out_dir.c_str());
            }
        }

        pugi::xml_document doc;
        pugi::xml_node root = doc.append_child("map");
        auto [w,h, tl_w, tl_h] = get_map_size(model);

        root.append_attribute("width").set_value(w);
        root.append_attribute("height").set_value(h);
        root.append_attribute("tilewidth").set_value(tl_w);
        root.append_attribute("tileheight").set_value(tl_h);
        root.append_attribute("orientation").set_value("orthogonal");
        unsigned fgid = 1;
        std::map<texture_id_t, unsigned> fgid_map;
        for (const auto& [tid, tex] : atlas.m_textures) {
            if (tex.m_rects.empty()) {
                continue;
            }
            sdl::surface surface(tex.m_texture.convert_to_surface(renderer));
            std::ostringstream os;
            os << tid << ".bmp";
            auto fname = os.str();

            surface.save_bmp(out_dir / fname);
            create_tsx(out_dir, tid, fname, root, tex.m_rects, fgid);
            fgid_map.emplace(tid, fgid);
            fgid += tex.m_rects.size();
        }
        int id = 1;
        for (const auto& layer : model) {
            std::visit(
                bsw::overload(
                    []([[maybe_unused]] const tiled::image_layer& il) {
                    },
                    []([[maybe_unused]] const tiled::color_layer& cl) {
                    },
                    [&](const tiled::tiles_layer& tl) {
                        add_layer(root, tl, id++, fgid_map);
                    },
                    []([[maybe_unused]] const tiled::objects_layer& ol) {
                    }
                ),
                layer
            );
        }

        std::ofstream ofs(out_dir / "map.tmx");
        doc.print(ofs);
    }
}