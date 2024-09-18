//
// Created by igor on 9/17/24.
//

#ifndef CC1_TEXT_RESOURCES_HH
#define CC1_TEXT_RESOURCES_HH

#include <bsw/macros.hh>
#include <array>
#include <cstdint>
#include "data_loader/text_resource_loader.hh"



#define TEXT_RESOURCE_PRINT_ENUM_(i, x, y) PPCAT(CC1_TEXT_RESOURCE_, x) = i,
#define TEXT_RESOURCE_PRINT_ARR_(i, x, y) PPCAT(0x, y),
#define DECLARE_CC1_TEXT_RESOURCE(NAME, KIND ,...)                                                              \
enum PPCAT(cc1_, PPCAT(NAME, _resources)) {                                                                     \
    FOR_EACH_2_COUNT(TEXT_RESOURCE_PRINT_ENUM_, __VA_ARGS__)                                                    \
    };                                                                                                          \
struct PPCAT(cc1_, PPCAT(NAME, _offsets)) : public std::array<uint64_t, PP_NARG(__VA_ARGS__)/2> {               \
    PPCAT(cc1_, PPCAT(NAME, _offsets))() : std::array<uint64_t, PP_NARG(__VA_ARGS__)/2>{                        \
        FOR_EACH_2_COUNT(TEXT_RESOURCE_PRINT_ARR_, __VA_ARGS__)                                                 \
    } {}                                                                                                        \
    };                                                                                                          \
    template <>                                                                                                 \
    struct text_resources_traits<PPCAT(cc1_, PPCAT(NAME, _resources))> {                                        \
        static constexpr std::size_t count = PP_NARG(__VA_ARGS__)/2;                                            \
        using offsets_t = PPCAT(cc1_, PPCAT(NAME, _offsets));                                                   \
        static constexpr auto kind = KIND;                                                                      \
    }

DECLARE_CC1_TEXT_RESOURCE(menu, text_resource::MENUS,
    QUIT,              13280,
    MAIN_MENU,         1925e,
    QUIT_MENU,         17bcc,
    SOUND_EFFECTS_ON,  13323,
    SWITCH_JOYSTICK,   17a63,
    HELP_MENU     ,    19666,
    ABOUT_APOGEE  ,    1ba90,
    INTRO_1,           1c609,
    INTRO_2,           1c6cb,
    OUTRO,             1c76d,
    TOP_SCORE,         15a64,
    TOP_SCORE_INPUT,   15cb8,
    VIDEO,             1c2da
    );

DECLARE_CC1_TEXT_RESOURCE(intro, text_resource::INSTRUCTIONS,
    INSTRUCTIONS_1,    15df3,
    INSTRUCTIONS_2,    15f67,
    INSTRUCTIONS_3,    160b8,
    INSTRUCTIONS_4,    16221,
    INSTRUCTIONS_5,    16387,
    INSTRUCTIONS_6,    164b7,
    INSTRUCTIONS_7,    165dc
    );

DECLARE_CC1_TEXT_RESOURCE(in_game, text_resource::IN_GAME,
    GRAVITY       ,    1be27,
    PERFECT_HEALTH,    1bfb2,
    RED_MUSHROOM  ,    180c9,
    ACTIVATE_LEVER,    181ad,
    ACTIVATE_SWITCH,   18279,
    CREATURES_STUNNED, 176a0,
    DONT_GIVE_UP,      177f5,
    PISTOL_BONUS,      18346,
    GREEN_MUSHROOM,    18423,
    AIR_GENERATOR,     18914
);

DECLARE_CC1_TEXT_RESOURCE(story, text_resource::STORY,
        STORY1, 14737,
        STORY2, 14891
    );

DECLARE_CC1_TEXT_RESOURCE(ordering, text_resource::ORDERING,
        ORDER1, 1558e,
        ORDER2, 15229,
        ORDER3, 14ed3,
        FORDER1, 1aab2,
        FORDER2, 1ac2b,
        FORDER3, 1ad29,
        FORDER4, 1ae2e,
        BBS,     1b711
    );

#endif
