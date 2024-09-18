//
// Created by igor on 9/17/24.
//

#ifndef  TEXT_RESOURCE_HH
#define  TEXT_RESOURCE_HH

#include <string>
#include <map>

class text_resource {
    public:
        enum kind_t {
            IN_GAME,
            MENUS
        };

        template<typename Enum>
        friend void load_text_resource(std::istream& unpacked_exe, kind_t, text_resource&);

        [[nodiscard]] std::string get(kind_t kind, int id) const;

    private:
        void bind_text(kind_t kind, int id, const std::string& text);

        std::map <int, std::string> m_menu_text;
        std::map <int, std::string> m_game_text;
};

#endif
