#pragma once

#include <algorithm>
#include <cassert>
#include <stdint.h>

class string_t {
public:
    static const size_t capacity_min;

private:
    size_t _size = 0;
    size_t _capacity = 16;
    char *_data = new char[16];

private:
    void realloc(size_t size) {
        _size = size;

        if (size <= _capacity - 1) {
            return;
        }

        auto next_capacity = std::max(16ULL, size + 4 - (size + 4) % 4);
        auto next_data = new char[_capacity];
        std::memset(next_data, '\0', next_capacity);
        if (_data && _size) {
            std::memcpy(next_data, _data, _size);
            delete _data;
        }
        _data = next_data;
        _capacity = next_capacity;
    }

    void assign(const char *data, size_t size) {
        realloc(size);
        std::memset(_data, '\0', _capacity);
        std::memcpy(_data, data, size);
    }

public:
    string_t() {
        assign("", 0);
    };

    string_t(const char *str) {
        assert(str != nullptr);
        auto size = std::strlen(str);
        assign(str, size);
    }

    string_t(const char *data, size_t size) {
        assert(data != nullptr);
        assign(data, size);
    }

    string_t(const string_t &other) {
        if (&other == this)
            return;
        assign(other._data, other._size);
    }

    string_t(string_t &&other) noexcept
        : _data(other._data),
          _size(other._size),
          _capacity(other._capacity) {
        other._data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    void operator=(const string_t &other) {
        if (&other == this)
            return;
        assign(other._data, other._size);
    }

    string_t &append(const char *str, size_t size) {
        assert(str != nullptr);
        auto append_at = _size;
        realloc(size + _size);
        std::memcpy(_data + append_at, str, size);
    }

    string_t &append(const char *str) {
        assert(str != nullptr);
        return append(str, std::strlen(str));
    }

    void operator=(string_t &&other) {
        _data = other._data;
        _size = other._size;
        _capacity = other._capacity;
        other._data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    void clear() {
        _size = 0;
        if (_data) {
            delete _data;
        }
        _data = nullptr;
    }

    char &operator[](size_t index) {
        return _data[index];
    }

    const char *c_str() const noexcept { return _data; }
    const char *data() const noexcept { return _data; }
    size_t size() const noexcept { return _size; }
    size_t capacity() const noexcept { return _capacity; }
    bool empty() const noexcept { return _size == 0; }
};

const size_t string_t::capacity_min = 16;