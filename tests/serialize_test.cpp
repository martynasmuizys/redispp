#include "core/parser.hpp"
#include <iostream>
#include <source_location>
#include <string>
#include <vector>

#define ASSERT_EQ(lhs, rhs)                                                                        \
    if (lhs != rhs) {                                                                              \
        const auto& loc = std::source_location::current();                                         \
        std::cerr << loc.function_name() << ": \e[31mAssertion failed:\e[0m " << lhs               \
                  << " != " << rhs;                                                                \
        exit(1);                                                                                   \
    }

int main() {

    std::vector<std::string> input_cmd[] = {
        {"ping"}, {"get", "vilkas"}, {"set", "arthur", "vilkas"}, {"set", "desc", "labas krabas"}};
    std::string output_cmd[] = {"*1\r\n$4\r\nping\r\n", "*2\r\n$3\r\nget\r\n$6\r\nvilkas\r\n",
                                "*3\r\n$3\r\nset\r\n$6\r\narthur\r\n$6\r\nvilkas\r\n",
                                "*3\r\n$3\r\nset\r\n$4\r\ndesc\r\n$12\r\nlabas krabas\r\n"};
    auto idx                 = 0;

    for (auto& in : input_cmd) {
        auto lhs = serialize(in, parser::Type::ARRAY);
        auto rhs = output_cmd[idx++];
        ASSERT_EQ(lhs, rhs);
    }
}
