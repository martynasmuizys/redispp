#include "parser.hpp"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace parser;

std::string parser::serialize(const std::vector<std::string>& data, Type type) {
    std::stringstream ss;
    std::vector<std::string> parsed_data = {};

    if (data.size() > 1 || type == Type::ARRAY) {

        parsed_data.push_back(
            std::string("$" + std::to_string(data[0].length()) + "\r\n" + data[0] + "\r\n"));
        for (size_t i = 1; i < data.size(); ++i) {

            std::string str;
            if (data[i][0] == '"') {
                str.append(std::string(data[i].begin() + 1, data[i].end()));
                str.push_back(' ');
                ++i;
                while (data[i][data[i].length() - 1] != '"') {
                    str.append(data[i]);
                    str.push_back(' ');
                    ++i;
                }
                str.append(std::string(data[i].begin(), data[i].end() - 1));
                // Removing "s from length
            } else {
                str = data[i];
            }
            parsed_data.push_back(
                std::string("$" + std::to_string(str.length()) + "\r\n" + str + "\r\n"));
        }

        ss << '*' << parsed_data.size() << "\r\n";
        for (auto d : parsed_data) {
            ss << d;
        }

        return ss.str();
    }

    if (type == Type::S_STRING) {
        ss << '+' << data[0] << "\r\n";
    } else if (type == Type::B_STRING) {
        ss << '$' << data[0].length() << "\r\n" << data[0] << "\r\n";
    } else if (type == Type::INTEGER) {
        ss << ':' << data[0] << "\r\n";
    } else if (type == Type::S_ERROR) {
        ss << '-' << data[0] << "\r\n";
    }

    return ss.str();
}

std::vector<std::vector<std::string>> parser::deserialize(char* cmd, int len, bool client) {
    std::vector<std::vector<std::string>> ret = {};

    int idx = 0;
    while (idx < len) {

        switch (Type(cmd[idx])) {
            case S_STRING: {
                auto str = parse_s_string(cmd + idx);
                ret.push_back({str});
                idx += str.length() + 5;
                break;
            }
            case S_ERROR: {
                auto str = parse_error(cmd + idx);
                ret.push_back({str});
                idx += str.length() + 5;
                break;
            }
            case INTEGER: {
                auto str = parse_int(cmd + idx);
                ret.push_back({str});
                idx += str.length() + 5;
                break;
            }
            case B_STRING: {
                if (cmd[idx + 1] == '-' && cmd[idx + 2] == '1') {
                    return {{"(nil)"}};
                }
                auto str = parse_b_string(cmd);

                if (!str.empty()) {
                    if (client) {
                        str = "\"" + str + "\"";
                    }
                    idx += std::floor(std::log10(str.length()) + 1) + str.length() + 5;
                } else {
                    idx += 6;
                }
                ret.push_back({str});
                break;
            }
            case ARRAY:
                auto [command, len] = parse_array(cmd + idx);

                if (len == 0) {
                    return ret;
                }

                ret.push_back(command);
                idx += len;
                break;
        }
    }

    return ret;
}

std::string parser::parse_b_string(char* cmd) {
    int idx = 1;
    std::string token;
    while (!(cmd[idx] == '\r' && cmd[idx + 1] == '\n')) {
        if (cmd[idx] < 48 || cmd[idx] > 57) {
            std::cerr << "Invalid symbol --> " << std::string(cmd + idx) << '\n';
            return "";
        }
        token.push_back(cmd[idx++]);
    }

    if (token == "0") {
        return "";
    }

    // Account for \r\n
    idx += 2;

    return std::string(cmd + idx, std::stoi(token));
}

std::string parser::parse_s_string(char* cmd) {
    int idx = 1;
    std::string token;

    while (!(cmd[idx] == '\r' && cmd[idx + 1] == '\n')) {
        token.push_back(cmd[idx++]);
    }

    return token;
}

std::string parser::parse_error(char* cmd) {
    int idx           = 1;
    std::string token = "(error) ";
    while (!(cmd[idx] == '\r' && cmd[idx + 1] == '\n')) {
        token.push_back(cmd[idx++]);
    }

    return token;
}

std::string parser::parse_int(char* cmd) {
    int idx           = 1;
    std::string token = "(integer) ";
    while (!(cmd[idx] == '\r' && cmd[idx + 1] == '\n')) {
        token.push_back(cmd[idx++]);
    }

    return token;
}

std::pair<std::vector<std::string>, int> parser::parse_array(char* cmd) {
    int idx = 1;
    std::string token;
    while (!(cmd[idx] == '\r' && cmd[idx + 1] == '\n')) {
        if (cmd[idx] < 48 || cmd[idx] > 57) {
            std::cerr << "Invalid symbol --> " << std::string(cmd + idx) << '\n';
            return {};
        }
        token.push_back(cmd[idx++]);
    }

    // Account for \r\n
    idx += 2;

    int len                      = std::stoi(token);
    std::vector<std::string> ret = {};
    for (int i = 0; i < len; ++i) {

        switch (Type(cmd[idx])) {
            case S_STRING:
                token = parse_s_string(cmd + idx);
                break;
            case S_ERROR:
                token = parse_error(cmd + idx);
                break;
            case INTEGER:
                token = parse_int(cmd + idx);
                break;
            case B_STRING:
                token = parse_b_string(cmd + idx);
                // add token length number
                idx += std::floor(std::log10(token.length()) + 1);
                break;
            case ARRAY:
                // Idk... prob would need to store pointers to handle infinite array nesting
                std::cerr << "Array nesting is not supported yet!\n";
                return {{}, 0};
        }

        ret.push_back(token);

        // add token length + symbols
        idx += token.length() + 5;
    }

    return {ret, idx};
}
