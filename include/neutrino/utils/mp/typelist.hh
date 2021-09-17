#ifndef CORE_MPL_TYPELIST_HH
#define CORE_MPL_TYPELIST_HH
#include <cstddef>
#include <tuple>
#include <variant>
#include <type_traits>
#include <initializer_list>

#include <neutrino/utils/mp/if_then_else.hh>

namespace neutrino::mp {
  /// @brief metafunction for generation of a map of message types to
  /// their associated callbacks.
  /// @details
  /// Usage: Use <tt>generate_callback_map<Type>::type</tt> to ...
  /// @tparam Ts the list of message types
  ///

  template <typename... Ts>
  struct type_list {
    using type = type_list;
    using size_type = std::size_t;

    static constexpr size_type size () noexcept {
      return sizeof...(Ts);
    }

    static constexpr bool empty () noexcept {
      return sizeof...(Ts) == 0;
    }

    static constexpr size_type npos = size_type (-1);
  };

  // =============================================================================
  // UTILS
  // =============================================================================

  /// @endcond
  // =============================================================================
  // Algorithms
  // =============================================================================
  namespace detail {
    template <typename X, typename LIST>
    struct type_list_append;

    /// @cond HIDDEN_SYMBOLS
    template <typename X, typename... Ts>
    struct type_list_append<X, type_list<Ts...>> {
      using type = type_list<Ts..., X>;
    };
  } // ns detail
  /// @endcond
  template <typename X, typename LIST>
  using type_list_append_t = typename detail::type_list_append<X, LIST>::type;

  // =============================================================================
  struct null_type;
  namespace detail {
    template <typename X, typename LIST>
    struct type_list_prepend;

    /// @cond HIDDEN_SYMBOLS
    template <typename X, typename... Ts>
    struct type_list_prepend<X, type_list<Ts...>> {
      using type = type_list<X, Ts...>;
    };
    /// @endcond
  }
  template <typename X, typename LIST>
  using type_list_prepend_t = typename detail::type_list_prepend<X, LIST>::type;

  // =============================================================================



  namespace detail {
    template <typename type_list<>::size_type IDX, typename type_list<>::size_type CURRENT, typename LIST>
    struct type_list_type_at_impl;

    template <typename type_list<>::size_type IDX, typename X, typename... Ts>
    struct type_list_type_at_impl<IDX, IDX, type_list<X, Ts...>> {
      using type = X;
    };

    template <typename type_list<>::size_type IDX, typename type_list<>::size_type CURRENT, typename X, typename... Ts>
    struct type_list_type_at_impl<IDX, CURRENT, type_list<X, Ts...>> {
      using type = typename type_list_type_at_impl<IDX, CURRENT + 1, type_list<Ts...>>::type;
    };
    /// @cond HIDDEN_SYMBOLS
    template <typename type_list<>::size_type IDX, typename LIST>
    struct type_list_type_at;

    template <typename type_list<>::size_type IDX, typename... Ts>
    struct type_list_type_at<IDX, type_list<Ts...>> {
      static_assert (IDX < type_list<Ts...>::size (), "Index is out of range");
      using type = typename detail::type_list_type_at_impl<IDX, 0, type_list<Ts...>>::type;
    };
    /// @endcond
  } // ns detail


  template <typename type_list<>::size_type IDX, typename LIST>
  using type_list_at_t = typename detail::type_list_type_at<IDX, LIST>::type;
  // =============================================================================

  namespace detail {
    template <typename LISTA, typename LISTB>
    struct type_list_merge_impl;

    template <typename LISTA>
    struct type_list_merge_impl<LISTA, type_list<>> {
      using type = LISTA;
    };

    template <typename LISTA, typename X, typename... Ts>
    struct type_list_merge_impl<LISTA, type_list<X, Ts...>> {
      using type = typename type_list_merge_impl<type_list_append_t<X, LISTA>, type_list<Ts...>>::type;
    };

    template <typename LISTA, typename LISTB>
    struct type_list_merge;

    template <typename LISTA, typename LISTB>
    struct type_list_merge {
      using type = typename detail::type_list_merge_impl<LISTA, LISTB>::type;
    };
  } // ns detail



  template <typename LISTA, typename LISTB>
  using type_list_merge_t = typename detail::type_list_merge<LISTA, LISTB>::type;
  // =============================================================================


  namespace detail {
    template <template <typename> class PREDICATE, typename RESULT, typename TLIST>
    struct type_list_filter_impl;

