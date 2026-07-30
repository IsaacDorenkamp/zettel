#include "buffer.hpp"

namespace zettel {

Buffer::Buffer(void* buf, size_t size) : m_buf((char*)buf), m_size(size) {}

const void* Buffer::buf() const {
    return (void*)m_buf.get();
}

void* Buffer::buf() {
    return (void*)m_buf.get();
}

size_t Buffer::size() const {
    return m_size;
}

char Buffer::operator[](size_t index) const {
    return m_buf[index];
}

}
