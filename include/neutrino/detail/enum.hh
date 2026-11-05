//
// Created by igor on 04/11/2026.
//

/**
 * @file enum.hh
 * @brief Opt-in bit-flag operators for enumerations.
 *
 * An enumeration becomes a set of bit flags by promoting it with
 * @ref d_NEUTRINO_PROMOTE_ENUM. A promoted enumeration with an unsigned underlying type
 * satisfies @ref neutrino::flags_enum and gets `|`, `&`, `^`, `~`, `|=`, `&=`, `^=` and
 * @ref neutrino::has. Every other type is left alone: unpromoted enumerations keep their own
 * operators (or none), and plain integers keep the built-in ones.
 *
 * @code
 * namespace neutrino {
 *     enum class scene_flags : std::uint8_t { none = 0, hides_below = 1, pauses_below = 2 };
 *     d_NEUTRINO_PROMOTE_ENUM(scene_flags);
 * }
 * auto f = scene_flags::hides_below | scene_flags::pauses_below;
 * if (has(f, scene_flags::pauses_below)) { ... }
 * @endcode
 *
 * The operators are templates in namespace @c neutrino and are found by argument-dependent
 * lookup, so the enumeration must be declared in namespace @c neutrino itself. An enumeration
 * declared elsewhere can be promoted, but its operators are only found after
 * `using neutrino::operator|;` (and the others) in its own namespace.
 */

#pragma once
#include <concepts>
#include <type_traits>

namespace neutrino {
    /**
     * @brief Trait marking an enumeration as a set of bit flags.
     *
     * False for every type; @ref d_NEUTRINO_PROMOTE_ENUM specialises it to true. Use the
     * macro rather than specialising by hand.
     *
     * @tparam T the type to test.
     */
    template<typename T>
    struct is_flag : std::false_type {
    };

    /**
     * @brief An enumeration promoted to bit flags.
     *
     * Satisfied when @p T is an enumeration, its underlying type is unsigned and it was
     * promoted with @ref d_NEUTRINO_PROMOTE_ENUM. A promoted enumeration with a signed
     * underlying type does not satisfy it.
     *
     * @tparam T the type to test.
     */
    template<typename T>
    concept flags_enum = std::is_enum_v <T> && std::is_unsigned_v <std::underlying_type_t <T>> && is_flag<T>::value;

#define d_NEUTRINO_FLAG_OP(OP)                                                                          \
    template <flags_enum T>                                                                             \
    [[nodiscard]] constexpr T operator OP (T a, T b) {                                                  \
        return static_cast <T>(                                                                         \
            static_cast <std::underlying_type_t <T>>(a) OP static_cast <std::underlying_type_t <T>>(b)  \
        );                                                                                              \
    }

#define d_NEUTRINO_FLAG_OP_EQ(OP1, OP2)                                                                 \
    template <flags_enum T>                                                                             \
    constexpr T& operator OP1 (T& a, T b) {                                                             \
        return  a = a OP2 b;                                                                            \
    }

    /// @brief Bitwise OR of two flag sets: every flag set in either.
    d_NEUTRINO_FLAG_OP(|)
    /// @brief Adds the flags of @p b to @p a; returns @p a.
    d_NEUTRINO_FLAG_OP_EQ(|=, |)
    /// @brief Bitwise AND of two flag sets: the flags set in both.
    d_NEUTRINO_FLAG_OP(&)
    /// @brief Keeps in @p a only the flags also set in @p b; returns @p a.
    d_NEUTRINO_FLAG_OP_EQ(&=, &)
    /// @brief Bitwise XOR of two flag sets: the flags set in exactly one.
    d_NEUTRINO_FLAG_OP(^)
    /// @brief Toggles in @p a the flags set in @p b; returns @p a.
    d_NEUTRINO_FLAG_OP_EQ(^=, ^)

#undef d_NEUTRINO_FLAG_OP
#undef d_NEUTRINO_FLAG_OP_EQ

    /**
     * @brief Complement of a flag set over the full width of the underlying type.
     *
     * Every bit of the underlying type is flipped, including bits no enumerator names;
     * `a & ~b` therefore clears exactly the flags of @p b.
     *
     * @param a the flag set.
     * @return the complemented set.
     */
    template<flags_enum T>
    [[nodiscard]] constexpr T operator~(T a) {
        return static_cast <T>(~static_cast <std::underlying_type_t <T>>(a));
    }

    /**
     * @brief Whether @p a contains every flag of @p mask.
     *
     * A mask with several flags requires all of them. An empty mask is contained in every
     * set, so `has(a, T{})` is always true.
     *
     * @param a    the flag set to inspect.
     * @param mask the flags that must all be present.
     * @return true when `(a & mask) == mask`.
     */
    template<flags_enum T>
    constexpr bool has(T a, T mask) {
        const auto av = static_cast <std::underlying_type_t <T>>(a);
        const auto maskv = static_cast <std::underlying_type_t <T>>(mask);
        return (av & maskv) == maskv;
    }
}

/**
 * @def d_NEUTRINO_PROMOTE_ENUM(T)
 * @brief Promotes the enumeration @p T to bit flags (specialises @ref neutrino::is_flag).
 *
 * Use it in namespace @c neutrino, after the enumeration, followed by a semicolon. @p T must
 * have an unsigned underlying type to satisfy @ref neutrino::flags_enum.
 */
#define d_NEUTRINO_PROMOTE_ENUM(T)      \
    template<> struct is_flag <T> : std::true_type {}
