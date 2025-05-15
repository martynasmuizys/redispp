#include "commands.hpp"
#include "log.hpp"
#include "parser.hpp"
#include "storage.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace commands;
using namespace storage;
using namespace parser;

std::unordered_map<std::string, std::shared_ptr<BaseStorage>> store;
std::unordered_map<std::string, std::string> conf = {{"save", "3600 1 300 1 60 10000"},
                                                     {"appendonly", "no"}};

void commands::send_error(int fd, const std::string_view e) {
    const std::string err = std::format("-Error: {}\r\n", e);
    if (::send(fd, err.data(), err.length(), 0) < 0) {
        log::error("send()");
    }
}

void commands::send(int fd, const std::string_view message) {
    if (::send(fd, message.data(), message.length(), 0) < 0) {
        log::error("send()");
    }
}

void commands::ping(int fd, const std::vector<std::string>&) {
    send(fd, "+PONG\r\n");
}
void commands::echo(int fd, const std::vector<std::string>& args) {
    std::string message;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) {
            message.push_back(' ');
        }

        message.append(args[i]);
    }
    message = serialize({message});
    send(fd, message);
}
void commands::get(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (store.contains(args[1])) {
        // std::unique_ptr<MapStorage> s = store.find(args[1])->second;
        std::weak_ptr<MapStorage> s =
            std::dynamic_pointer_cast<MapStorage>(store.find(args[1])->second);

        if (s.expired()) {
            send_error(fd, "Stored value for '" + args[1] + "' is of different type");
            return;
        } else {
            message = serialize({s.lock()->get_data().value_or("(empty)")});
        }

    } else {
        message = "$-1\r\n";
    }
    send(fd, message);
}
void commands::set(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 3) {
        send_error(fd, "No <value> provided");
        return;
    }

    time_t expires = 0;
    if (args.size() > 3) {
        time_t current_time;
        time(&current_time);
        std::string cmd = args[args.size() - 2];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        try {
            if (cmd == "ex") {
                expires = current_time + std::stoi(args.back());
            } else if (cmd == "px") {
                expires = current_time + std::stoi(args.back()) * 1000;
            } else if (cmd == "exat") {
                expires = std::stoi(args.back());
            } else if (cmd == "pxat") {
                expires = std::stoi(args.back()) * 1000;
            } else {
                send_error(fd, "Syntax error");
            }
        } catch (...) {
            send_error(fd, "Invalid expiry value");
        }
    }

    std::shared_ptr<MapStorage> s;
    if (!store.contains(args[1])) {
        store[args[1]] = s = std::make_shared<MapStorage>();
    } else {
        s = std::dynamic_pointer_cast<MapStorage>(store.find(args[1])->second);
    }

    if (s == nullptr) {
        send_error(fd, "Stored value for '" + args[1] + "' is of different type");
        return;
    }

    s->set_data(args[2], expires);
    message = "+OK\r\n";
    send(fd, message);
}

