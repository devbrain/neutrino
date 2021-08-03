//
// Created by igor on 04/04/2021.
//

#ifndef NEUTRINO_UTILS_STRING_CONSTANT_HH
#define NEUTRINO_UTILS_STRING_CONSTANT_HH

#include <cstddef>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <array>

namespace neutrino::utils
{
// Recursive comparison of each individual character in a string
// The last bit with std::enable_if uses SFINAE (Substitution Failure Is Not An Error)
// to rule this function out and switch to the base case for the recursion when the Index == Length
    template<std::size_t Length, std::size_t Index, typename Left, typename Right>
    constexpr auto
    compare_characters(const Left& lhs, const Right& rhs) -> typename std::enable_if<Index != Length, bool>::type
    {
        return lhs[Index] == rhs[Index] && compare_characters<Length, Index + 1>(lhs, rhs);
    }

// Recursion base case. If you run past the last index of
    template<std::size_t Length, std::size_t Index, typename Left, typename Right, typename std::enable_if<
            Index == Length, bool>::type = 0>
    constexpr bool compare_characters([[maybe_unused]] const Left& lhs, [[maybe_unused]]  const Right& rhs)
    {
        return true;
    }

// Helper type traits to determine the length of either
// a string literal or a StringConstant (specialized below)
    template<typename T>
    struct length_of
    {
        static_assert(std::is_void<T>::value, "Unsupported type for length_of");

        static constexpr std::size_t value = 1;
    };

    template<std::size_t N>
    struct length_of<const char (&)[N]>
    {
        static constexpr std::size_t value = N - 1;
    };

    template<std::size_t N>
    struct length_of<char[N]>
    {
        static constexpr std::size_t value = N - 1;
    };

    template<std::size_t N>
    struct length_of<const char[N]>
    {
        static constexpr std::size_t value = N - 1;
    };

// This small class is the heart of the constant string implementation.
// It has constructors for string literals and individual chars, as well
// as operators to interact with string literals or other instances. This
// allows for it to have a very natural interface, and it's all constexpr
// Inspired heavily by a class described in a presentation by Scott Schurr
// at Boostcon:
// https://github.com/boostcon/cppnow_presentations_2012/blob/master/wed/schurr_cpp11_tools_for_class_authors.pdf
    template<std::size_t N>
    class string_constant
    {
    public:
        // Constructor which takes individual chars. Allows for unpacking
        // parameter packs directly into the constructor
        template<typename... Characters>
        constexpr explicit string_constant(Characters... characters)
                : m_value{characters..., '\0'}
        {
        }

        // Copy constructor
        template<std::size_t... Indexes>
        constexpr
        explicit string_constant(const string_constant<N>& rhs,
                                 [[maybe_unused]] std::index_sequence<Indexes...> dummy = string_constant<sizeof...(Indexes)>::g_indexes)
                : m_value{rhs[Indexes]..., '\0'}
        {
        }

        template<std::size_t X, std::size_t... Indexes>
        constexpr string_constant(const string_constant<X>& rhs, [[maybe_unused]] std::index_sequence<Indexes...> dummy)
                : m_value{rhs[Indexes]..., '\0'}
        {
        }

        template<std::size_t... Indexes>
        constexpr string_constant(const char(& value)[N + 1], [[maybe_unused]] std::index_sequence<Indexes...> dummy)
                : string_constant(value[Indexes]...)
        {
        }

        constexpr explicit string_constant(const char(& value)[N + 1])
                : string_constant(value, std::make_index_sequence<N>{})
        {
        }

        // Array subscript operator, with some basic range checking
        constexpr char operator[](const std::size_t index) const
        {
            return index < N ? m_value[index] : throw std::out_of_range("Index out of range");
        }

        [[nodiscard]] constexpr const char* c_str() const
        { return m_value.data(); }

        [[nodiscard]] constexpr std::size_t length() const
        { return N; }

        [[nodiscard]] std::string to_string() const
        { return std::string(m_value); }

    protected:
        const std::array<char, N + 1> m_value;

        static constexpr auto g_indexes = typename std::make_index_sequence<N>{};
    };

// Specialize the length_of trait for the StringConstant class
    template<std::size_t N>
    struct length_of<string_constant<N> >
    {
        static constexpr std::size_t value = N;
    };

    template<std::size_t N>
    struct length_of<const string_constant<N> >
    {
        static constexpr std::size_t value = N;
    };

    template<std::size_t N>
    struct length_of<const string_constant<N>&>
    {
        static constexpr std::size_t value = N;
    };

// A helper trait for checking if something is a StringConstant
// without having to know the length of the string it contains
    template<typename T>
    struct is_string_constant
    {
        static constexpr bool value = false;
    };

