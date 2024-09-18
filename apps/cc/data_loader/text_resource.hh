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
            MENUS,
            INSTRUCTIONS,
            STORY,
            ORDERING
        };

        [[nodiscard]] std::string get(kind_t kind, int id) const;
        void bind_text(kind_t kind, int id, const std::string& text);

    private:
        using text_map_t = std::map <int, std::string>;
        std::map <kind_t, text_map_t> m_text;
};

#endif
