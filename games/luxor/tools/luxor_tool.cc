#include <iostream>
#include <string_view>
#include <filesystem>

#include <luxor/resources/storage/archive.hh>
#include <luxor/resources/storage/game.hh>
#include <luxor/resources/storage/vfs.hh>
#include <luxor/resources/parser/cfg_parser.hh>
#include <luxor/resources/parser/spr_parser.hh>
#include <luxor/resources/parser/font_parser.hh>
#include <luxor/resources/parser/depth_parser.hh>
#include <luxor/resources/parser/sl3_parser.hh>

namespace {
    void print_usage(const char* prog) {
        std::cout << "Luxor Asset Tool\n"
                  << "Usage: " << prog << " <command> [arguments...]\n\n"
                  << "Commands:\n"
                  << "  probe <path>                  Probe game directory or archive\n"
                  << "  list  <archive_file>          List contents of an archive (.mjz, .npk, .nhp, .sfs, .pfp)\n"
                  << "  dump  <archive_file> <out_dir> Extract archive contents to directory\n"
                  << "  info  <asset_file>            Display information about .lvl, .ini, .psys, .spr, .font, or .txt file\n"
                  << "  json  <asset_file>            Dump parsed entity as JSON to stdout\n";
    }

    int cmd_probe(std::string_view path_str) {
        std::filesystem::path p(path_str);
        auto k = luxor::rc::game::probe(p);
        if (k != luxor::rc::game_kind::unknown) {
            std::cout << "Detected game: " << to_display_name(k) << " (" << to_string(k) << ")\n";
            return 0;
        }

        if (luxor::rc::archive::probe(p)) {
            std::cout << "Detected valid Luxor archive: " << p.string() << "\n";
            return 0;
        }

        std::cout << "Unknown or unrecognized game/archive path: " << p.string() << "\n";
        return 1;
    }

    int cmd_list(const std::filesystem::path& archive_path) {
        auto a = luxor::rc::archive::load(archive_path);
        if (!a) {
            std::cerr << "Failed to open archive: " << archive_path.string() << "\n";
            return 1;
        }

        auto files = a->list_files();
        std::cout << "Archive: " << archive_path.string() << " (" << files.size() << " entries)\n";
        for (const auto& f : files) {
            std::cout << (f.is_directory ? " [DIR]  " : " [FILE] ")
                      << f.path << " (" << f.size << " bytes)\n";
        }
        return 0;
    }

    int cmd_dump(const std::filesystem::path& archive_path, const std::filesystem::path& out_dir) {
        auto a = luxor::rc::archive::load(archive_path);
        if (!a) {
            std::cerr << "Failed to open archive: " << archive_path.string() << "\n";
            return 1;
        }

        std::cout << "Extracting " << archive_path.string() << " to " << out_dir.string() << "...\n";
        luxor::rc::dump(a, out_dir);
        std::cout << "Done!\n";
        return 0;
    }