    template<std::size_t N>
    struct is_string_constant<string_constant<N> >
    {
        static constexpr bool value = true;
    };

    template<std::size_t N>
    struct is_string_constant<string_constant<N>&>
    {
        static constexpr bool value = true;
    };

    template<std::size_t N>
    struct is_string_constant<const string_constant<N> >
    {
        static constexpr bool value = true;
    };

    template<std::size_t N>
    struct is_string_constant<const string_constant<N>&>
    {
        static constexpr bool value = true;
    };


// A helper function for concatenating StringConstants

// Less than human friendly concat function, wrapped by a huamn friendly one below
    template<typename Left, typename Right, std::size_t... IndexesLeft, std::size_t... IndexesRight>
    constexpr string_constant<sizeof...(IndexesLeft) + sizeof...(IndexesRight)>
    concat_strings(const Left& lhs, const Right& rhs, [[maybe_unused]] std::index_sequence<IndexesLeft...> dummy1,
                   [[maybe_unused]] std::index_sequence<IndexesRight...> dummy2)
    {
        return string_constant<sizeof...(IndexesLeft) + sizeof...(IndexesRight)>(lhs[IndexesLeft]...,
                                                                                 rhs[IndexesRight]...);
    }

// Human friendly concat function for string literals
    template<typename Left, typename Right>
    constexpr string_constant<length_of<Left>::value + length_of<Right>::value>
    concat_strings(const Left& lhs, const Right& rhs)
    {
        return concat_strings(lhs, rhs, typename std::make_index_sequence<length_of<decltype(lhs)>::value>{},
                              typename std::make_index_sequence<length_of<decltype(rhs)>::value>{});
    }


// Finally, operators for dealing with a string literal LHS and StringConstant RHS

// Addition operator
    template<std::size_t N, typename Right>
    constexpr string_constant<N + length_of<Right>::value> operator+(const string_constant<N>& lhs, const Right& rhs)
    {
        return concat_strings(lhs, rhs);
    }

    template<typename Left, std::size_t N>
    constexpr string_constant<length_of<Left>::value + N> operator+(const Left& lhs, const string_constant<N>& rhs)
    {
        return concat_strings(lhs, rhs);
    }

    template<std::size_t X, std::size_t Y>
    constexpr string_constant<X + Y> operator+(const string_constant<X>& lhs, const string_constant<Y>& rhs)
    {
        return concat_strings(lhs, rhs);
    }

// Equality operator
    template<std::size_t N, typename Right>
    constexpr auto operator==(const string_constant<N>& lhs, const Right& rhs) -> typename std::enable_if<
            N == length_of<Right>::value, bool>::type
    {
        return compare_characters<N, 0>(lhs, rhs);
    }

    template<typename Left, std::size_t N>
    constexpr auto operator==(const Left& lhs, const string_constant<N>& rhs) -> typename std::enable_if<
            length_of<Left>::value == N, bool>::type
    {
        return compare_characters<N, 0>(lhs, rhs);
    }

    template<std::size_t X, std::size_t Y>
    constexpr auto operator==(const string_constant<X>& lhs, const string_constant<Y>& rhs) -> typename std::enable_if<
            X == Y, bool>::type
    {
        return compare_characters<X, 0>(lhs, rhs);
    }

// Different length strings can never be equal
    template<std::size_t N, typename Right, typename std::enable_if<N != length_of<Right>::value, bool>::type = 0>
    constexpr bool operator==(const string_constant<N>& lhs, const Right& rhs)
    {
        return false;
    }

// Different length strings can never be equal
    template<typename Left, std::size_t N, typename std::enable_if<length_of<Left>::value != N, bool>::type = 0>
    constexpr bool operator==(const Left& lhs, const string_constant<N>& rhs)
    {
        return false;
    }

// Different length strings can never be equal
    template<std::size_t X, std::size_t Y, typename std::enable_if<X != Y, bool>::type = 0>
    constexpr bool operator==(const string_constant<X>& lhs, const string_constant<Y>& rhs)
    {
        return false;
    }

    template<std::size_t N, std::size_t... Indexes>
    constexpr auto string_factory(const char(& value)[N], [[maybe_unused]] std::index_sequence<Indexes...> dummy)
    {
        return string_constant<N - 1>(value[Indexes]...);
    }

// A helper factory function for creating FixedStringConstant objects
// which handles figuring out the length of the string for you
    template<std::size_t N>
    constexpr auto string_factory(const char(& value)[N])
    {
        return string_factory(value, typename std::make_index_sequence<N - 1>{});
    }
} // ns neutrino::utils
#endif //NEUTRINO_UTILS_STRING_CONSTANT_HH
