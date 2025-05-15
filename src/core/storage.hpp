#pragma once

#include <optional>
#include <string>

namespace storage {

struct Node {
    Node* next = nullptr;
    Node* prev = nullptr;
    std::string data;

    Node(std::string str) : data(str) {};
};

class BaseStorage {
public:
    time_t expires;
    virtual void set_data(const std::string, int, bool) = 0;
    virtual std::optional<std::string> get_data(bool)   = 0;
    virtual ~BaseStorage()                              = default;
};

class ListStorage : public BaseStorage {
    Node* head = nullptr;
    Node* tail = nullptr;

    void rpush(const std::string str);
    void lpush(const std::string str);

    std::optional<std::string> rpop();
    std::optional<std::string> lpop();

public:
    ~ListStorage();

    int length = 0;

    void set_data(const std::string str, int = 0, bool right = true) override;
    std::optional<std::string> get_data(bool right = true) override;
    const Node* get_head();
};

class MapStorage : public BaseStorage {
    std::string data;

public:
    ~MapStorage();
    void set_data(const std::string str, int expires, bool = true) override;
    std::optional<std::string> get_data(bool = true) override;
};

} // namespace storage
