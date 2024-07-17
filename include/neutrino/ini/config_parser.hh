//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_INI_CONFIG_PARSER_HH
#define NEUTRINO_INI_CONFIG_PARSER_HH

#include <map>
#include <any>
#include <vector>
#include <list>
#include <variant>
#include <sstream>
#include <type_traits>
#include <string_view>
#include <optional>

#include <boost/pfr.hpp>
#include <nlohmann/json.hpp>

#include <neutrino/ini/config.hh>
#include <bsw/exception.hh>
#include <bsw/mp/constexpr_for.hh>
#include <bsw/strings/number_parser.hh>
#include <bsw/mp/type_name/type_name.hpp>
#include <bsw/override.hh>

namespace neutrino::utils {
  namespace detail {
    nlohmann::json load_from_json (const std::string& input);
    nlohmann::json load_from_ini (const std::string& input);
    nlohmann::json load_from_yaml (const std::string& input);

    template <class T>
    struct is_container {
      static bool const value = false;
    };

    template <class T>
    struct is_container<std::vector<T> > {
      using type = T;
      static bool const value = true;
    };

    template <class T>
    struct is_container<std::list<T> > {
      using type = T;
      static bool const value = true;
    };

    template <class T>
    struct get_real_type {
      using type = std::remove_reference_t<T>;
    };

    template <class T>
    struct get_real_type<std::optional<T>> {
      using type = std::remove_reference_t<T>;
    };

  }

  class config_parser {
    public:
      enum kind_t {
        INI,
        JSON,
        YAML
      };

      template <class Config>
      static std::variant<Config, std::vector<Config>> parse(const std::string& input);

      template <class Config>
      static void parse (Config& out, const std::string& input);

      template <class Config>
      static std::variant<Config, std::vector<Config>> parse(const std::string& input, config_parser::kind_t kind);

      template <class Config>
      static void parse (Config& out, const std::string& input, kind_t kind);

    private:
      static std::variant<std::string, nlohmann::json>
          parser_dispatch (const std::string& input, kind_t kind);

      template <class Config>
      static void parse (Config& out, const nlohmann::json& map);

      template <class Config>
      static std::variant<Config, std::vector<Config>> parse (const nlohmann::json& map);

      template <class Config>
      static void parse (std::vector<Config>& out, const nlohmann::json& map);

      static void parse (std::map<std::string, std::any>& out, const nlohmann::json& map);
      static void
      parse (std::vector<std::map<std::string, std::any>>& out, const nlohmann::json& map);

      template <class Config>
      static void set_field (Config& field, const nlohmann::json& cfg_val);

      static std::any parse_field (const nlohmann::json& cfg_val);
      static std::any parse_object (const nlohmann::json& cfg_val);
      static std::any parse_vector (const nlohmann::json& cfg_val);
  };


  template <class Config>
  std::variant<Config, std::vector<Config>> config_parser::parse(const std::string& input) {
    static constexpr kind_t kinds[] {INI, JSON, YAML};
    std::vector<std::string> errors;
    for (auto kind : kinds) {
      const auto res = parser_dispatch (input, kind);
      if (const nlohmann::json* map = std::get_if<nlohmann::json>(&res)) {
        return parse<Config> (map);
      } else {
        errors.push_back (std::get<std::string>(res));
      }
    }
    auto itr = errors.begin();
    std::ostringstream os;
    os << "Failed to parse config: \n";
    for (auto kind : kinds) {
      os << "(" << (kind == YAML ? "YAML" : (kind == JSON) ? "JSON" : "INI") << ") "
         << " Error: " << *itr << "\n";
      itr++;
    }
    RAISE_EX(os.str());
  }

  template <class Config>
  void config_parser::parse (Config& out, const std::string& input) {
    static constexpr kind_t kinds[] {INI, JSON, YAML};
    std::vector<std::string> errors;
    for (auto kind : kinds) {
      const auto res = parser_dispatch (input, kind);
      if (const nlohmann::json* map = std::get_if<nlohmann::json>(&res)) {
        parse (out, map);
        return;
      } else {
        errors.push_back (std::get<std::string>(res));
      }
    }
    auto itr = errors.begin();
    std::ostringstream os;
    os << "Failed to parse config: \n";
    for (auto kind : kinds) {
      os << "(" << (kind == YAML ? "YAML" : (kind == JSON) ? "JSON" : "INI") << ") "
          << " Error: " << *itr << "\n";
      itr++;
    }
    RAISE_EX(os.str());
  }

