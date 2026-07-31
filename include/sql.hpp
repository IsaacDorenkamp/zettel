#pragma once

#include <filesystem>
#include <vector>

#include <sqlite3.h>

#include "exception.hpp"
#include "format.hpp"

namespace zettel {

class SQLite {
public:
    DEFINE_EXCEPTION;

    template <typename RowType>
    class iterator {
    public:
        using RowBuilder = std::function<RowType(const std::vector<sqlite3_value*>&)>;
        explicit iterator(SQLite* owner, sqlite3_stmt* statement, RowBuilder builder) : m_owner(owner), m_statement(statement), m_columns(sqlite3_column_count(statement)), m_current(), m_builder(builder), m_done(false) {
            m_owner->m_locked = true;
        }
        virtual ~iterator() {
            m_owner->m_locked = false;
            sqlite3_finalize(m_statement);
        }
        iterator& operator++() {
            int result = sqlite3_step(m_statement);
            switch (result) {
            case SQLITE_DONE:
                m_done = true;
            case SQLITE_OK:
                break;
            default:
                throw SQLite::Exception("An error occurred fetching the next result.");
            }

            std::vector<sqlite3_value*> row;
            for (uint8_t index = 0; index < m_columns; index++) {
                row.push_back(sqlite3_column_value(m_statement, index));
            }
            m_current = m_builder(row);
            return *this;
        }
        RowType& operator*() {
            return *m_current;
        }
        bool done() const {
            return m_done;
        }
    private:
        SQLite* m_owner;
        sqlite3_stmt* m_statement;
        uint8_t m_columns;
        std::optional<RowType> m_current;
        std::function<RowType(const std::vector<sqlite3_value*>&)> m_builder;
        bool m_done;
    };

    SQLite(std::filesystem::path file);
    virtual ~SQLite();

    bool valid() const;
    void close();
    
    template <typename RowType>
    iterator<RowType> query(std::string query, typename iterator<RowType>::RowBuilder resultBuilder) {
        if (m_locked) throw SQLite::Exception("Cannot perform query when locked!");

        sqlite3_stmt* statement;
        int result = sqlite3_prepare(m_handle, query.c_str(), query.length(), &statement, NULL);
        if (result != SQLITE_OK) {
            throw SQLite::Exception(fmt("Failed to compile query: %s", query.c_str()));
        }
        iterator<RowType> it(this, statement, resultBuilder);
        it++;  // initialize iterator to first result
        return it;
    }
private:
    sqlite3* m_handle;
    bool m_valid;
    bool m_locked;
};

}
