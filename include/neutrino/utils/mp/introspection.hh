//
// Created by igor on 02/06/2020.
//

#ifndef SDLPP_INTROSPECTION_HH
#define SDLPP_INTROSPECTION_HH

#include <type_traits>

#include <neutrino/utils/macros.hh>

/**
 * This macro generates traits class for determination if the given class
 * has member function with specific signature.
 * Example:
 * struct A {
 * void int f(double a, const char* b);
 * };
 * GENERATE_HAS_MEMBER_FUNCTION(f);
 * .
 * .
 * .
 * if constexpr (has_f_v<A, int(double, const char*)>) {
 * ...}
 */

#define GENERATE_HAS_MEMBER_FUNCTION(NAME)                                                      \
    template<typename C, typename T>                                                            \
    struct PPCAT(has_, NAME) {                                                                  \
        static_assert(                                                                          \
                std::integral_constant<T, false>::value,                                        \
                "Second template parameter needs to be of function type.");                     \
    };                                                                                          \
    template<typename C, typename Ret, typename... Args>                                        \
    struct PPCAT(has_, NAME)<C, Ret(Args...)> {                                                 \
    private:                                                                                    \
    template<typename T>                                                                        \
    static constexpr T* as_ptr() {return nullptr;}                                              \
                                                                                                \
    template<typename T>                                                                        \
    static constexpr auto check(T*)                                                             \
        -> typename                                                                             \
        std::is_same<                                                                           \
                decltype( as_ptr<T>()->NAME( std::declval<Args>()... ) ),                       \
                Ret                                                                             \
        >::type;                                                                                \
        template<typename>                                                                      \
        static constexpr std::false_type check(...);                                            \
    public:                                                                                     \
        typedef decltype(check<C>(0)) type;                                                     \
    public:                                                                                     \
        static constexpr bool value = type::value;                                              \
    }

/**
 * This macro generates traits class for determination if the given class
 * has static member (i.e., something that could be referenced as T::member) with specific type.
 * Example:
 * struct A {
 * static constexpr int type = 0xDEAD;
 * };
 * GENERATE_HAS_MEMBER_STATIC(type);
 * .
 * .
 * .
 * if constexpr (has_type_v<A, int>) {
 * ...}
 */


#define GENERATE_HAS_MEMBER_STATIC(NAME)                                                        \
    template<typename C, typename T>                                                            \
    struct PPCAT(has_, NAME) {                                                                  \
    private:                                                                                    \
        template<typename U>                                                                    \
        static constexpr auto check(U*)                                                         \
        -> typename                                                                             \
        std::is_same<std::remove_const_t<decltype(U::NAME)>, T>::type;                          \
        template<typename>                                                                      \
        static constexpr std::false_type check(...);                                            \
    public:                                                                                     \
        typedef decltype(check<C>(0)) type;                                                     \
    public:                                                                                     \
        static constexpr bool value = type::value;                                              \
    }



/**
 * This macro generates traits class for determination if the given class
 * has member function with specific arguments, no knowledge of return type is required
 * Example:
 * struct A {
 * void int f(double a, const char* b);
 * };
 * GENERATE_HAS_MEMBER_WEAK(f);
 * .
 * .
 * .
 * if constexpr (has_f_v<A, double, const char*>) {
 * ...}
 */
#define GENERATE_HAS_MEMBER_WEAK(NAME)                                                                              \
namespace PPCAT(PPCAT(detail::has_,NAME),_type)  {                                                                  \
    template<typename valid, typename C, typename... Args>                                                          \
    struct ret {};                                                                                                  \
    template<typename C, typename... Args>                                                                          \
    struct ret<std::true_type, C, Args...>                                                                          \
    {                                                                                                               \
        template<typename T>                                                                                        \
        static constexpr T* as_ptr() {return nullptr; }                                                             \
        using type = decltype(as_ptr<C>()->NAME(std::declval<Args>()...));                                          \
    };                                                                                                              \
}                                                                                                                   \
template<typename C, typename... Args>                                                                              \
struct PPCAT(has_,NAME)                                                                                             \
{                                                                                                                   \
private:                                                                                                            \
    template<typename T>                                                                                            \
    static constexpr T* as_ptr() {return nullptr; }                                                                 \
    template<typename T>                                                                                            \
    static constexpr auto check(T*)                                                                                 \
    -> typename std::is_same<std::void_t<decltype(as_ptr<T>()->NAME(std::declval<Args>()...))>, void>::type;        \
                                                                                                                    \
    template<typename>                                                                                              \
    static constexpr std::false_type check(...);                                                                    \
public:                                                                                                             \
    typedef decltype(check<C>(0)) type;                                                                             \
public:                                                                                                             \
    static constexpr bool value = type::value;                                                                      \
};                                                                                                                  \
template<typename C, typename... Args>                                                                              \
using PPCAT(return_type_of_, NAME) = typename PPCAT(PPCAT(detail::has_,NAME),_type)::                               \
                                                  ret<typename PPCAT(has_, NAME)<C,Args...>::type, C, Args...>::type

#endif //SDLPP_INTROSPECTION_HH
