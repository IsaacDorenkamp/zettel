#include "buffer.hpp"

#include <cstring>

namespace zettel {

buffer::buffer(void* buf, size_t size) : m_buf((char*)buf), m_size(size) {}
buffer::buffer(const void* buf, size_t size) : m_buf(nullptr), m_size(size) {
    m_buf = std::unique_ptr<char[]>(new char[size]);
    memcpy(m_buf.get(), buf, size);
}

const void* buffer::buf() const {
    return (void*)m_buf.get();
}

void* buffer::buf() {
    return (void*)m_buf.get();
}

size_t buffer::size() const {
    return m_size;
}

char buffer::operator[](size_t index) const {
    return m_buf[index];
}

}
