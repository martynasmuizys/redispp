#include "core/log.cpp"
#include "core/parser.hpp"
#include <array>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define HELP_TEXT                                                                                  \
    "Sisyphus Redis Client 0.1.0\n"                                                               \
    "Available commands:\n"                                                                        \
    "   - ping: check if Redis host is reachable.\n"                                                 \
    "   - echo: send back message the server received.\n"                                            \
    "   - get <key>: request data for <key> value.\n"                                                \
    "   - set <key> <data>: set <data> for <key> value.\n"                                           \
    "   - del <key>: delete <key> entry.\n"                                                          \
    "   - exists <key>: check if <key> is present.\n"                                                \
    "   - exit: close connection and exit\n"

using namespace log;

class Redis {
private:
    int sock;
    std::string err;
    std::string_view host;

public:
    Redis(const std::string_view host) {
        this->host = host;
    }

    ~Redis() {
        close(sock);
    }

    int start() {
        addrinfo hints = {}, *res_raw = nullptr;

        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int status;
        if ((status = getaddrinfo(host.data(), "6379", &hints, &res_raw)) != 0) {
            err = std::format("error: getaddrinfo(): {}\n", gai_strerror(status));
            return -1;
        }

        std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> res(res_raw, freeaddrinfo);

        if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            error("socket()");
            return -1;
        }

        if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
            error("connect()");
            return -1;
        }

        return 0;
    }

    const std::string& get_error() {
        return err;
    }

    void get_help() {
        std::cout << HELP_TEXT;
    }

    int send_cmd(const std::vector<std::string>& args) {
        if (args.empty()) {
            return 0;
        }
        auto message = parser::serialize(args, parser::Type::ARRAY);
        size_t bytes_sent = 0;

        while (bytes_sent < message.size()) {
            int nbytes = send(sock, message.data(), message.size(), 0);

            if (nbytes < 0) {
                error("send()");
                return -1;
            }
            bytes_sent += nbytes;
        }

        std::array<char, 4096> buf = {};
        // for (int i = 0; i < message.length(); ++i) {
        //     buf[i] = message[i];
        // }
        int nbytes = recv(sock, buf.data(), buf.size(), 0);

        if (nbytes < 0) {
            error("recv()");
            return -1;
        }

        auto resp = parser::deserialize(buf.data(), nbytes, true);
        for (auto r : resp[0]) {
            std::cout << r << '\n';
        }

        return 0;
    }
};

int main(int argc, char* argv[]) {

    std::string_view host = "127.0.0.1";
    if (argc > 1) {
        host = argv[1];
    }

    Redis redis(host);

    if (redis.start() < 0) {
        std::cerr << redis.get_error();
        return 1;
    }

    std::string line;

    std::cout << host << ":6379> ";

    std::signal(SIGINT, [](int) {
        exit(0);
    });
    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }

        if (line == "?" || line == "help") {
            redis.get_help();
        } else {
            std::string word;
            std::stringstream ss(line);
            std::vector<std::string> args;

            while (ss >> word) {
                args.push_back(word);
            }

            redis.send_cmd(args);
        }

        std::cout << host << ":6379> ";
    }

    return 0;
}
