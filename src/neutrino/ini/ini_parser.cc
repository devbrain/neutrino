//
// Created by igor on 7/14/24.
//
#include <memory>
#include <bsw/exception.hh>
#include <bsw/strings/number_parser.hh>
#include <neutrino/ini/config_parser.hh>


#include <ryml.hpp>
#include <ryml_std.hpp>

#include "inih.h"

namespace neutrino::utils {

  namespace detail {
    namespace {
      class json_builder {
        public:
          void add (const std::string& section, const std::string& key, const std::string& value) {
            int ival;
            double fval;
            bool bval;
            if (bsw::number_parser::try_parse (value, ival)) {
              add_internal (section, key, ival);
            } else if (bsw::number_parser::try_parse_float (key, fval)) {
              add_internal (section, key, (float)fval);
            } else if (bsw::number_parser::try_parse_bool (key, bval)) {
              add_internal (section, key, (float)bval);
            } else {
              add_internal (section, key, value);
            }
          }

          nlohmann::json build () {
            if (obj) {
              root[current_section] = *obj;
              obj = nullptr;
            }
            return std::move (root);
          }
        private:
          template <typename T>
          void add_internal (const std::string& section, const std::string& key, const T& value) {
            if (section != current_section) {
              if (obj) {
                root[current_section] = *obj;
                obj = std::make_unique<nlohmann::json> ();
                (*obj)[key] = value;
              }
              else {
                obj = std::make_unique<nlohmann::json> ();
                (*obj)[key] = value;
              }
              current_section = section;
            }
            else {
              if (!obj) {
                obj = std::make_unique<nlohmann::json> ();
              }
              (*obj)[key] = value;
            }
          }

        private:
          std::string current_section;
          nlohmann::json root;
          std::unique_ptr<nlohmann::json> obj;
      };

      std::string to_str (const char* v) {
        if (!v) {
          return "";
        }
        return v;
      }

      int json_ini_handler (void* user, const char* section,
                            const char* name, const char* value) {
        auto* builder = (json_builder*) user;
        builder->add (to_str (section), to_str (name), to_str (value));
        return 1;
      }
    }

    nlohmann::json load_from_ini (const std::string& input) {
      json_builder builder;
      int error_line = ini_parse_string (input.c_str (), json_ini_handler, &builder);
      if (error_line != 0) {
        RAISE_EX("Error occured while parsing INI on line ", error_line);
      }
      return builder.build ();
    }
    // ----------------------------------------------------------------------------------------------
    nlohmann::json load_from_json (const std::string& input) {
      try {
        return nlohmann::json::parse (input);
      }
      catch (const std::exception& e) {
        RAISE_EX("Failed to parse json: ", e.what ());
      }
    }
    // ----------------------------------------------------------------------------------------------
    nlohmann::json load_from_yaml (const std::string& input) {
      try {
        std::vector<char> data(input.size());
        std::memcpy(data.data(), input.c_str(), input.size());
        c4::substr in(data.data (), data.size ());

        auto tree = c4::yml::parse_in_place(in);
        const auto stream = tree.rootref();
        auto is_stream = stream.is_stream();
        if (!is_stream) {
          std::ostringstream ss;
          ss << ryml::as_json (tree);
          return nlohmann::json::parse (ss.str ());
        } else {
          auto out = nlohmann::json::array();
          for(const auto& doc : stream.children()) {
            std::ostringstream ss;
            ss << ryml::as_json(doc);
            out.push_back (nlohmann::json::parse (ss.str ()));
          }
          return out;
        }
      }
      catch (const std::exception& e) {
        RAISE_EX("Failed to parse yaml: ", e.what ());
      }
    }
  }
  // ----------------------------------------------------------------------------------------------------------------
  std::any config_parser::parse_field(const nlohmann::json& cfg_val) {
    if (cfg_val.is_boolean()) {
      return cfg_val.get<bool>();
    } else if (cfg_val.is_string()) {
      return cfg_val.get<std::string>();
    } else if (cfg_val.is_number()) {
      if (cfg_val.is_number_float()) {
        return cfg_val.get<float>();
      } else if (cfg_val.is_number_integer()) {
        return cfg_val.get<int>();
      }
    }
    RAISE_EX("Only booleans, strings and numbers are supported");
  }
  // ----------------------------------------------------------------------------------------------------------------
  std::any config_parser::parse_object(const nlohmann::json& cfg_val) {
    std::map<std::string, std::any> out;
    for (auto i = cfg_val.begin(); i!=cfg_val.end(); i++) {
      const auto& obj = i.value();
      if (obj.is_object()) {
        out.insert ({i.key(), parse_object (obj)});
      } else if (obj.is_array()) {
        out.insert ({i.key(), parse_vector (obj)});
      } else {
        out.insert ({i.key(), parse_field (obj)});
      }
    }
    return out;
  }
  // ----------------------------------------------------------------------------------------------------------------
  std::any config_parser::parse_vector(const nlohmann::json& cfg_val) {
    std::vector<std::any> out;
    for (auto i = cfg_val.begin(); i!=cfg_val.end(); i++) {
      const auto& obj = i.value();
      if (obj.is_object()) {
        out.push_back(parse_object (obj));
      } else if (obj.is_array()) {
        out.push_back (parse_vector (obj));
      } else {
        out.push_back(parse_field (obj));
      }
    }
    return out;
  }
  // ----------------------------------------------------------------------------------------------------------------
  void config_parser::parse (std::map<std::string, std::any>& out, const nlohmann::json& map) {
    if (map.is_object ()) {
      for (auto i = map.begin (); i != map.end (); i++) {
        const auto& obj = i.value ();
        if (obj.is_object ()) {
          out.insert ({i.key (), parse_object (obj)});
        }
        else if (obj.is_array ()) {
          out.insert ({i.key (), parse_vector (obj)});
        }
        else {
          out.insert ({i.key (), parse_field (obj)});
        }
      }
    } else {
      RAISE_EX("Multidocument configuration found. Please use parse(std::vect<std::map<std::string, std::any>>)");
    }
  }
  // ------------------------------------------------------------------------------------------------------------
  void config_parser::parse(std::vector<std::map<std::string, std::any>>& out, nlohmann::json const& map) {
    if (map.is_object()) {
      std::map<std::string, std::any> obj;
      parse(obj, map);
      out.push_back (std::move(obj));
    } else if (map.is_array()) {
      for (const auto &obj : map) {
        std::map<std::string, std::any> obj_out;
        parse(obj_out, obj);
        out.push_back (std::move(obj_out));
      }
    }
  }
  // ------------------------------------------------------------------------------------------------------------
  std::variant<std::string, nlohmann::json>
  config_parser::parser_dispatch (const std::string& input, kind_t kind) {
    nlohmann::json map;
    try {
      switch (kind) {
        case INI:
          map = detail::load_from_ini (input);
          break;
        case JSON:
          map = detail::load_from_json (input);
          break;
        case YAML:
          map = detail::load_from_yaml (input);
          break;
      }
    }
    catch (bsw::exception& e) {
      return std::string{e.what ()};
    }
    catch (std::exception& e) {
      return std::string{e.what ()};
    }

    return map;
  }
}
