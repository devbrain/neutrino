//
// Created by igor on 12/06/2021.
//

#include <neutrino/hal/events/events_s11n.hh>
#include <neutrino/utils/macros.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::hal::events
{

#define d_KM2STR(MOD)                   \
    if (x & key_mod_t::MOD) {           \
        return STRINGIZE(MOD);          \
    }

#define d_STR2KM(MOD)                   \
    if (x == STRINGIZE(MOD)) {          \
        return key_mod_t::MOD;          \
    }

#define d_KM_FUNC               \
    d_KM_S11(LSHIFT)            \
    d_KM_S11(RSHIFT)            \
    d_KM_S11(LCTRL)             \
    d_KM_S11(RCTRL)             \
    d_KM_S11(LALT)              \
    d_KM_S11(RALT)              \
    d_KM_S11(LGUI)              \
    d_KM_S11(RGUI)              \
    d_KM_S11(NUM)               \
    d_KM_S11(CAPS)              \
    d_KM_S11(MODE)              \
    d_KM_S11(CTRL)              \
    d_KM_S11(ALT)               \
    d_KM_S11(SHIFT)             \
    d_KM_S11(GUI)

#define d_KM_S11(MOD) d_KM2STR(MOD)

    const char* s11n<key_mod_t>::to_string(const key_mod_t& x)
    {
        d_KM_FUNC
        return "";
    }


#undef d_KM_S11
#define d_KM_S11(MOD) d_STR2KM(MOD)

    key_mod_t s11n<key_mod_t>::from_string(const std::string& x)
    {
        d_KM_FUNC
        if (x.empty())
        {
            return key_mod_t::NONE;
        }
        RAISE_EX("Can not deserialize ", x, " as key_mod_t");
    }
    // ===============================================================================================
#define d_SC2STR(SC)  case scan_code_t::SC: return STRINGIZE(SC);
#define d_SC2STR1(SC)  case scan_code_t::PPCAT(_,SC): return STRINGIZE(SC);

#define d_STR2SC(SC)  if (x == STRINGIZE(SC)) return scan_code_t::SC;
#define d_STR2SC1(SC)  if (x == STRINGIZE(SC)) return scan_code_t::PPCAT(_,SC);

#define d_SCFN              \
    d_SC_S11(A)             \
    d_SC_S11(B)             \
    d_SC_S11(C)             \
    d_SC_S11(D)             \
    d_SC_S11(E)             \
    d_SC_S11(F)             \
    d_SC_S11(G)             \
    d_SC_S11(H)             \
    d_SC_S11(I)             \
    d_SC_S11(J)             \
    d_SC_S11(K)             \
    d_SC_S11(L)             \
    d_SC_S11(M)             \
    d_SC_S11(N)             \
    d_SC_S11(O)             \
    d_SC_S11(P)             \
    d_SC_S11(Q)             \
    d_SC_S11(R)             \
    d_SC_S11(S)             \
    d_SC_S11(T)             \
    d_SC_S11(U)             \
    d_SC_S11(V)             \
    d_SC_S11(W)             \
    d_SC_S11(X)             \
    d_SC_S11(Y)             \
    d_SC_S11(Z)             \
    d_SC_S11_1(1)           \
    d_SC_S11_1(2)           \
    d_SC_S11_1(3)           \
    d_SC_S11_1(4)           \
    d_SC_S11_1(5)           \
    d_SC_S11_1(6)           \
    d_SC_S11_1(7)           \
    d_SC_S11_1(8)           \
    d_SC_S11_1(9)           \
    d_SC_S11_1(0)           \
    d_SC_S11(RETURN)        \
    d_SC_S11(ESCAPE)        \
    d_SC_S11(BACKSPACE)     \
    d_SC_S11(TAB)           \
    d_SC_S11(SPACE)         \
    d_SC_S11(MINUS)         \
    d_SC_S11(EQUALS)        \
    d_SC_S11(LEFTBRACKET)   \
    d_SC_S11(RIGHTBRACKET)  \
    d_SC_S11(BACKSLASH)     \
    d_SC_S11(NONUSHASH)     \
    d_SC_S11(SEMICOLON)     \
    d_SC_S11(APOSTROPHE)    \
    d_SC_S11(GRAVE)         \
    d_SC_S11(COMMA)         \
    d_SC_S11(PERIOD)        \
    d_SC_S11(SLASH)         \
    d_SC_S11(CAPSLOCK)      \
    d_SC_S11(F1)            \
    d_SC_S11(F2)            \
    d_SC_S11(F3)            \
    d_SC_S11(F4)            \
    d_SC_S11(F5)            \
    d_SC_S11(F6)            \
    d_SC_S11(F7)            \
    d_SC_S11(F8)            \
    d_SC_S11(F9)            \
    d_SC_S11(F10)           \
    d_SC_S11(F11)           \
    d_SC_S11(F12)           \
    d_SC_S11(PRINTSCREEN)   \
    d_SC_S11(SCROLLLOCK)    \
    d_SC_S11(PAUSE)         \
    d_SC_S11(INSERT)        \
    d_SC_S11(HOME)          \
    d_SC_S11(PAGEUP)        \
    d_SC_S11(DEL)           \
    d_SC_S11(END)           \
    d_SC_S11(PAGEDOWN)      \
    d_SC_S11(RIGHT)         \
    d_SC_S11(LEFT)          \
    d_SC_S11(DOWN)          \
    d_SC_S11(UP)            \
    d_SC_S11(NUMLOCKCLEAR)  \
    d_SC_S11(KP_DIVIDE)     \
    d_SC_S11(KP_MULTIPLY)   \
    d_SC_S11(KP_MINUS)      \
    d_SC_S11(KP_PLUS)       \
    d_SC_S11(KP_ENTER)      \
    d_SC_S11(KP_1)          \
    d_SC_S11(KP_2)          \
    d_SC_S11(KP_3)          \
    d_SC_S11(KP_4)          \
    d_SC_S11(KP_5)          \
    d_SC_S11(KP_6)          \
    d_SC_S11(KP_7)          \
    d_SC_S11(KP_8)          \
    d_SC_S11(KP_9)          \
    d_SC_S11(KP_0)          \
    d_SC_S11(KP_PERIOD)     \
    d_SC_S11(NONUSBACKSLASH)\
    d_SC_S11(APPLICATION)   \
    d_SC_S11(POWER)         \
    d_SC_S11(KP_EQUALS)     \
    d_SC_S11(F13)           \
    d_SC_S11(F14)           \
    d_SC_S11(F15)           \
    d_SC_S11(F16)           \
    d_SC_S11(F17)           \
    d_SC_S11(F18)           \
    d_SC_S11(F19)           \
    d_SC_S11(F20)           \
    d_SC_S11(F21)           \
    d_SC_S11(F22)           \
    d_SC_S11(F23)           \
    d_SC_S11(F24)           \
    d_SC_S11(EXECUTE)       \
    d_SC_S11(HELP)          \
    d_SC_S11(MENU)          \
    d_SC_S11(SELECT)        \
    d_SC_S11(STOP)          \
    d_SC_S11(AGAIN)         \
    d_SC_S11(UNDO)          \
    d_SC_S11(CUT)           \
    d_SC_S11(COPY)          \
    d_SC_S11(PASTE)         \
    d_SC_S11(FIND)          \
    d_SC_S11(MUTE)          \
    d_SC_S11(VOLUMEUP)      \
    d_SC_S11(VOLUMEDOWN)    \
    d_SC_S11(KP_COMMA)      \
    d_SC_S11(KP_EQUALSAS400)\
    d_SC_S11(INTERNATIONAL1)\
    d_SC_S11(INTERNATIONAL2)\
    d_SC_S11(INTERNATIONAL3)\
    d_SC_S11(INTERNATIONAL4)\
    d_SC_S11(INTERNATIONAL5)\
    d_SC_S11(INTERNATIONAL6)\
    d_SC_S11(INTERNATIONAL7)\
    d_SC_S11(INTERNATIONAL8)\
    d_SC_S11(INTERNATIONAL9)\
    d_SC_S11(LANG1)         \
    d_SC_S11(LANG2)         \
    d_SC_S11(LANG3)         \
    d_SC_S11(LANG4)         \
    d_SC_S11(LANG5)         \
    d_SC_S11(LANG6)         \
    d_SC_S11(LANG7)         \
    d_SC_S11(LANG8)         \
    d_SC_S11(LANG9)         \
    d_SC_S11(ALTERASE)      \
    d_SC_S11(SYSREQ)        \
    d_SC_S11(CANCEL)        \
    d_SC_S11(CLEAR)         \
    d_SC_S11(PRIOR)         \
    d_SC_S11(RETURN2)       \
    d_SC_S11(SEPARATOR)     \
    d_SC_S11(KBD_OUT)       \
    d_SC_S11(OPER)          \
    d_SC_S11(CLEARAGAIN)    \
    d_SC_S11(CRSEL)         \
    d_SC_S11(EXSEL)         \
    d_SC_S11(KP_00)         \
    d_SC_S11(KP_000)            \
    d_SC_S11(THOUSANDSSEPARATOR)\
    d_SC_S11(DECIMALSEPARATOR)  \
    d_SC_S11(CURRENCYUNIT)      \
    d_SC_S11(CURRENCYSUBUNIT)   \
    d_SC_S11(KP_LEFTPAREN)      \
    d_SC_S11(KP_RIGHTPAREN)     \
    d_SC_S11(KP_LEFTBRACE)      \
    d_SC_S11(KP_RIGHTBRACE)     \
    d_SC_S11(KP_TAB)            \
    d_SC_S11(KP_BACKSPACE)      \
    d_SC_S11(KP_A)              \
    d_SC_S11(KP_B)              \
    d_SC_S11(KP_C)              \
    d_SC_S11(KP_D)              \
    d_SC_S11(KP_E)              \
    d_SC_S11(KP_F)              \
    d_SC_S11(KP_XOR)            \
    d_SC_S11(KP_POWER)          \
    d_SC_S11(KP_PERCENT)        \
    d_SC_S11(KP_LESS)           \
    d_SC_S11(KP_GREATER)        \
    d_SC_S11(KP_AMPERSAND)      \
    d_SC_S11(KP_DBLAMPERSAND)   \
    d_SC_S11(KP_VERTICALBAR)    \
    d_SC_S11(KP_DBLVERTICALBAR) \
    d_SC_S11(KP_COLON)          \
    d_SC_S11(KP_HASH)           \
    d_SC_S11(KP_SPACE)          \
    d_SC_S11(KP_AT)             \
    d_SC_S11(KP_EXCLAM)         \
    d_SC_S11(KP_MEMSTORE)       \
    d_SC_S11(KP_MEMRECALL)      \
    d_SC_S11(KP_MEMCLEAR)       \
    d_SC_S11(KP_MEMADD)         \
    d_SC_S11(KP_MEMSUBTRACT)    \
    d_SC_S11(KP_MEMMULTIPLY)    \
    d_SC_S11(KP_MEMDIVIDE)      \
    d_SC_S11(KP_PLUSMINUS)      \
    d_SC_S11(KP_CLEAR)          \
    d_SC_S11(KP_CLEARENTRY)     \
    d_SC_S11(KP_BINARY)         \
    d_SC_S11(KP_OCTAL)          \
    d_SC_S11(KP_DECIMAL)        \
    d_SC_S11(KP_HEXADECIMAL)    \
    d_SC_S11(LCTRL)             \
    d_SC_S11(LSHIFT)            \
    d_SC_S11(LALT)              \
    d_SC_S11(LGUI)              \
    d_SC_S11(RCTRL)             \
    d_SC_S11(RSHIFT)            \
    d_SC_S11(RALT)              \
    d_SC_S11(RGUI)              \
    d_SC_S11(MODE)              \
    d_SC_S11(AUDIONEXT)         \
    d_SC_S11(AUDIOPREV)         \
    d_SC_S11(AUDIOSTOP)         \
    d_SC_S11(AUDIOPLAY)         \
    d_SC_S11(AUDIOMUTE)         \
    d_SC_S11(MEDIASELECT)       \
    d_SC_S11(WWW)               \
    d_SC_S11(MAIL)              \
    d_SC_S11(CALCULATOR)        \
    d_SC_S11(COMPUTER)          \
    d_SC_S11(AC_SEARCH)         \
    d_SC_S11(AC_HOME)           \
    d_SC_S11(AC_BACK)           \
    d_SC_S11(AC_FORWARD)        \
    d_SC_S11(AC_STOP)           \
    d_SC_S11(AC_REFRESH)        \
    d_SC_S11(AC_BOOKMARKS)      \
    d_SC_S11(BRIGHTNESSDOWN)    \
    d_SC_S11(BRIGHTNESSUP)      \
    d_SC_S11(DISPLAYSWITCH)     \
    d_SC_S11(KBDILLUMTOGGLE)    \
    d_SC_S11(KBDILLUMDOWN)      \
    d_SC_S11(KBDILLUMUP)        \
    d_SC_S11(EJECT)             \
    d_SC_S11(SLEEP)             \
    d_SC_S11(APP1)              \
    d_SC_S11(APP2)

#define d_SC_S11(SC) d_SC2STR(SC)
#define d_SC_S11_1(SC) d_SC2STR1(SC)

    const char* s11n<scan_code_t>::to_string(const scan_code_t& x)
    {
        switch (x)
        {
            d_SCFN
            default:
                return "";
        }
    }
    // ------------------------------------------------------------------------------------------------
#undef d_SC_S11
#undef d_SC_S11_1

#define d_SC_S11(SC) d_STR2SC(SC)
#define d_SC_S11_1(SC) d_STR2SC1(SC)
    scan_code_t s11n<scan_code_t>::from_string(const std::string& x)
    {
        d_SCFN
        return scan_code_t::UNKNOWN;
    }
    // ================================================================================================
#define d_PB2STR(MOD)                   \
    if (x == pointer_button_t::MOD) {   \
        return STRINGIZE(MOD);          \
    }

#define d_STR2PB(MOD)                   \
    if (x == STRINGIZE(MOD)) {          \
        return pointer_button_t::MOD;   \
    }

#define d_PB_FUNC               \
    d_PB_S11(LEFT)              \
    d_PB_S11(MIDDLE)            \
    d_PB_S11(RIGHT)             \
    d_PB_S11(X1)                \
    d_PB_S11(X2)                \
    d_PB_S11(WHEEL)


#define d_PB_S11(MOD) d_PB2STR(MOD)

    const char* s11n<pointer_button_t>::to_string(const pointer_button_t& x)
    {
        d_PB_FUNC
        return "";
    }

#undef d_PB_S11
#define d_PB_S11(MOD) d_STR2PB(MOD)

    pointer_button_t s11n<pointer_button_t>::from_string(const std::string& x)
    {
        d_PB_FUNC
        RAISE_EX("Can not deserialize ", x, " as pointer_button_t");
    }
}