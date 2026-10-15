//
// Created by igor on 14/10/2026.
//

#include "assets.hh"

#include <cctype>
#include <algorithm>
#include <neutrino/audio/audio.hh>

namespace luxor::rc {

    assets& assets::instance() {
        static assets s_instance;
        return s_instance;
    }

    std::string assets::normalize_key(std::string_view path) {
        std::string key;
        key.reserve(path.size());
        for (char c : path) {
            if (c == '\\') {
                key.push_back('/');
            } else {
                key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
        }
        size_t start = 0;
        while (start < key.size() && key[start] == '/') {
            ++start;
        }
        if (start > 0) {
            return key.substr(start);
        }
        return key;
    }

    bool assets::init(const std::filesystem::path& game_path, std::string_view locale) {
        clear_cache();
        m_vfs.clear();

        std::error_code ec;
        if (!std::filesystem::exists(game_path, ec)) {
            return false;
        }

        bool mounted = false;

        // 1. Luxor 1: data/data.mjz (+ data/<locale>.mjz)
        if (std::filesystem::is_regular_file(game_path / "data" / "data.mjz", ec)) {
            m_vfs.mount("", game_path / "data" / "data.mjz");
            mounted = true;

            if (!locale.empty() && std::filesystem::is_regular_file(game_path / "data" / (std::string(locale) + ".mjz"), ec)) {
                m_vfs.mount("", game_path / "data" / (std::string(locale) + ".mjz"));
            } else if (std::filesystem::is_regular_file(game_path / "data" / "english.mjz", ec)) {
                m_vfs.mount("", game_path / "data" / "english.mjz");
            }
        } else if (std::filesystem::is_regular_file(game_path / "data.mjz", ec)) {
            // Luxor 2: data.mjz
            m_vfs.mount("", game_path / "data.mjz");
            mounted = true;
        } else if (std::filesystem::is_regular_file(game_path / "data.nhp", ec)) {
            // Luxor 3 / Luxor Adventures: data.nhp
            m_vfs.mount("", game_path / "data.nhp");
            mounted = true;
        } else if (std::filesystem::is_regular_file(game_path / "data.npk", ec)) {
            // Luxor: Quest for the Afterlife: data.npk
            m_vfs.mount("", game_path / "data.npk");
            mounted = true;
        } else if (std::filesystem::is_regular_file(game_path / "assets" / "assets.pfp", ec)) {
            // Luxor: 5th Passage: assets/assets.pfp
            m_vfs.mount("", game_path / "assets" / "assets.pfp");
            mounted = true;
        } else if (std::filesystem::is_regular_file(game_path / "resources.sfs", ec)) {
            // Luxor Adventures: resources.sfs
            m_vfs.mount("", game_path / "resources.sfs");
            mounted = true;
        } else if (std::filesystem::is_directory(game_path, ec)) {
            // Unpacked directory / dump mount
            m_vfs.mount("", game_path);
            mounted = true;
        }

        if (!locale.empty()) {
            m_vfs.set_locale(locale);
        }

        return mounted;
    }

    void assets::init(vfs fs) {
        clear_cache();
        m_vfs = std::move(fs);
    }

    void assets::set_vfs(vfs fs) {
        clear_cache();
        m_vfs = std::move(fs);
    }

    void assets::mount(std::string_view mount_point, const std::filesystem::path& physical_path) {
        m_vfs.mount(mount_point, physical_path);
    }

    void assets::mount(std::string_view mount_point, std::unique_ptr<archive> ar) {
        m_vfs.mount(mount_point, std::move(ar));
    }

    void assets::mount(std::string_view mount_point, std::shared_ptr<archive> ar) {
        m_vfs.mount(mount_point, std::move(ar));
    }

    bool assets::unmount(std::string_view mount_point) {
        return m_vfs.unmount(mount_point);
    }

    void assets::set_locale(std::string_view locale) {
        m_vfs.set_locale(locale);
    }

    std::string_view assets::locale() const noexcept {
        return m_vfs.locale();
    }

    bool assets::exists(std::string_view path) const {
        return m_vfs.exists(path);
    }

    std::vector<uint8_t> assets::read(std::string_view path) const {
        return m_vfs.read(path);
    }

    std::unique_ptr<std::istream> assets::open(std::string_view path) const {
        return m_vfs.open(path);
    }

    expected<neutrino::sprite_def, std::string> assets::get_sprite(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_sprites.find(key);
        if (it != m_sprites.end()) {
            return it->second;
        }

        auto loaded = load_sprite(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_sprites.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const neutrino::sprite_def* assets::find_sprite(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_sprites.find(key);
        if (it != m_sprites.end()) {
            return &it->second;
        }

        auto loaded = load_sprite(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_sprites.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    bool assets::has_sprite(std::string_view path) const {
        std::string key = normalize_key(path);
        return m_sprites.find(key) != m_sprites.end();
    }

    expected<neutrino::sprite_set_handle, std::string> assets::acquire_sprite(std::string_view path) {
        auto spr = get_sprite(path);
        if (!spr) {
            return unexpected(spr.error());
        }
        try {
            return neutrino::acquire_sprite(*spr);
        } catch (const std::exception& e) {
            return unexpected(std::string("Failed to acquire sprite: ") + e.what());
        }
    }

    std::shared_ptr<const sdlpp::surface> assets::get_image(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_images.find(key);
        if (it != m_images.end()) {
            return it->second;
        }

        auto surf = load_image(m_vfs, std::string(path));
        if (!surf) {
            return nullptr;
        }

        auto shared = std::make_shared<sdlpp::surface>(std::move(surf));
        m_images.emplace(std::move(key), shared);
        return shared;
    }

    const sdlpp::surface* assets::find_image(std::string_view path) {
        auto img = get_image(path);
        return img ? img.get() : nullptr;
    }

    bool assets::has_image(std::string_view path) const {
        std::string key = normalize_key(path);
        return m_images.find(key) != m_images.end();
    }

    expected<font_file, std::string> assets::get_font(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_fonts.find(key);
        if (it != m_fonts.end()) {
            return it->second;
        }

        auto loaded = load_font(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_fonts.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const font_file* assets::find_font(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_fonts.find(key);
        if (it != m_fonts.end()) {
            return &it->second;
        }

        auto loaded = load_font(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_fonts.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    bool assets::has_font(std::string_view path) const {
        std::string key = normalize_key(path);
        return m_fonts.find(key) != m_fonts.end();
    }

    expected<depth_table, std::string> assets::get_depths(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_depths.find(key);
        if (it != m_depths.end()) {
            return it->second;
        }

        auto loaded = load_depths(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_depths.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const depth_table* assets::find_depths(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_depths.find(key);
        if (it != m_depths.end()) {
            return &it->second;
        }

        auto loaded = load_depths(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_depths.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    expected<string_list, std::string> assets::get_string_list(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_string_lists.find(key);
        if (it != m_string_lists.end()) {
            return it->second;
        }

        auto loaded = load_string_list(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_string_lists.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const string_list* assets::find_string_list(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_string_lists.find(key);
        if (it != m_string_lists.end()) {
            return &it->second;
        }

        auto loaded = load_string_list(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_string_lists.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    expected<config_document, std::string> assets::get_config(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_configs.find(key);
        if (it != m_configs.end()) {
            return it->second;
        }

        auto loaded = load_config(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_configs.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const config_document* assets::find_config(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_configs.find(key);
        if (it != m_configs.end()) {
            return &it->second;
        }

        auto loaded = load_config(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_configs.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    expected<level_file, std::string> assets::get_level(std::string_view path) {
        return get_config(path);
    }

    const level_file* assets::find_level(std::string_view path) {
        return find_config(path);
    }

    expected<config_document, std::string> assets::get_psys(std::string_view path) {
        return get_config(path);
    }

    const config_document* assets::find_psys(std::string_view path) {
        return find_config(path);
    }

    expected<tinyobj::ObjReader, std::string> assets::get_obj(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_objs.find(key);
        if (it != m_objs.end()) {
            return it->second;
        }

        auto loaded = load_obj(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_objs.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const tinyobj::ObjReader* assets::find_obj(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_objs.find(key);
        if (it != m_objs.end()) {
            return &it->second;
        }

        auto loaded = load_obj(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_objs.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    expected<sl3_file, std::string> assets::get_sl3(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_sl3s.find(key);
        if (it != m_sl3s.end()) {
            return it->second;
        }

        auto loaded = load_sl3(m_vfs, std::string(path));
        if (!loaded) {
            return unexpected(loaded.error());
        }

        auto [inserted, _] = m_sl3s.emplace(std::move(key), std::move(*loaded));
        return inserted->second;
    }

    const sl3_file* assets::find_sl3(std::string_view path) {
        std::string key = normalize_key(path);
        auto it = m_sl3s.find(key);
        if (it != m_sl3s.end()) {
            return &it->second;
        }

        auto loaded = load_sl3(m_vfs, std::string(path));
        if (!loaded) {
            return nullptr;
        }

        auto [inserted, _] = m_sl3s.emplace(std::move(key), std::move(*loaded));
        return &inserted->second;
    }

    bool assets::has_sl3(std::string_view path) const {
        std::string key = normalize_key(path);
        return m_sl3s.find(key) != m_sl3s.end();
    }

    neutrino::sound_effect assets::load_sound(std::string_view path) {
        auto is = m_vfs.open(path);
        if (!is) {
            return {};
        }
        try {
            return neutrino::load_sfx(*is);
        } catch (...) {
            return {};
        }
    }

    void assets::play_sound(std::string_view path, float volume) {
        std::string key = normalize_key(path);
        auto it = m_active_sounds.find(key);
        if (it == m_active_sounds.end()) {
            auto sfx = load_sound(path);
            auto [ins, _] = m_active_sounds.emplace(std::move(key), std::move(sfx));
            it = ins;
        }
        it->second.play(volume);
    }

    void assets::play_music(std::string_view path, bool loop) {
        auto is = m_vfs.open(path);
        if (!is) {
            return;
        }
        try {
            neutrino::play_music(*is, loop);
        } catch (...) {
        }
    }

    void assets::stop_music() {
        try {
            neutrino::stop_music();
        } catch (...) {
        }
    }

    bool assets::play_cue(std::string_view cue_name, std::string_view sl3_path) {
        auto sl3 = get_sl3(sl3_path);
        if (!sl3) {
            return false;
        }
        if (const auto* sample = sl3->find_sample(cue_name)) {
            play_sound(sample->file, sample->volume);
            return true;
        }
        if (const auto* stream = sl3->find_stream(cue_name)) {
            play_music(stream->file, stream->loop);
            return true;
        }
        return false;
    }

    bool assets::play_playlist(std::string_view playlist_name, std::string_view sl3_path) {
        auto sl3 = get_sl3(sl3_path);
        if (!sl3) {
            return false;
        }
        const auto* pl = sl3->find_playlist(playlist_name);
        if (!pl || pl->tracks.empty()) {
            return false;
        }
        const auto& track_name = pl->tracks.front();
        if (const auto* stream = sl3->find_stream(track_name)) {
            play_music(stream->file, stream->loop);
            return true;
        }
        if (const auto* sample = sl3->find_sample(track_name)) {
            play_sound(sample->file, sample->volume);
            return true;
        }
        play_music(track_name, true);
        return true;
    }

    void assets::clear_cache() {
        m_sprites.clear();
        m_images.clear();
        m_fonts.clear();
        m_depths.clear();
        m_string_lists.clear();
        m_configs.clear();
        m_objs.clear();
        m_sl3s.clear();
        m_active_sounds.clear();
    }

    void assets::reset() {
        clear_cache();
        m_vfs.clear();
    }

    size_t assets::sprite_cache_size() const noexcept { return m_sprites.size(); }
    size_t assets::image_cache_size() const noexcept { return m_images.size(); }
    size_t assets::font_cache_size() const noexcept { return m_fonts.size(); }
    size_t assets::depth_cache_size() const noexcept { return m_depths.size(); }
    size_t assets::string_list_cache_size() const noexcept { return m_string_lists.size(); }
    size_t assets::config_cache_size() const noexcept { return m_configs.size(); }
    size_t assets::obj_cache_size() const noexcept { return m_objs.size(); }
    size_t assets::sl3_cache_size() const noexcept { return m_sl3s.size(); }

} // namespace luxor::rc