  template <class Config>
  std::variant<Config, std::vector<Config>> config_parser::parse(const std::string& input, config_parser::kind_t kind) {

    const auto res = parser_dispatch (input, kind);

    return std::visit (
        overload(
            [kind](const std::string& error) {
              RAISE_EX("Failed to parse ",
                       kind == YAML ? "YAML" : (kind == JSON) ? "JSON" : "INI",
                       " Error: ", error
              );
            },
            [](const nlohmann::json& map) {
              return parse<Config> (map);
            }
        ),
        res
    );
  }

  template <class Config>
  void config_parser::parse (Config& out, const std::string& input, config_parser::kind_t kind) {

    const auto res = parser_dispatch (input, kind);

    std::visit (
        overload(
            [kind](const std::string& error) {
              RAISE_EX("Failed to parse ",
                       kind == YAML ? "YAML" : (kind == JSON) ? "JSON" : "INI",
                       " Error: ", error
              );
            },
            [&out](const nlohmann::json& map) {
              parse (out, map);
            }
            ),
        res
        );
  }

  template <class Config>
  void config_parser::parse (std::vector<Config>& out, const nlohmann::json& map) {
    if (map.is_object ()) {
      Config out_obj;
      parse (out_obj, map);
      out.push_back (out_obj);
    }
    else if (map.is_array ()) {
      for (const auto& obj: map) {
        Config out_obj;
        parse (out_obj, obj);
        out.push_back (out_obj);
      }
    }
    else {
      RAISE_EX("top level config should be object or array");
    }
  }

  template <class Config>
  void config_parser::parse (Config& out, const nlohmann::json& map) {
    if (!map.is_object ()) {
      RAISE_EX("Section is expected");
    }
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<Config>> ();

    bsw::mp::constexpr_for<0, fields_count_val, 1> ([&map, &out] (auto index) {
      constexpr auto idx = index.value;
      auto& field = boost::pfr::get<idx> (out);
      using field_t = typename detail::get_real_type<std::remove_reference_t<decltype (field)>>::type;

      if constexpr(std::is_convertible_v<field_t, detail::config_var>) {
        std::string name = field.name.data ();
        if (map.contains (name)) {
          try {
            set_field (boost::pfr::get<idx - 1> (out), map[name]);
          }
          catch (bsw::exception& e) {
            RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse config field ", name);
          }
        }
      }
      else if constexpr(std::is_convertible_v<field_t, neutrino::utils::detail::config_section>) {
        std::string name = field.name.data ();
        if (map.contains (name)) {
          try {
            parse (boost::pfr::get<idx - 1> (out), map[name]);
          }
          catch (bsw::exception& e) {
            RAISE_EX_WITH_CAUSE(std::move (e), "Failed to parse config section ", name);
          }
        }
      }
    });
  }

  template <class Config>
  void config_parser::set_field (Config& field, const nlohmann::json& cfg_val) {
    using field_t = typename detail::get_real_type<std::remove_reference_t<decltype (field)>>::type;
    if constexpr (std::is_fundamental_v<field_t>) {
      if constexpr(std::is_same_v<field_t, char>) {
        auto val = cfg_val.get<std::string> ();
        if (val.size () != 1) {
          RAISE_EX("Request to parse char, but string with length", val.size (), "found");
        }
        field = val[0];
      }
      else  {
        field = cfg_val.get<field_t> ();
      }
    }
    else {
      if constexpr(detail::is_container<field_t>::value) {
        if (!cfg_val.is_array ()) {
          RAISE_EX("Config value is not an array");
        }
        using elem_t = typename detail::is_container<field_t>::type;
        for (const auto& elem: cfg_val) {
          elem_t e;
          if (elem.is_object ()) {
            parse (e, elem);
          }
          else {
            set_field (e, elem);
          }
          field.push_back (e);
        }
      }
      else {
        if constexpr(std::is_same_v<field_t, std::map<std::string, std::any>>) {
          parse (field, cfg_val);
        }
        else {
          RAISE_EX("Only fundamental types, map<string,any> or vector/list are supported", type_name_v<field_t>, " is supplied");
        }
      }
    }
  }

  template <class Config>
  std::variant<Config, std::vector<Config>> config_parser::parse (const nlohmann::json& map) {
    if (map.is_array()) {
      std::vector<Config> out;
      parse(out, map);
      return out;
    } else if (map.is_object()) {
      Config out;
      parse(out, map);
      return out;
    } else {
      RAISE_EX("Only object/array type is supported as a top level config type");
    }
  }
}


#endif
