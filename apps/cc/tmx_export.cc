//
// Created by igor on 9/15/24.
//

#include <filesystem>
#include <fstream>
#include <map>
#include <functional>

#include <pugixml.hpp>
#include <sstream>
#include "tmx_export.hh"
#include "data_loader/data_directory.hh"
#include "data_loader/crystal_caves/cc_level_loader.hh"
#include "data_loader/crystal_caves/cc_map_cell.hh"
#include "factory.hh"

using tile_props = std::map <int, std::map <std::string, std::string>>;

void create_tsx(const std::filesystem::path& out_dir,
                const std::vector <neutrino::sdl::rect>& rects,
                pugi::xml_node& parent,
                unsigned fgid) {
    std::ostringstream os;
    os << "tileset";
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
        tl.append_child("image").append_attribute("source").set_value("tiles.bmp");
    }
    os << ".tsx";
    std::ofstream ofs(out_dir / os.str());

    auto ts = parent.append_child("tileset");
    ts.append_attribute("firstgid").set_value(fgid);
    ts.append_attribute("source").set_value("tileset.tsx");

    doc.print(ofs);
}

template<typename T, typename Callable>
static void create_layer(pugi::xml_node& root, const char* name, unsigned w, unsigned h,
                         const std::vector <T>& items,
                         Callable value_mapper) {
    static int layer_id = 0;
    pugi::xml_node layer = root.append_child("layer");
    layer.append_attribute("id").set_value(layer_id++);

    layer.append_attribute("name").set_value(name);
    layer.append_attribute("width").set_value(w);
    layer.append_attribute("height").set_value(h);
    pugi::xml_node data = layer.append_child("data");
    data.append_attribute("encoding").set_value("csv");
    std::ostringstream os;

    for (unsigned r = 0; r < h; r++) {
        for (unsigned c = 0; c < w; c++) {
            if (c > 0) {
                os << ", ";
            }
            auto value = items[c + r * w];
            auto gid = value_mapper(value);
            os << gid;
        }
        if (r < h - 1) {
            os << ",";
        }
        os << "\n";
    }
    data.append_child(pugi::node_pcdata).set_value(os.str().c_str());
}



static void save_tmx(const raw_level_map& lvl,
                     const std::tuple <neutrino::sdl::surface, std::vector <neutrino::sdl::rect>>& tiles) {
    std::filesystem::path root("tmx_export");
    std::ostringstream os;
    os << lvl.level_num;
    auto odir = root / os.str();
    if (!std::filesystem::exists(odir)) {
        std::filesystem::create_directories(odir);
    }
    auto [w, h] = lvl.dims;
    pugi::xml_document doc;
    pugi::xml_node root_node = doc.append_child("map");

    root_node.append_attribute("width").set_value(w);
    root_node.append_attribute("height").set_value(h);
    root_node.append_attribute("tilewidth").set_value(16);
    root_node.append_attribute("tileheight").set_value(16);
    root_node.append_attribute("orientation").set_value("orthogonal");
    unsigned fgid = 1;

    const auto& [srf, rects] = tiles;
    auto srf_name = odir / "tiles.bmp";
    srf.save_bmp(srf_name.u8string());
    create_tsx(odir, rects, root_node, fgid);

    // BG
    create_layer(root_node, "bg", w, h, lvl.backround, [](const int& v) {
        if (v == -1) {
            return 0;
        }
        return 1 + v;
    });

    create_layer(root_node, "bg2", w, h, lvl.cells, [](const cell& v) {
        if (v.is_empty() || (v.flags & CC_TILE_FLAG_RENDER_IN_FRONT) != CC_TILE_FLAG_RENDER_IN_FRONT) {
            return 0;
        }
        return 1 + v.tile_name_id;
    });

    create_layer(root_node, "fg", w, h, lvl.cells, [](const cell& v) {
        if (v.is_empty() || (v.flags & CC_TILE_FLAG_RENDER_IN_FRONT) == CC_TILE_FLAG_RENDER_IN_FRONT) {
            return 0;
        }
        return 1 + v.tile_name_id;
    });

    os.str("");
    os << "map-" << lvl.level_num << ".tmx";
    std::ofstream ofs(odir / os.str());
    doc.print(ofs);
}

void tmx_export() {
    data_directory data_dir("/home/igor/proj/ares/games/CAVES/");
    auto tileset = data_dir.load_tileset(data_directory::CC1_TILES);
    auto cc_level_descriptions = data_dir.load_levels(data_directory::CC1_EXE);
    for (const auto& cc_level : cc_level_descriptions) {
        save_tmx(cc_level, tileset);
    }
}
