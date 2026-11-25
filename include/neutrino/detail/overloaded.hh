//
// Created by igor on 24/11/2026.
//

#pragma once

namespace neutrino::detail {
    // The "overloaded" helper pattern
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    // (Explicit deduction guide only needed for C++17; C++20/23/26 deduces this automatically)
    // template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
/*
    int main() {
        std::variant<int, double, std::string> v = 42;

        std::visit(overloaded {
            [](int x) { std::cout << "Int: " << x << '\n'; },
            [](double x) { std::cout << "Double: " << x << '\n'; },
            [](const std::string& x) { std::cout << "String: " << x << '\n'; }
        }, v);
    }
*/
}