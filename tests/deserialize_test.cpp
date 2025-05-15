#include "core/parser.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <source_location>
#include <string>
#include <vector>

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, std::vector<std::string>& vec) {
    os << "[";

    auto idx = 0;
    for (const auto& v : vec) {
        if (idx == vec.size() - 1) {
            os << v << "]";
            break;
        }
        os << v << ", ";
        ++idx;
    }
    return os;
}

#define ASSERT_EQ(lhs, rhs)                                                                        \
    if (lhs != rhs) {                                                                              \
        const auto& loc = std::source_location::current();                                         \
        std::cerr << loc.function_name() << ": \e[31mAssertion failed:\e[0m " << lhs               \
                  << " != " << rhs;                                                                \
        exit(1);                                                                                   \
    }

int main() {
    std::string input[]               = {"$-1\r\n",
                                         "*1\r\n$4\r\nping\r\n",
                                         "*2\r\n$4\r\necho\r\n$11\r\nhello world\r\n",
                                         "*2\r\n$3\r\nget\r\n$3\r\nkey\r\n",
                                         "+OK\r\n",
                                         "-Error message\r\n",
                                         "$0\r\n\r\n",
                                         "+hello world\r\n"};
    std::vector<std::string> output[] = {{"(nil)"},      {"ping"},       {"echo", "hello world"},
                                         {"get", "key"}, {"OK"},         {"(error) Error message"},
                                         {""},           {"hello world"}

    };

    auto idx = 0;

    for (auto& in : input) {
        // assert(deserialize(cmd.data(), cmd.length()) == expected_out[idx++]);
        auto lhs = parser::deserialize(in.data(), in.length())[0];
        auto rhs = output[idx++];
        ASSERT_EQ(lhs, rhs);
    }

    return 0;
}
