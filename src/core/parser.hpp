#pragma once

#include <string>
#include <vector>

namespace parser {

enum Type {
    S_STRING = '+',
    S_ERROR  = '-',
    INTEGER  = ':',
    B_STRING = '$',
    ARRAY    = '*',
};

std::vector<std::vector<std::string>> deserialize(char* cmd, int len, bool client = false);
std::string serialize(const std::vector<std::string>& data, Type type = Type::B_STRING);

std::string parse_s_string(char* cmd);
std::string parse_b_string(char* cmd);
std::string parse_int(char* cmd);
std::string parse_error(char* cmd);
std::pair<std::vector<std::string>, int> parse_array(char* cmd);

} // namespace parser
