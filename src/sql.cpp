#include "sql.hpp"

using std::string;

namespace zettel {

namespace sql {

void* salloc(size_t block) {
    return malloc(block);
}

void sfree(void* memory) {
    free(memory);
}

SQLite::SQLite(const char* uri) : m_handle(nullptr), m_in_transaction(false), m_valid(true), m_locked(false) {
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

void SQLite::query(string query, const sql::IParamAdapter& params) {
    checkState();
    sqlite3_stmt* statement;
    int result = sqlite3_prepare_v2(m_handle, query.c_str(), query.size(), &statement, NULL);
    if (result != SQLITE_OK) throw SQLite::Exception(fmt("Failed to compile query: %s", query.c_str()));
    result = params.bind(statement);
    if (result != SQLITE_OK) throw SQLite::Exception(fmt("Failed to bind parameters to statement: %s", params.error(result)->c_str()));
    result = sqlite3_step(statement);
    while (result != SQLITE_DONE) {
        if (result == SQLITE_ROW) continue;
        else {
            sqlite3_finalize(statement);
            const char *msg = sqlite3_errmsg(m_handle);
            throw SQLite::Exception(fmt("Error while executing query: %s", msg));
        }
        result = sqlite3_step(statement);
    }
    sqlite3_finalize(statement);
}

void SQLite::query(string query) {
    return this->query(query, sql::nullparams{});
}

void SQLite::begin() {
    checkState();
    if (m_in_transaction) throw SQLite::Exception("Already in transaction!");
    char* msg = nullptr;
    int result = sqlite3_exec(m_handle, "BEGIN TRANSACTION;", nullptr, nullptr, &msg);
    if (result != SQLITE_OK) {
        std::string errmsg(fmt("Could not begin transaction: %s", msg));
        sqlite3_free(msg);
        throw SQLite::Exception(errmsg);
    }
    m_in_transaction = true;
}

void SQLite::commit() {
    checkState();
    if (!m_in_transaction) throw SQLite::Exception("Not in transaction!");
    char* msg = nullptr;
    int result = sqlite3_exec(m_handle, "COMMIT;", nullptr, nullptr, &msg);
    if (result != SQLITE_OK) {
        std::string errmsg(fmt("Could not commit transaction: %s", msg));
        sqlite3_free(msg);
        throw SQLite::Exception(errmsg);
    }
    m_in_transaction = false;
}

void SQLite::checkState() {
    if (!m_valid) throw SQLite::Exception("Not in a valid state!");
    if (m_locked) throw SQLite::Exception("Cannot perform query when locked!");
}

}

}