void commands::del(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 2) {
        send_error(fd, "No <key> provided");
        return;
    }

    // Clear memory
    // delete store.find(args[1])->second;

    store.erase(args[1]);
    message = "+OK\r\n";
    send(fd, message);
}
void commands::incr(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 2) {
        send_error(fd, "No <key> provided");
        return;
    }
    std::shared_ptr<MapStorage> s =
        std::dynamic_pointer_cast<MapStorage>(store.find(args[1])->second);

    if (s == nullptr) {
        send_error(fd, "Stored value is not of type 'MapStorage'");
        return;
    }

    try {
        int val = std::stoi(s->get_data().value_or("0"));
        s->set_data(std::to_string(++val), s->expires);
        message = ":" + std::to_string(val) + "\r\n";
        send(fd, message);
    } catch (...) {
        send_error(fd, "Stored value is not of type 'integer'");
    }
}
void commands::decr(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 2) {
        send_error(fd, "No <key> provided");
        return;
    }

    std::shared_ptr<MapStorage> s =
        std::dynamic_pointer_cast<MapStorage>(store.find(args[1])->second);

    if (s == nullptr) {
        send_error(fd, "Stored value is not of type 'MapStorage'");
        return;
    }

    try {
        int val = std::stoi(s->get_data().value_or("0"));
        s->set_data(std::to_string(--val), s->expires);
        message = ":" + std::to_string(val) + "\r\n";
        send(fd, message);
    } catch (...) {
        send_error(fd, "Stored value is not of type 'integer'");
    }
}
void commands::lpush(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 3) {
        send_error(fd, "No <value> provided");
        return;
    }

    std::shared_ptr<ListStorage> s;
    if (!store.contains(args[1])) {
        store[args[1]] = s = std::make_shared<ListStorage>();
    } else {
        s = std::dynamic_pointer_cast<ListStorage>(store.find(args[1])->second);
    }

    if (s == nullptr) {
        send_error(fd, "Stored value for '" + args[1] + "' is of different type");
        return;
    }

    for (size_t i = 2; i < args.size(); ++i) {
        s->set_data(args[i], 0, false);
    }
    message = "+OK\r\n";
    send(fd, message);
}
void commands::rpush(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 3) {
        send_error(fd, "No <value> provided");
        return;
    }

    std::shared_ptr<ListStorage> s;
    if (!store.contains(args[1])) {
        store[args[1]] = s = std::make_shared<ListStorage>();
    } else {
        s = std::dynamic_pointer_cast<ListStorage>(store.find(args[1])->second);
    }

    if (s == nullptr) {
        send_error(fd, "Stored value for '" + args[1] + "' is of different type");
        return;
    }

    for (size_t i = 2; i < args.size(); ++i) {
        s->set_data(args[i]);
    }
    message = "+OK\r\n";
    send(fd, message);
}
void commands::lpop(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 2) {
        send_error(fd, "No <value> provided");
        return;
    }

    std::shared_ptr<ListStorage> s;
    if (!store.contains(args[1])) {
        store[args[1]] = s = std::make_shared<ListStorage>();
    } else {
        s = std::dynamic_pointer_cast<ListStorage>(store.find(args[1])->second);
    }

    int n = 1;
    if (args.size() > 2) {
        try {
            n = std::stoi(args[2]);
        } catch (...) {
            send_error(fd, "Syntax error");
        }
    }

    std::vector<std::string> ret;
    while (n > 0) {
        ret.push_back(s->get_data(false).value());
        --n;
    }
    message = serialize(ret);
    send(fd, message);
}
void commands::rpop(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 2) {
        send_error(fd, "No <value> provided");
        return;
    }

    std::shared_ptr<ListStorage> s =
        std::dynamic_pointer_cast<ListStorage>(store.find(args[1])->second);

    if (s == nullptr) {
        message = serialize({});
        send(fd, message);
        return;
    }

    int n = 1;
    if (args.size() > 2) {
        try {
            n = std::stoi(args[2]);
        } catch (...) {
            send_error(fd, "Syntax error");
        }
    }

    std::vector<std::string> ret;
    while (n > 0) {
        ret.push_back(s->get_data().value_or("(nil)"));
        --n;
    }
    message = serialize(ret);
    send(fd, message);
}
void commands::exists(int fd, const std::vector<std::string>& args) {
    std::string message;
    if (args.size() < 2) {
        send_error(fd, "No <key> provided");
        return;
    }
    message = ":0\r\n";
    if (store.contains(args[1])) {
        message = ":1\r\n";
    }
    send(fd, message);
}
void commands::save(int fd, const std::vector<std::string>&) {
    std::string message;
    std::ofstream out("memory.save");
    out.clear();

    for (auto [k, v] : store) {
        std::weak_ptr<MapStorage> m = std::dynamic_pointer_cast<MapStorage>(store.find(k)->second);

        if (!m.expired()) {
            out << '#' << k << ',' << m.lock()->get_data().value() << ',' << m.lock()->expires
                << '\n';
            continue;
        }

        std::weak_ptr<ListStorage> l =
            std::dynamic_pointer_cast<ListStorage>(store.find(k)->second);

        if (!l.expired() && l.lock()->get_head() != nullptr) {
            out << '*' << k;
            auto node = l.lock()->get_head();
            while (node != nullptr) {
                out << ',' << node->data;
                node = node->next;
            }
            out << '\n';
        }
    }

    out.close();
    message = "+DATA SAVED\r\n";
    send(fd, message);
}

