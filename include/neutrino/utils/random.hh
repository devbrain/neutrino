//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_UTILS_RANDOM_HH
#define NEUTRINO_UTILS_RANDOM_HH

#include <random>
#include <algorithm>
#include <array>
#include <neutrino/neutrino_export.hh>

// https://github.com/MatthewObi/SimpleRandomAPI/blob/main/Random.hpp

namespace neutrino {
    class NEUTRINO_EXPORT random {
        private:
            struct std_rng {
                std_rng()
                    : rd{},
                      mt(rd()) {
                }

                void seed(long x) {
                    mt.seed(x);
                }

                std::random_device rd;
                std::mt19937 mt;
            };

        private:
            std_rng rng;

        public:
            random() = default;

            random(const random&) = delete;
            random(random&&) = delete;
            random& operator=(const random&) = delete;
            random& operator=(random&&) = delete;
        private:
            struct charset {
                /// <summary>
                /// Charset for base64 strings.
                /// </summary>
                static constexpr const char* Base64
                    = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-";

                /// <summary>
                /// Charset for alphabetic strings.
                /// </summary>
                static constexpr const char* Alpha
                    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

                /// <summary>
                /// Charset for alphanumeric strings.
                /// </summary>
                static constexpr const char* AlphaNum
                    = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

                /// <summary>
                /// Charset for numeric strings.
                /// </summary>
                static constexpr const char* Numeric
                    = "0123456789";

                /// <summary>
                /// Charset for hexadecimal strings.
                /// </summary>
                static constexpr const char* Hex
                    = "0123456789ABCDEF";

                /// <summary>
                /// Charset for binary strings.
                /// </summary>
                static constexpr const char* Binary
                    = "01";
            };

        private:
            static random& get() noexcept {
                static random instance;
                return instance;
            }

            template<typename T>
            T get_int_impl(T begin, T end) {
                std::uniform_int_distribution <T> dis(begin, end);
                return dis(rng.mt);
            }

            template<typename T>
            T get_int_binomial_impl(T t, double p) {
                std::binomial_distribution <T> dis{t, p};
                return dis(rng.mt);
            }

            template<typename T, size_t N>
            std::array <T, N>&& get_int_array_impl(T begin, T end) {
                std::array <T, N>&& arr{};
                std::uniform_int_distribution <T> distribution{begin, end};
                for (auto& i : arr) {
                    i = distribution(rng.mt);
                }
                return std::move(arr);
            }

            template<typename Float_t, size_t N>
            std::array <Float_t, N>&& get_float_array_impl(Float_t min, Float_t max) {
                std::array <Float_t, N>&& arr{};
                std::uniform_real_distribution <Float_t> distribution(min, max);
                for (auto& i : arr) {
                    i = distribution(rng.mt);
                }
                return std::move(arr);
            }

            template<typename Float_t>
            Float_t get_float_impl(Float_t min, Float_t max) {
                std::uniform_real_distribution <Float_t> dis{min, max};
                return dis(rng.mt);
            }

            template<typename Float_t>
            Float_t get_float_normal_impl(Float_t mean, Float_t stddev = static_cast <Float_t>(1.0)) {
                std::normal_distribution <Float_t> dis{mean, stddev};
                return dis(rng.mt);
            }

            std::string get_string_impl(char begin, char end, const size_t length) {
                std::uniform_int_distribution <std::int16_t> dis{begin, end};
                std::string str;
                str.resize(length + 1);
                for (size_t i = 0; i < length + 1; i++) {
                    str[i] = static_cast <char>(dis(rng.mt));
                }
                return str;
            }

            std::string get_string_impl(std::string_view charset, const size_t length) {
                std::uniform_int_distribution <size_t> dis{0, charset.length() - 1};
                std::string str;
                str.resize(length + 1);
                for (size_t i = 0; i < length + 1; i++) {
                    str[i] = charset[dis(rng.mt)];
                }
                return str;
            }

        public:
            /// <summary>
            /// Generates a random integer of chosen size between begin and end (inclusive).
            /// Uses a uniform distribution.
            /// </summary>
            /// <param name="begin"></param>
            /// <param name="end"></param>
            /// <returns></returns>
            template<class Int_t>
            static Int_t get_int(Int_t begin, Int_t end) {
                return get().get_int_impl(begin, end);
            }

            /// <summary>
            /// Generates a random integer of word size between 0 and t (inclusive).
            /// Uses a binomial distribution with a probability of p.
            /// </summary>
            /// <param name="t"></param>
            /// <param name="p"></param>
            /// <returns></returns>
            template<class Int_t>
            static Int_t get_int_binomial(Int_t t, double p) {
                return get().get_int_binomial_impl(t, p);
            }

