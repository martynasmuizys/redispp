#include "core/commands.hpp"
#include "core/log.hpp"
#include "core/parser.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace commands;
using namespace log;

int main(int argc, char* argv[]) {
    if (load_data() < 0) {
        error("load_data()");
    }

    addrinfo hints = {}, *res_raw = nullptr;
    std::vector<pollfd> pfds;

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(nullptr, "6379", &hints, &res_raw)) != 0) {
        std::cerr << std::format("error: getaddrinfo(): {}\n", gai_strerror(status));
        return 2;
    }

    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> res(res_raw, freeaddrinfo);

    int sock;
    if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        error("socket()");
        return 1;
    }
    pfds.push_back({sock, POLLIN, 0});

    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        error("fcntl()");
        return 1;
    }

    int val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
        error("setsockopt()");
        return 1;
    };

    if (bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
        error("bind()");
        return 1;
    }

    if (listen(sock, 20) < 0) {
        error("listen()");
        return 1;
    }

    int conn;
    sockaddr_storage c_addr;
    std::array<char, 4096> buf = {};

    std::thread(monitor).detach();

    std::signal(SIGINT, [](int) {
        exit(0);
    });
    while (true) {
        int poll_events = poll(pfds.data(), pfds.size(), -1);

        if (poll_events == -1) {
            error("poll()");
            return 1;
        }

        for (size_t i = 0; i < pfds.size(); ++i) {
            if (pfds[i].revents & (POLLIN | POLLHUP)) {
                if (pfds[i].fd == sock) {
                    socklen_t c_addr_len = sizeof(c_addr);
                    if ((conn = accept(sock, reinterpret_cast<sockaddr*>(&c_addr), &c_addr_len)) <
                        0) {
                        error("accept()");
                        return 1;
                    }

                    pfds.push_back({conn, POLLIN, 0});
                } else {
                    int nbytes = recv(pfds[i].fd, buf.data(), buf.size(), 0);

                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                        } else {
                            error("recv()");
                        }
                        close(pfds[i].fd);

                        auto idx = pfds.begin() + i;
                        pfds.erase(idx, idx + 1);
                        --i;
                    } else {
                        auto cmds = parser::deserialize(buf.data(), nbytes);

                        for (auto& args : cmds) {
                            std::transform(args[0].begin(), args[0].end(), args[0].begin(),
                                           [](unsigned char c) {
                                               return std::tolower(c);
                                           });

                            if (COMMANDS.contains(args[0])) {
                                auto func = COMMANDS.find(args[0])->second;
                                func(pfds[i].fd, args);
                            } else {
                                send_error(pfds[i].fd, "Unknown command");
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
