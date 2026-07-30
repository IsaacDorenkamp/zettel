#pragma once

#include <cstddef>
#include <memory>

namespace zettel {

class Buffer {
public:
    Buffer(void* buf, size_t size);
    virtual ~Buffer() = default;

    const void* buf() const;
    void* buf();

    size_t size() const;

    char operator[](size_t index) const;
private:
    std::unique_ptr<char[]> m_buf;
    size_t m_size;
};

}