            /// <summary>
            /// Generates a random unsigned 8-bit integer between 0 and 255 (inclusive).
            /// Uses a uniform distribution.
            /// </summary>
            static unsigned char get_byte() {
                return static_cast <unsigned char>(get_int <int16_t>(0, 255));
            }

            /// <summary>
            /// Generates a random float between min and max (exclusive).
            /// Uses a uniform distribution.
            /// </summary>
            /// <param name="min"></param>
            /// <param name="max"></param>
            /// <returns></returns>
            template<typename Float_t>
            static Float_t get_float(Float_t min, Float_t max) {
                return get().get_float_impl(min, max);
            }

            /// <summary>
            /// Generates a random float between min and max (exclusive).
            /// Uses a uniform distribution.
            /// </summary>
            template<typename Float_t>
            static Float_t get_float_normal(Float_t mean, Float_t stddev = static_cast <Float_t>(1.0)) {
                return get().get_float_normal_impl(mean, stddev);
            }

            /// <summary>
            /// Generates a random double between 0.0 and 1.0 (exclusive), and returns true if the double is
            /// less than pct.
            /// </summary>
            /// <param name="pct">The percentage chance that the function returns true.</param>
            /// <returns></returns>
            static bool chance(double pct) {
                pct = std::clamp(pct, 0.0, 1.0);
                return (get().get_float_impl(0.0, 1.0)) < pct;
            }

            /// <summary>
            /// Generates a random integer between 1 and d (inclusive), and returns true if the integer is
            /// less than or equal to n.
            /// </summary>
            /// <param name="n">Numerator of the fraction.</param>
            /// <param name="d">Denominator of the fraction.</param>
            /// <returns></returns>
            static bool chance(int n, int d) {
                n = std::clamp(n, 0, d);
                return get().get_int_impl(1, d) <= n;
            }

            /// <summary>
            /// Shuffles a container between iterators begin and end.
            /// </summary>
            /// <typeparam name="Iter_t">Iterator type</typeparam>
            /// <param name="begin">Start iterator.</param>
            /// <param name="end">End iterator.</param>
            template<typename Iter_t>
            static void shuffle(Iter_t begin, Iter_t end) {
                std::shuffle(begin, end, get().rng.mt);
            }

            /// <summary>
            /// Shuffles a container.
            /// </summary>
            /// <typeparam name="Container_t">Iterator type</typeparam>
            /// <param name="container">Reference to container to be shuffled.</param>
            template<typename Container_t>
            static void shuffle(Container_t& container) {
                std::shuffle(container.begin(), container.end(), get().rng.mt);
            }

            /// <summary>
            /// Copies a container, then shuffles and returns the copy.
            /// </summary>
            /// <typeparam name="Container_t">Iterator type</typeparam>
            /// <param name="container">Constant reference to container to be copied and shuffled.</param>
            template<typename Container_t>
            static Container_t&& shuffle_copy(const Container_t& container) {
                Container_t copy = container;
                std::shuffle(copy.begin(), copy.end(), get().rng.mt);
                return std::move(copy);
            }

            /// <summary>
            /// Generates a string of length "length" + 1 with characters between begin and end (inclusive).
            /// </summary>
            /// <param name="begin"></param>
            /// <param name="end"></param>
            /// <param name="length"></param>
            /// <returns></returns>
            static std::string get_string(char begin, char end, const size_t length) {
                return get().get_string_impl(begin, end, length);
            }

            /// <summary>
            /// Generates a string of length "length" + 1 with a defined charset.
            /// </summary>
            /// <param name="length"></param>
            /// <returns></returns>
            static std::string get_string(std::string_view charset, const size_t length) {
                return get().get_string_impl(charset, length);
            }

            static void seed(long x) {
                get().rng.seed(x);
            }

        public:
            /// <summary>
            /// Generates N random integers between begin and end and returns them in a std::array
            /// by r-value reference.
            /// </summary>
            /// <param name="begin"></param>
            /// <param name="end"></param>
            /// <returns></returns>
            template<size_t N, typename Int_t>
            static std::array <Int_t, N>&& get_int_array(Int_t begin, Int_t end) {
                return std::move(get().get_int_array_impl <Int_t, N>(begin, end));
            }

            /// <summary>
            /// Generates N random floats between begin and end and returns them in a std::array
            /// by r-value reference.
            /// </summary>
            /// <param name="begin"></param>
            /// <param name="end"></param>
            /// <returns></returns>
            template<size_t N, typename Float_t>
            static std::array <Float_t, N>&& get_float_array(Float_t begin, Float_t end) {
                return std::move(get().get_float_array_impl <Float_t, N>(begin, end));
            }
    };
}

#endif