    template <template <typename> class PREDICATE, typename RESULT>
    struct type_list_filter_impl<PREDICATE, RESULT, type_list<>> {
      using type = RESULT;
    };

    template <template <typename> class PREDICATE, typename RESULT, typename X, typename... Ts>
    struct type_list_filter_impl<PREDICATE, RESULT, type_list<X, Ts...>> {
      using type = if_then_else_t<PREDICATE<X>::value (),
                                  typename type_list_filter_impl<PREDICATE,
                                                                 type_list_append_t<X, RESULT>,
                                                                 type_list<Ts...>>::type,
                                  typename type_list_filter_impl<PREDICATE, RESULT, type_list<Ts...>>::type>;
    };

    template <template <typename> class PREDICATE, typename TLIST>
    struct type_list_filter;

    template <template <typename> class PREDICATE, typename... Ts>
    struct type_list_filter<PREDICATE, type_list<Ts...>> {
      using type = typename detail::type_list_filter_impl<PREDICATE, type_list<>, type_list<Ts...>>::type;
    };
  } // ns detail

  template <template <typename> class PREDICATE, typename TLIST>
  using type_list_filter_t = typename detail::type_list_filter<PREDICATE, TLIST>::type;
  // ===============================================================================


  namespace detail {
    template <template <typename> class MAPPER, typename RESULT, typename TLIST>
    struct type_list_map_impl;

    template <template <typename> class MAPPER, typename RESULT>
    struct type_list_map_impl<MAPPER, RESULT, type_list<>> {
      using type = RESULT;
    };

    template <template <typename> class MAPPER, typename RESULT, typename X, typename... Ts>
    struct type_list_map_impl<MAPPER, RESULT, type_list<X, Ts...>> {
      using type = typename type_list_map_impl<MAPPER,
                                               type_list_append_t<typename MAPPER<X>::type, RESULT>,
                                               type_list<Ts...>>::type;

    };

    template <template <typename> class MAPPER, typename TLIST>
    struct type_list_map;

    template <template <typename> class MAPPER, typename... Ts>
    struct type_list_map<MAPPER, type_list<Ts...>> {
      using type = typename detail::type_list_map_impl<MAPPER, type_list<>, type_list<Ts...>>::type;
    };
  } // ns detail



  template <template <typename> class MAPPER, typename TLIST>
  using type_list_map_t = typename detail::type_list_map<MAPPER, TLIST>::type;
  // ==========================================================================


  namespace detail {
    template <typename type_list<>::size_type IDX, typename X, typename TLIST>
    struct type_list_find_first_impl;

    template <typename type_list<>::size_type IDX, typename X>
    struct type_list_find_first_impl<IDX, X, type_list<>> {
      static constexpr typename type_list<>::size_type value () noexcept {
        return type_list<>::npos;
      }
    };

    template <typename type_list<>::size_type IDX, typename X, typename... Ts>
    struct type_list_find_first_impl<IDX, X, type_list<X, Ts...>> {
      static constexpr typename type_list<>::size_type value () noexcept {
        return IDX;
      }
    };

    template <typename type_list<>::size_type IDX, typename X, typename A, typename... Ts>
    struct type_list_find_first_impl<IDX, X, type_list<A, Ts...>> {
      static constexpr typename type_list<>::size_type value () noexcept {
        return type_list_find_first_impl<IDX + 1, X, type_list<Ts...>>::value ();
      }
    };

    template <typename X, typename TLIST>
    struct type_list_find_first;

    template <typename X, typename... Ts>
    struct type_list_find_first<X, type_list<Ts...>> {
      static constexpr typename type_list<>::size_type value () noexcept {
        return type_list_find_first_impl<0, X, type_list<Ts...>>::value ();
      }
    };
  } // ns detail



  template <typename X, typename TLIST>
  inline constexpr auto type_list_find_first_v = detail::type_list_find_first<X, TLIST>::value ();
  // ==========================================================================


  namespace detail {
    template <typename type_list<>::size_type IDX, typename X, typename TLIST>
    struct type_list_find_last_impl;

    template <typename type_list<>::size_type IDX, typename X>
    struct type_list_find_last_impl<IDX, X, type_list<>> {
      using result = std::integral_constant<typename type_list<>::size_type, type_list<>::npos>;
    };

    template <typename type_list<>::size_type IDX, typename X, typename A>
    struct type_list_find_last_impl<IDX, X, type_list<A>> {
      using result = std::integral_constant<typename type_list<>::size_type, type_list<>::npos>;
    };