void commands::config(int fd, const std::vector<std::string>& args) {
    std::string message;

    if (args[1] == "get") {
        for (size_t i = 2; i < args.size(); ++i) {
            if (i == 3) {
                message.push_back('\n');
            }
            message.append(args[i] + '\n' + conf[args[i]]);
        }

        send(fd, serialize({message}, parser::S_STRING));
        return;
    }

    if (args[1] == "set") {
        std::string opt = args[2];
        std::string val;

        for (const auto& v : std::vector(args.begin() + 3, args.end())) {
            val += v + ' ';
        }

        if (!conf.contains(opt)) {
            send_error(fd, "Unknown config option");
            return;
        }

        if (opt == "appendonly") {
            if (val != "yes" || val != "no") {
                send_error(fd, "Config SET failed (possibly related to argument 'appendonly') - "
                               "Argument must be 'yes' or 'no'");
                return;
            }

            conf[opt] = val;
            goto END;
        }

        if (opt == "save") {
            std::stringstream ss(val);

            std::string line;
            while (std::getline(ss, line, ' ')) {
                try {
                    std::stoi(line);
                } catch (...) {
                    send_error(fd, "Config SET failed (possibly related to argument 'save') - "
                                   "Invalid save parameters");
                    return;
                }
            }

            conf[opt] = val;
            goto END;
        }

    END:
        send(fd, "+OK\r\n");
        return;
    }

    send_error(fd, "Unknown config command");
}

int commands::load_data() {
    std::ifstream in("memory.save");

    std::string line;
    while (std::getline(in, line)) {

        char type = line[0];
        line      = std::string(line.begin() + 1, line.end());
        std::stringstream ss(line);

        if (type == '#') {
            std::string key;
            std::string data;
            time_t expires;
            int idx = 0;
            while (std::getline(ss, line, ',')) {
                switch (idx) {
                    case 0:
                        key = line;
                        break;
                    case 1:
                        data = line;
                        break;
                    case 2:
                        try {
                            expires = std::stoi(line);
                        } catch (...) {
                            store.clear();
                            return 1;
                        }
                        break;
                }
                ++idx;
                std::shared_ptr<MapStorage> m = std::make_shared<MapStorage>();
                m->set_data(data, expires);
                store[key] = m;
            }
        } else if (type == '*') {
            std::shared_ptr<ListStorage> l = std::make_shared<ListStorage>();
            std::string key;
            std::string data;
            int idx = 0;
            while (std::getline(ss, line, ',')) {
                switch (idx) {
                    case 0:
                        key = line;
                        break;
                    default:
                        data = line;
                        break;
                }
                ++idx;
                l->set_data(data);
            }
            store[key] = l;
        }
    }
    in.close();

    return 0;
}

void commands::monitor() {
    while (true) {
        time_t current_time;
        time(&current_time);

        // find keys that needs to be deleted
        std::vector<std::string> keys_to_delete = {};
        for (auto it = store.begin(); it != store.end(); ++it) {
            if (it->second->expires != 0 && it->second->expires < current_time) {
                keys_to_delete.push_back(it->first);
            }
        }

        // delete keys
        for (const auto& k : keys_to_delete) {
            store.erase(k);
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
