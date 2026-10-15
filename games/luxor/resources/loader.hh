//
// Created by igor on 14/10/2026.
//

#pragma once

#include <string>

#include <luxor/util/expected.hh>
#include <luxor/resources/storage/vfs.hh>
#include <neutrino/video/sprites.hh>
#include <luxor/resources/parser/font_parser.hh>
#include <luxor/resources/parser/depth_parser.hh>
#include <luxor/resources/parser/cfg_parser.hh>
#include <luxor/resources/parser/sl3_parser.hh>
#include <tiny_obj_loader.h>

namespace luxor::rc {
    sdlpp::surface load_image(const vfs& fs, const std::string& path);
    expected<neutrino::sprite_def, std::string> load_sprite(const vfs& fs, const std::string& path);
    expected<font_file, std::string> load_font(const vfs& fs, const std::string& path);
    expected<depth_table, std::string> load_depths(const vfs& fs, const std::string& path);
    expected<string_list, std::string> load_string_list(const vfs& fs, const std::string& path);
    expected<config_document, std::string> load_config(const vfs& fs, const std::string& path);
    expected<level_file, std::string> load_level(const vfs& fs, const std::string& path);
    expected<config_document, std::string> load_psys(const vfs& fs, const std::string& path);
    expected<tinyobj::ObjReader, std::string> load_obj(const vfs& fs, const std::string& path);
    expected<sl3_file, std::string> load_sl3(const vfs& fs, const std::string& path);
}
