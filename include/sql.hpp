#pragma once

#include <functional>
#include <optional>
#include <type_traits>
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
        explicit iterator(SQLite* owner, sqlite3_stmt* statement, RowBuilder builder) : m_owner(owner), m_statement(statement), m_columns(sqlite3_column_count(statement)), m_current(), m_builder(builder) {
            m_owner->m_locked = true;
        }
        virtual ~iterator() {
            if (m_statement) finalize();
        }
        iterator& operator++() {
            int result = sqlite3_step(m_statement);
            switch (result) {
            case SQLITE_DONE:
                finalize();
                return *this;
            case SQLITE_OK:
            case SQLITE_ROW:
                break;
            default:
                const char* message = sqlite3_errmsg(m_owner->m_handle);
                if (message == nullptr) message = "Unknown error";
                throw SQLite::Exception(fmt("An error occurred fetching the next result: %s", message));
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
            return m_statement == nullptr;
        }
    private:
        void finalize() {
            m_owner->m_locked = false;
            sqlite3_finalize(m_statement);
            m_current = std::optional<RowType>();
            m_statement = nullptr;
        }

        SQLite* m_owner;
        sqlite3_stmt* m_statement;
        uint8_t m_columns;
        std::optional<RowType> m_current;
        std::function<RowType(const std::vector<sqlite3_value*>&)> m_builder;
    };

    SQLite(const char* uri);
    virtual ~SQLite();

    bool valid() const;
    void close();
    
    template <typename RowType>
    iterator<RowType> query(std::string query, typename iterator<RowType>::RowBuilder resultBuilder) {
        checkState();

        sqlite3_stmt* statement;
        int result = sqlite3_prepare(m_handle, query.c_str(), query.size(), &statement, NULL);
        if (result != SQLITE_OK) {
            throw SQLite::Exception(fmt("Failed to compile query: %s", query.c_str()));
        }
        iterator<RowType> it(this, statement, resultBuilder);
        ++it;  // initialize iterator to first result
        return it;
    }

    template <typename RowType, std::enable_if_t<
        std::is_invocable_r_v<RowType, decltype(RowType::from), const std::vector<sqlite3_value*>&>,
        bool
    > = true>
    iterator<RowType> query(std::string query) {
        return this->query<RowType>(query, RowType::from);
    }

    void query(std::string query);
private:
    sqlite3* m_handle;
    bool m_valid;
    bool m_locked;

    void checkState();
};

}