    template <typename type_list<>::size_type IDX, typename X>
    struct type_list_find_last_impl<IDX, X, type_list<X>> {
      using result = std::integral_constant<typename type_list<>::size_type, IDX>;
    };

    template <typename type_list<>::size_type IDX, typename X, typename TLIST>
    using type_list_find_last_impl_result = typename type_list_find_last_impl<IDX, X, TLIST>::result;

    template <typename type_list<>::size_type IDX, typename X, typename H, typename... Ts>
    struct type_list_find_last_impl<IDX, X, type_list<H, Ts...>> {
      using result = if_then_else_t<
          type_list_find_last_impl_result<IDX + 1, X, type_list<Ts...>>::value == type_list<>::npos,
          typename type_list_find_last_impl<IDX, X, type_list<H>>::result,
          type_list_find_last_impl_result<IDX + 1, X, type_list<Ts...>>
      >;
    };
    template <typename X, typename TLIST>
    struct type_list_find_last;

    template <typename X, typename... Ts>
    struct type_list_find_last<X, type_list<Ts...>> {
      static constexpr typename type_list<>::size_type value () noexcept {
        return detail::type_list_find_last_impl_result<0, X, type_list<Ts...>>::value;
      }
    };
  } // ns detail



  template <typename X, typename TLIST>
  inline constexpr auto type_list_find_last_v = detail::type_list_find_last<X, TLIST>::value ();

  // ==========================================================================
  // Conversions
  // ==========================================================================
  namespace detail {
    template <typename TLIST>
    struct typelist_to_tuple;

    template <typename... Ts>
    struct typelist_to_tuple<type_list<Ts...>> {
      using type = std::tuple<Ts...>;
    };
  }

  template <typename TLIST>
  using type_list_to_tuple_t = typename detail::typelist_to_tuple<TLIST>::type;

  namespace detail {
    template <typename TLIST>
    struct typelist_to_variant;

    template <typename... Ts>
    struct typelist_to_variant<type_list<Ts...>> {
      using type = std::variant<Ts...>;
    };
  }

  template <typename TLIST>
  using type_list_to_variant_t = typename detail::typelist_to_variant<TLIST>::type;
  // ====================================================================================

  namespace detail {
    template <typename TUPLE>
    struct tuple_to_type_list;

    template <typename... Ts>
    struct tuple_to_type_list<std::tuple<Ts...>> {
      using type = type_list<Ts...>;
    };
  }
  template <typename TUPLE>
  using tuple_to_type_list_t = typename detail::tuple_to_type_list<TUPLE>::type;

  // ====================================================================================
  namespace detail {
    template <typename TLIST>
    struct typelist_unique;

    template <typename Result, typename TLIST>
    struct typelist_unique_helper;

    template <typename Result>
    struct typelist_unique_helper<Result, type_list<>> {
      using type = Result;
    };

    template <typename Result, typename V, typename... Rest>
    struct typelist_unique_helper<Result, type_list<V, Rest...>> {
      using type = if_then_else_t<type_list_find_first_v<V, Result> == Result::npos,
                                  typename typelist_unique_helper<type_list_append_t<V, Result>,
                                                                  type_list<Rest...>>::type,
                                  typename typelist_unique_helper<Result, type_list<Rest...>>::type>;
    };

    template <>
    struct typelist_unique<type_list<>> {
      using type = type_list<>;
    };

    template <typename ...T>
    struct typelist_unique<type_list<T...>> {
      using type = typename typelist_unique_helper<type_list<>, type_list<T...>>::type;
    };
  } // ns detail
  template <typename TLIST>
  using type_list_unique_t = typename detail::typelist_unique<TLIST>::type;

  namespace detail {
    // first parameter - accumulator
    // second parameter - input list
    template <class T, class U>
    struct flatten_helper;

    // first case - the head of the List is List too
    // expand this List and continue
    template <class... Ts, class... Heads, class... Tail>
    struct flatten_helper<type_list<Ts...>, type_list<type_list<Heads...>, Tail...>> {
      using type = typename flatten_helper<type_list<Ts...>, type_list<Heads..., Tail...>>::type;
    };

    // second case - the head of the List is not a List
    // append it to our new, flattened list
    template <class... Ts, class Head, class... Tail>
    struct flatten_helper<type_list<Ts...>, type_list<Head, Tail...>> {
      using type = typename flatten_helper<type_list<Ts..., Head>, type_list<Tail...>>::type;
    };

    // base case - input List is empty
    // return our flattened list
    template <class... Ts>
    struct flatten_helper<type_list<Ts...>, type_list<>> {
      using type = type_list<Ts...>;
    };