    int cmd_info(const std::filesystem::path& file_path) {
        std::string ext = file_path.extension().string();
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (ext == ".lvl" || ext == ".ini" || ext == ".psys" || ext == ".ui") {
            auto cfg = luxor::rc::parse_config_file(file_path);
            if (!cfg) {
                std::cerr << "Failed to parse config file: " << cfg.error() << "\n";
                return 1;
            }
            std::cout << "Config document: " << file_path.string() << "\n"
                      << "  Root properties: " << cfg->properties.size() << "\n"
                      << "  Sections: " << cfg->sections.size() << "\n";
            for (const auto& s : cfg->sections) {
                std::cout << "    [" << (s.type.empty() ? "" : s.type + " ") << s.name
                          << "] (" << s.properties.size() << " properties)\n";
            }
            return 0;
        }

        if (ext == ".sl3") {
            auto sl3 = luxor::rc::parse_sl3_file(file_path);
            if (!sl3) {
                std::cerr << "Failed to parse SL3 file: " << sl3.error() << "\n";
                return 1;
            }
            std::cout << "Sound/Music script (SL3): " << file_path.string() << "\n"
                      << "  Samples:   " << sl3->samples.size() << "\n"
                      << "  Streams:   " << sl3->streams.size() << "\n"
                      << "  Playlists: " << sl3->playlists.size() << "\n";
            return 0;
        }

        if (ext == ".spr") {
            auto spr = luxor::rc::parse_spr_file(file_path);
            if (!spr) {
                std::cerr << "Failed to parse sprite file: " << spr.error() << "\n";
                return 1;
            }
            std::cout << "Sprite: " << file_path.string() << "\n"
                      << "  Diffuse: " << spr->diffuse_texture << "\n"
                      << "  Alpha:   " << (spr->has_alpha ? spr->alpha_texture : "none") << "\n"
                      << "  Frame size: " << spr->grid.cell_w << "x" << spr->grid.cell_h << "\n"
                      << "  Animations: " << spr->anims.size() << "\n"
                      << "  Total frames: " << spr->total_frames() << "\n";
            return 0;
        }

        if (ext == ".font") {
            auto font = luxor::rc::parse_font_file(file_path);
            if (!font) {
                std::cerr << "Failed to parse font file: " << font.error() << "\n";
                return 1;
            }
            std::cout << "Font: " << file_path.string() << "\n"
                      << "  Diffuse: " << font->diffuse_texture << "\n"
                      << "  Alpha:   " << (font->has_alpha ? font->alpha_texture : "none") << "\n"
                      << "  Char height: " << font->char_height << "\n"
                      << "  Glyph count: " << font->glyph_count << "\n";
            return 0;
        }

        if (ext == ".txt") {
            auto dt = luxor::rc::parse_depths_file(file_path);
            if (dt) {
                std::cout << "Depth table: " << file_path.string() << "\n"
                          << "  Total layers: " << dt->size() << "\n";
                for (size_t i = 0; i < dt->size(); ++i) {
                    std::cout << "    [" << i << "] " << (*dt)[i] << "\n";
                }
                return 0;
            }

            auto sl = luxor::rc::parse_string_list_file(file_path);
            if (sl) {
                std::cout << "String list: " << file_path.string() << "\n"
                          << "  Total items: " << sl->size() << "\n";
                for (size_t i = 0; i < sl->size(); ++i) {
                    std::cout << "    [" << i << "] " << (*sl)[i] << "\n";
                }
                return 0;
            }

            std::cerr << "Failed to parse text list file: " << dt.error() << "\n";
            return 1;
        }

        std::cerr << "Unsupported file format for info: " << ext << "\n";
        return 1;
    }

    int cmd_json(const std::filesystem::path& file_path) {
        std::string ext = file_path.extension().string();
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (ext == ".lvl" || ext == ".ini" || ext == ".psys" || ext == ".ui") {
            auto cfg = luxor::rc::parse_config_file(file_path);
            if (!cfg) {
                std::cerr << "Failed to parse config file: " << cfg.error() << "\n";
                return 1;
            }
            std::cout << cfg->to_json().dump(2) << "\n";
            return 0;
        }

        if (ext == ".sl3") {
            auto sl3 = luxor::rc::parse_sl3_file(file_path);
            if (!sl3) {
                std::cerr << "Failed to parse SL3 file: " << sl3.error() << "\n";
                return 1;
            }
            std::cout << sl3->to_json().dump(2) << "\n";
            return 0;
        }

        if (ext == ".spr") {
            auto spr = luxor::rc::parse_spr_file(file_path);
            if (!spr) {
                std::cerr << "Failed to parse sprite file: " << spr.error() << "\n";
                return 1;
            }
            std::cout << spr->to_json().dump(2) << "\n";
            return 0;
        }

        if (ext == ".font") {
            auto font = luxor::rc::parse_font_file(file_path);
            if (!font) {
                std::cerr << "Failed to parse font file: " << font.error() << "\n";
                return 1;
            }
            std::cout << font->to_json().dump(2) << "\n";
            return 0;
        }

        if (ext == ".txt") {
            auto dt = luxor::rc::parse_depths_file(file_path);
            if (dt) {
                std::cout << dt->to_json().dump(2) << "\n";
                return 0;
            }
            auto sl = luxor::rc::parse_string_list_file(file_path);
            if (sl) {
                std::cout << sl->to_json().dump(2) << "\n";
                return 0;
            }
            std::cerr << "Failed to parse text list file: " << dt.error() << "\n";
            return 1;
        }

        std::cerr << "Unsupported file format for json: " << ext << "\n";
        return 1;
    }
} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    std::string_view cmd = argv[1];
    if (cmd == "probe" && argc >= 3) {
        return cmd_probe(argv[2]);
    }
    if (cmd == "list" && argc >= 3) {
        return cmd_list(argv[2]);
    }
    if (cmd == "dump" && argc >= 4) {
        return cmd_dump(argv[2], argv[3]);
    }
    if (cmd == "info" && argc >= 3) {
        return cmd_info(argv[2]);
    }
    if (cmd == "json" && argc >= 3) {
        return cmd_json(argv[2]);
    }

    print_usage(argv[0]);
    return 1;
}