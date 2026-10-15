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

SQLite::SQLite(const char* uri) : m_uri(uri), m_handle(nullptr), m_transaction_layers(0), m_valid(true), m_locked(false) {
    int status = sqlite3_open(uri, &m_handle);
    m_valid = status == SQLITE_OK;
    if (m_valid) sqlite3_exec(m_handle, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
}

SQLite::~SQLite() {
    if (m_handle != nullptr) sqlite3_close(m_handle);
}

bool SQLite::valid() const {
    return (m_handle != nullptr) && m_valid;
}

SQLite::ConnectResult SQLite::connect() {
    int status;
    if (!m_handle) {
        status = sqlite3_open(m_uri.c_str(), &m_handle);
        m_valid = status == SQLITE_OK;
        if (m_valid) {
            sqlite3_exec(m_handle, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
            return SQLite::ConnectResult::Success;
        } else {
            return SQLite::ConnectResult::Failure;
        }
    } else {
        if (!m_valid) {
            sqlite3_close(m_handle);
            status = sqlite3_open(m_uri.c_str(), &m_handle);
            m_valid = status == SQLITE_OK;
            if (m_valid) {
                sqlite3_exec(m_handle, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
                return SQLite::ConnectResult::Success;
            } else {
                return SQLite::ConnectResult::Failure;
            }
        } else {
            return SQLite::ConnectResult::Connected;
        }
    }
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
    if (result != SQLITE_OK) throw SQLite::Exception(fmt("Failed to compile query '%s': %s", query.c_str(), sqlite3_errmsg(m_handle)));
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
    int result;
    char* msg = nullptr;
    if (m_transaction_layers == 0) result = sqlite3_exec(m_handle, "BEGIN TRANSACTION", nullptr, nullptr, &msg);
    else result = sqlite3_exec(m_handle, fmt("SAVEPOINT layer_%d", m_transaction_layers).c_str(), nullptr, nullptr, &msg);
    if (result != SQLITE_OK) {
        std::string errmsg(fmt("Could not begin transaction: %s", msg));
        sqlite3_free(msg);
        throw SQLite::Exception(errmsg);
    }
    m_transaction_layers++;
}

void SQLite::rollback() {
    checkState();
    if (m_transaction_layers == 0) throw SQLite::Exception("Not in transaction!");
    int result;
    char* msg = nullptr;
    if (m_transaction_layers == 1) result = sqlite3_exec(m_handle, "ROLLBACK", nullptr, nullptr, &msg);
    else result = sqlite3_exec(m_handle, fmt("ROLLBACK TO SAVEPOINT layer_%d", m_transaction_layers - 1).c_str(), nullptr, nullptr, &msg);
    if (result != SQLITE_OK) {
        std::string errmsg(fmt("Could not commit transaction: %s", msg));
        sqlite3_free(msg);
        throw SQLite::Exception(errmsg);
    }
    m_transaction_layers--;
}

void SQLite::commit() {
    checkState();
    if (!m_transaction_layers) throw SQLite::Exception("Not in transaction!");
    char* msg = nullptr;
    int result;
    if (m_transaction_layers == 1) result = sqlite3_exec(m_handle, "COMMIT", nullptr, nullptr, &msg);
    else result = sqlite3_exec(m_handle, fmt("RELEASE SAVEPOINT layer_%d", m_transaction_layers - 1).c_str(), nullptr, nullptr, &msg);
    if (result != SQLITE_OK) {
        std::string errmsg(fmt("Could not commit transaction: %s", msg));
        sqlite3_free(msg);
        throw SQLite::Exception(errmsg);
    }
    m_transaction_layers--;
}

const string& SQLite::uri() const {
    return m_uri;
}

void SQLite::checkState() {
    if (!m_valid) throw SQLite::Exception("Not in a valid state!");
    if (m_locked) throw SQLite::Exception("Cannot perform query when locked!");
}

}

}
