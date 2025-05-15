#include <cstring>
#include <format>
#include <iostream>
#include <string>

namespace log {

void error(const std::string& err) {
    std::cerr << std::format("error: {}: {}\n", err, strerror(errno));
};

template<typename T> void log(const T& msg) {
    std::cout << std::format("log: {}\n", msg);
};

} // namespace log
