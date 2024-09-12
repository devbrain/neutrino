//
// Created by igor on 9/10/24.
//

#ifndef NEUTRINO_CONFIG_SERVICE_HH
#define NEUTRINO_CONFIG_SERVICE_HH

#include <filesystem>
#include <string>
#include <iostream>
#include <memory>
#include <tuple>
#include <bsw/s11n/s11n.hh>

namespace neutrino {
    class config_service {
        public:
            static std::string get_executable_name();
            static std::filesystem::path get_executable_dir();
            static std::filesystem::path get_config_root();
            static void set_config_root(const std::filesystem::path& new_path);

            static void set_config_prefix(const std::string& prefix);
            static std::string get_config_prefix();
            static bool has_config_prefix();

            static std::filesystem::path get_path_to_configs();
            static bool is_config_root_exists();
            static bool is_config_root_writable();
            static bool is_config_readable();
            static bool make_application_config_dirs();

            static bool file_exists(const std::string& name);

            template <typename T>
            static std::tuple<T, bool> load(const std::string& name) {
                auto is = get_istream(name);
                if (is->good()) {
                    return {bsw::s11n::deserialize_from_yaml<T>(*is), true};
                }
                T cfg;
                save(cfg, name);
                return {cfg, false};
            }

            template <typename T>
            static void save(const T& cfg, const std::string& name) {
                auto ofs = get_ostream(name);
                bsw::s11n::serialize_to_yaml(cfg, *ofs);
            }

        private:
            static std::unique_ptr<std::istream> get_istream(const std::string& name);
            static std::unique_ptr<std::ostream> get_ostream(const std::string& name);
    };
}

#endif