    // wrapper around flatten_helper
    template <class T>
    struct typelist_flatten;

    // start with an empty accumulator
    template <class... Ts>
    struct typelist_flatten<type_list<Ts...>> {
      using type = typename flatten_helper<type_list<>, type_list<Ts...>>::type;
    };
  }
  template <typename TLIST>
  using type_list_flatten_t = typename detail::typelist_flatten<TLIST>::type;
  // ===============================================================================
  namespace detail {
    template <typename T, type_list<>::size_type N>
    struct typelist_repeat_helper {
      using type = type_list_append_t<T, typename typelist_repeat_helper<T, N - 1>::type>;
    };

    template <typename T>
    struct typelist_repeat_helper<T, 0> {
      using type = type_list<>;
    };

    template <typename T, type_list<>::size_type N>
    struct typelist_repeat {
      using type = typename typelist_repeat_helper<T, N>::type;
    };
  }
  template <typename T, type_list<>::size_type N>
  using type_list_repeat_t = typename detail::typelist_repeat<T, N>::type;

  namespace detail {
    template <typename A, typename B> // B is subset of A
    struct type_list_contains_types;

    template <>
    struct type_list_contains_types<type_list<>, type_list<>> {
      static constexpr bool value () {
        return true;
      }
    };

    template <typename ... T>
    struct type_list_contains_types<type_list<>, type_list<T...>> {
      static constexpr bool value () {
        return false;
      }
    };

    template <typename ... T>
    struct type_list_contains_types<type_list<T...>, type_list<>> {
      static constexpr bool value () {
        return false;
      }
    };

    template <typename ... Ta, typename Tb>
    struct type_list_contains_types<type_list<Ta...>, type_list<Tb>> {
      static constexpr bool value () {
        return type_list_find_first_v<Tb, type_list<Ta...>> != type_list < > ::npos;
      }
    };

    template <typename ... Ta, typename Tb0, typename ... Tb>
    struct type_list_contains_types<type_list<Ta...>, type_list<Tb0, Tb...>> {
      static constexpr bool value () {
        return type_list_contains_types<type_list<Ta...>, type_list<Tb0>>::value () ||
               type_list_contains_types<type_list<Ta...>, type_list<Tb...>>::value ();
      }
    };
  }

  template <typename A, typename B> // B is subset of A
  inline constexpr auto type_list_contains_types_v = detail::type_list_contains_types<A, B>::value ();
  // ====================================================================
  /*
  template<typename Seq>
  struct list_length;

  template<typename ... T>
  struct list_length<type_list<T...>>
  {
      static constexpr std::size_t value() noexcept
      {
          return type_list<T...>::size();
      }
  };

  template<typename T, T ... Idx>
  struct list_length<std::integer_sequence<T, Idx...>>
  {
      static constexpr std::size_t value() noexcept
      {
          return sizeof...(Idx);
      }
  };
   */
  // ====================================================================
  namespace detail {
    template <typename... Set>
    struct Check {
      template <typename Type>
      static constexpr bool verify () {
        using accumulator_type = bool[];
        bool check = false;
        accumulator_type accumulator = {(check = check || std::is_same<Type, Set> ())...};
        (void) accumulator;
        return check;
      }

      template <typename... SubSet>
      static constexpr bool contain () {
        using accumulator_type = bool[];
        bool check = true;
        accumulator_type accumulator = {(check = check && verify<SubSet> ())...};
        (void) accumulator;
        return check;
      }
    };

    template <typename, typename>
    struct is_subset_of;

    template <typename ... Types1, typename ... Types2>
    struct is_subset_of<std::tuple<Types1...>, std::tuple<Types2...>> {
      static constexpr bool value = detail::Check<Types2...>::template contain<Types1...> ();
    };

    template <typename ... Types1, typename ... Types2>
    struct is_subset_of<type_list<Types1...>, type_list<Types2...>> {
      static constexpr bool value = detail::Check<Types2...>::template contain<Types1...> ();
    };

    template <typename T, T ... Types1, T ... Types2>
    struct is_subset_of<std::integer_sequence<T, Types1...>, std::integer_sequence<T, Types2...>> {
      static constexpr bool value = detail::Check<std::integral_constant<T, Types2>...>::
      template contain<std::integral_constant<T, Types1>...> ();
    };
  }
  template <typename A, typename B>
  inline constexpr auto is_subset_of_v = detail::is_subset_of<A, B>::value;
} // ns mpl
#endif
