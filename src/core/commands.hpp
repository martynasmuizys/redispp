#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace commands {

enum Command {
    HELP,
    PING,
    ECHO,
    GET,
    SET,
    DEL,
    INCR,
    DECR,
    LPUSH,
    RPUSH,
    LPOP,
    RPOP,
    EXISTS,
    SAVE
};

void send_error(int fd, const std::string_view e);
void send(int fd, const std::string_view data);

void ping(int fd, const std::vector<std::string>&);
void echo(int fd, const std::vector<std::string>& args);
void get(int fd, const std::vector<std::string>& args);
void set(int fd, const std::vector<std::string>& args);
void del(int fd, const std::vector<std::string>& args);
void incr(int fd, const std::vector<std::string>& args);
void decr(int fd, const std::vector<std::string>& args);
void lpush(int fd, const std::vector<std::string>& args);
void rpush(int fd, const std::vector<std::string>& args);
void lpop(int fd, const std::vector<std::string>& args);
void rpop(int fd, const std::vector<std::string>& args);
void exists(int fd, const std::vector<std::string>& args);
void save(int fd, const std::vector<std::string>&);
void config(int fd, const std::vector<std::string>& args);

int load_data();
void monitor();

// works the same, better performance but something is different idk
using Func = void (*)(int, const std::vector<std::string>&);

// no need to write &get -> automatically decays to a pointer
const std::unordered_map<std::string, Func> COMMANDS = {
    {"ping", ping}, {"echo", echo},     {"get", get},     {"set", set},      {"del", del},
    {"incr", incr}, {"decr", decr},     {"lpush", lpush}, {"rpush", rpush},  {"lpop", lpop},
    {"rpop", rpop}, {"exists", exists}, {"save", save},   {"config", config}};
} // namespace commands
// typedef std::function<void(int, const std::vector<std::string>&)> Func;
//
// const std::unordered_map<std::string, Func> COMMANDS = {
//     {"help", &help}, {"ping", &ping}, {"echo", &echo},     {"get", &get},
//     {"set", &set},   {"del", &del},   {"exists", &exists}, {"save", &save}};
