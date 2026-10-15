//
// Created by igor on 14/10/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include <luxor/util/expected.hh>
#include <luxor/resources/storage/vfs.hh>
#include <luxor/resources/parser/spr_parser.hh>
#include <luxor/resources/parser/font_parser.hh>
#include <luxor/resources/parser/depth_parser.hh>
#include <luxor/resources/parser/cfg_parser.hh>
#include <luxor/resources/parser/sl3_parser.hh>
#include <luxor/resources/loader.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/audio/sound_effect.hh>
#include <tiny_obj_loader.h>

namespace luxor::rc {

    class assets {
    public:
        // Singleton access
        static assets& instance();
        static assets& get() { return instance(); }

        assets(const assets&) = delete;
        assets& operator=(const assets&) = delete;
        assets(assets&&) = delete;
        assets& operator=(assets&&) = delete;

        // Lifecycle & VFS initialization
        bool init(const std::filesystem::path& game_path, std::string_view locale = "english");
        void init(vfs fs);
        void set_vfs(vfs fs);

        [[nodiscard]] vfs& get_vfs() noexcept { return m_vfs; }
        [[nodiscard]] const vfs& get_vfs() const noexcept { return m_vfs; }

        // VFS forwarding helpers
        void mount(std::string_view mount_point, const std::filesystem::path& physical_path);
        void mount(std::string_view mount_point, std::unique_ptr<archive> ar);
        void mount(std::string_view mount_point, std::shared_ptr<archive> ar);
        bool unmount(std::string_view mount_point);
        void set_locale(std::string_view locale);
        [[nodiscard]] std::string_view locale() const noexcept;
        [[nodiscard]] bool exists(std::string_view path) const;
        [[nodiscard]] std::vector<uint8_t> read(std::string_view path) const;
        [[nodiscard]] std::unique_ptr<std::istream> open(std::string_view path) const;

        // Sprites (cached neutrino::sprite_def)
        [[nodiscard]] expected<neutrino::sprite_def, std::string> get_sprite(std::string_view path);
        [[nodiscard]] const neutrino::sprite_def* find_sprite(std::string_view path);
        [[nodiscard]] bool has_sprite(std::string_view path) const;

        // Sprite Set Handle (via active neutrino::get_sprite_cache())
        [[nodiscard]] expected<neutrino::sprite_set_handle, std::string> acquire_sprite(std::string_view path);

        // Images / Surfaces
        [[nodiscard]] std::shared_ptr<const sdlpp::surface> get_image(std::string_view path);
        [[nodiscard]] const sdlpp::surface* find_image(std::string_view path);
        [[nodiscard]] bool has_image(std::string_view path) const;

        // Fonts
        [[nodiscard]] expected<font_file, std::string> get_font(std::string_view path);
        [[nodiscard]] const font_file* find_font(std::string_view path);
        [[nodiscard]] bool has_font(std::string_view path) const;

        // Depths Table
        [[nodiscard]] expected<depth_table, std::string> get_depths(std::string_view path = "data/uiscript/depths.txt");
        [[nodiscard]] const depth_table* find_depths(std::string_view path = "data/uiscript/depths.txt");

        // String Lists (e.g. stage_names.txt)
        [[nodiscard]] expected<string_list, std::string> get_string_list(std::string_view path);
        [[nodiscard]] const string_list* find_string_list(std::string_view path);

        // Configs, Levels, Psys
        [[nodiscard]] expected<config_document, std::string> get_config(std::string_view path);
        [[nodiscard]] const config_document* find_config(std::string_view path);

        [[nodiscard]] expected<level_file, std::string> get_level(std::string_view path);
        [[nodiscard]] const level_file* find_level(std::string_view path);

        [[nodiscard]] expected<config_document, std::string> get_psys(std::string_view path);
        [[nodiscard]] const config_document* find_psys(std::string_view path);

        // 3D Paths / OBJ Meshes
        [[nodiscard]] expected<tinyobj::ObjReader, std::string> get_obj(std::string_view path);
        [[nodiscard]] const tinyobj::ObjReader* find_obj(std::string_view path);

        // Sound & Music Scripts (SL3)
        [[nodiscard]] expected<sl3_file, std::string> get_sl3(std::string_view path);
        [[nodiscard]] const sl3_file* find_sl3(std::string_view path);
        [[nodiscard]] bool has_sl3(std::string_view path) const;

        // Audio
        [[nodiscard]] neutrino::sound_effect load_sound(std::string_view path);
        void play_sound(std::string_view path, float volume = 1.0f);
        void play_music(std::string_view path, bool loop = true);
        void stop_music();

        // SL3 Audio Cues and Playlists
        bool play_cue(std::string_view cue_name, std::string_view sl3_path = "data/sound/sounds.sl3");
        bool play_playlist(std::string_view playlist_name, std::string_view sl3_path = "data/music/music.sl3");

        // Cache Management
        void clear_cache();
        void reset();

        [[nodiscard]] size_t sprite_cache_size() const noexcept;
        [[nodiscard]] size_t image_cache_size() const noexcept;
        [[nodiscard]] size_t font_cache_size() const noexcept;
        [[nodiscard]] size_t depth_cache_size() const noexcept;
        [[nodiscard]] size_t string_list_cache_size() const noexcept;
        [[nodiscard]] size_t config_cache_size() const noexcept;
        [[nodiscard]] size_t obj_cache_size() const noexcept;
        [[nodiscard]] size_t sl3_cache_size() const noexcept;

    private:
        assets() = default;
        ~assets() = default;

        static std::string normalize_key(std::string_view path);

        vfs m_vfs;
        std::unordered_map<std::string, neutrino::sprite_def> m_sprites;
        std::unordered_map<std::string, std::shared_ptr<sdlpp::surface>> m_images;
        std::unordered_map<std::string, font_file> m_fonts;
        std::unordered_map<std::string, depth_table> m_depths;
        std::unordered_map<std::string, string_list> m_string_lists;
        std::unordered_map<std::string, config_document> m_configs;
        std::unordered_map<std::string, tinyobj::ObjReader> m_objs;
        std::unordered_map<std::string, sl3_file> m_sl3s;
        std::unordered_map<std::string, neutrino::sound_effect> m_active_sounds;
    };

} // namespace luxor::rc

namespace luxor {
    using rc::assets;
}