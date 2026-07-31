#include "sql.hpp"

namespace zettel {

SQLite::SQLite(std::filesystem::path file) : m_handle(nullptr), m_valid(true), m_locked(false) {
    int status = sqlite3_open(file.c_str(), &m_handle);
    m_valid = status == SQLITE_OK;
}

SQLite::~SQLite() {
    if (m_handle != nullptr) sqlite3_close(m_handle);
}

bool SQLite::valid() const {
    return (m_handle != nullptr) && m_valid;
}

void SQLite::close() {
    if (m_handle) {
        sqlite3_close(m_handle);
        m_handle = nullptr;
    }
}

}
