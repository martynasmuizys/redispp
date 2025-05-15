#include "storage.hpp"
#include "log.hpp"
#include <optional>
#include <string>

using namespace storage;

// LIST STORAGE
ListStorage::~ListStorage() {
    while (length > 0) {
        Node* node = tail;

        if (length == 1) {
            head = tail = nullptr;
        } else {
            tail       = node->prev;
            tail->next = nullptr;
        }

        delete node;
        --length;
    }
}

std::optional<std::string> ListStorage::get_data(bool right) {
    if (right) {
        return rpop();
    }

    return lpop();
}

void ListStorage::set_data(const std::string str, int, bool right) {
    if (right) {
        return rpush(str);
    }

    return lpush(str);
}

const Node* ListStorage::get_head() {
    return head;
}

void ListStorage::rpush(const std::string str) {
    Node* node = new Node(str);
    ++length;

    if (head == nullptr && tail == nullptr) {
        head = tail = node;
        return;
    }

    node->prev = tail;
    tail->next = node;
    tail       = node;
}

void ListStorage::lpush(const std::string str) {
    Node* node = new Node(str);
    ++length;

    if (head == nullptr && tail == nullptr) {
        head = tail = node;
        return;
    }

    node->next = head;
    head->prev = node;
    head       = node;
}

std::optional<std::string> ListStorage::rpop() {
    if (head == nullptr && tail == nullptr) {
        return std::nullopt;
    }

    Node* node       = tail;
    std::string data = node->data;

    if (length == 1) {
        head = tail = nullptr;
    } else {
        tail       = node->prev;
        tail->next = nullptr;
    }

    delete node;
    --length;

    return data;
}

std::optional<std::string> ListStorage::lpop() {
    if (head == nullptr && tail == nullptr) {
        return std::nullopt;
    }

    Node* node       = head;
    std::string data = node->data;

    if (length == 1) {
        head = tail = nullptr;
    } else {
        head       = node->next;
        head->prev = nullptr;
    }

    delete node;
    --length;

    return data;
}

// MAP STORAGE
MapStorage::~MapStorage() {}

std::optional<std::string> MapStorage::get_data(bool) {
    return data;
}

void MapStorage::set_data(const std::string str, int expires, bool) {
    data          = str;
    this->expires = expires;
}
