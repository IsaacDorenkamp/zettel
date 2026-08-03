#include "sql.hpp"

using std::string;

namespace zettel {

SQLite::SQLite(const char* uri) : m_handle(nullptr), m_valid(true), m_locked(false) {
    int status = sqlite3_open(uri, &m_handle);
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

void SQLite::query(string query) {
    checkState();
    char* err = nullptr;
    int result = sqlite3_exec(m_handle, query.c_str(), nullptr, nullptr, &err);
    if (result != SQLITE_OK) {
        throw SQLite::Exception(fmt("Error in query %s: %s", query.c_str(), err));
    }
}

void SQLite::checkState() {
    if (!m_valid) throw SQLite::Exception("Not in a valid state!");
    if (m_locked) throw SQLite::Exception("Cannot perform query when locked!");
}

}
