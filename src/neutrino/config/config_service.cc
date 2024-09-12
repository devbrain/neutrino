//
// Created by igor on 9/10/24.
//
#if defined(_WIN32)
#include <io.h>
#define W_OK 02
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif
#include <fstream>
#include <neutrino/config/config_service.hh>
#include <bsw/singleton.hh>
#include <bsw/config_path.hh>
#include <bsw/whereami.hh>

namespace neutrino {

    namespace detail {
        struct cfg_info_impl {
            cfg_info_impl() {
                config_root = bsw::get_config_path();
                auto exe_path = bsw::get_executable_path();
                exe_name = exe_path.filename().replace_extension("").string();
                exe_dir = exe_path.parent_path();
            }
            std::string exe_name;
            std::filesystem::path exe_dir;
            std::filesystem::path config_root;
            std::string app_name;
        };
    }

    using cfg_info = bsw::singleton<detail::cfg_info_impl>;

    std::string config_service::get_executable_name() {
        return cfg_info::instance().exe_name;
    }

    std::filesystem::path config_service::get_executable_dir() {
        return cfg_info::instance().exe_dir;
    }

    std::filesystem::path config_service::get_config_root() {
        return cfg_info::instance().config_root;
    }

    void config_service::set_config_root(const std::filesystem::path& new_path) {
        cfg_info::instance().config_root = new_path;
    }

    void config_service::set_config_prefix(const std::string& prefix) {
        cfg_info::instance().app_name = prefix;
    }

    std::string config_service::get_config_prefix() {
        return cfg_info::instance().app_name;
    }

    bool config_service::has_config_prefix() {
        return !get_config_prefix().empty();
    }

    std::filesystem::path config_service::get_path_to_configs() {
        return get_config_root() / cfg_info::instance().app_name;
    }

    bool config_service::is_config_root_exists() {
        auto target = get_config_root();
        return std::filesystem::exists(target) && std::filesystem::is_directory(target);
    }

    bool config_service::is_config_root_writable() {
        return F_OK == access(get_config_root().u8string().c_str(), W_OK);
    }

    bool config_service::is_config_readable() {
        return F_OK == access(get_path_to_configs().u8string().c_str(), W_OK);
    }

    bool config_service::make_application_config_dirs() {
        if (cfg_info::instance().app_name.empty()) {
            return false;
        }
        auto target = get_path_to_configs();
        if (std::filesystem::exists(target)) {
            return true;
        }
        return std::filesystem::create_directories(get_path_to_configs());
    }

    bool config_service::file_exists(const std::string& name) {
        auto iname = get_path_to_configs() / name;
        return std::filesystem::exists(iname);
    }

    std::unique_ptr<std::istream> config_service::get_istream(const std::string& name) {
        return std::make_unique<std::ifstream>(get_path_to_configs() / name);
    }

    std::unique_ptr<std::ostream> config_service::get_ostream(const std::string& name) {
        auto oname = get_path_to_configs() / name;
        auto ofs = std::make_unique<std::ofstream>(oname);
        if (!ofs->good()) {
            RAISE_EX("Can not create config file ", oname);
        }
        return ofs;
    }
}
