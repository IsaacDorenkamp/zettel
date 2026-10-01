#pragma once

#include <cstddef>
#include <memory>

namespace zettel {

class buffer {
public:
    buffer(void* buf, size_t size);
    buffer(const void* buf, size_t size);
    buffer(buffer&& other) : m_buf(std::move(other.m_buf)), m_size(other.m_size) {
        other.m_size = 0;
    }
    virtual ~buffer() = default;

    const void* buf() const;
    void* buf();

    size_t size() const;

    char operator[](size_t index) const;
private:
    std::unique_ptr<char[]> m_buf;
    size_t m_size;
};

}
