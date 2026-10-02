#pragma once

#include <cstring>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

#include <sqlite3.h>

#include "buffer.hpp"
#include "exception.hpp"
#include "format.hpp"

namespace zettel {

namespace sql {

void* salloc(size_t block);
void sfree(void* ptr);

class IParamAdapter {
public:
    virtual size_t length() const = 0;
    virtual int bind(sqlite3_stmt* statement) const = 0;
    virtual std::optional<std::string> error(int bindResult) const = 0;
};

class NullParamAdapter : public IParamAdapter {
public:
    virtual size_t length() const { return 0; }
    virtual int bind(sqlite3_stmt*) const { return SQLITE_OK; }
    virtual std::optional<std::string> error(int bindResult) const { return nullptr; }
};

template <typename T>
class ParamAdapter;

typedef std::variant<buffer, std::string, double, int, std::nullptr_t> value;

template <>
class ParamAdapter<std::initializer_list<value>> : public IParamAdapter {
public:
    ParamAdapter(const std::initializer_list<value>& params) : m_params(params) {}

    virtual size_t length() const {
        return m_params.size();
    }

    virtual int bind(sqlite3_stmt* statement) const {
        size_t args = sqlite3_bind_parameter_count(statement);
        if (args != m_params.size()) return -1;

        // NOTE: param index starts at 1, not 0!
        int index = 1;
        int ok;
        for (const value& value : m_params) {
            ok = std::visit([statement, index](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, buffer>) {
                    void* newbuf = salloc(arg.size());
                    memcpy(newbuf, arg.buf(), arg.size());
                    return sqlite3_bind_blob(statement, index, newbuf, arg.size(), &sfree);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    void* newbuf = salloc(arg.size());
                    memcpy(newbuf, arg.c_str(), arg.size());
                    return sqlite3_bind_text(statement, index, (const char*)newbuf, arg.size(), &sfree);
                } else if constexpr (std::is_same_v<T, double>) {
                    return sqlite3_bind_double(statement, index, arg);
                } else if constexpr (std::is_same_v<T, int>) {
                    return sqlite3_bind_int(statement, index, arg);
                } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    return sqlite3_bind_null(statement, index);
                }
            }, value);
            if (ok != SQLITE_OK) return ok;
            index++;
        }
        return SQLITE_OK;
    }

    virtual std::optional<std::string> error(int bindResult) const {
        if (bindResult == -1) {
            return "Mismatch between query variables and number of parameters in list";
        } else if (bindResult == SQLITE_OK) {
            return std::nullopt;
        } else {
            return sqlite3_errstr(bindResult);
        }
    }
private:
    std::initializer_list<value> m_params;
};

typedef ParamAdapter<std::initializer_list<value>> paramlist;

template <>
class ParamAdapter<std::initializer_list<std::pair<const std::string, value>>> : public IParamAdapter {
public:
    ParamAdapter(std::initializer_list<std::pair<const std::string, value>>& params) : m_names(), m_params(std::move(params)) {
        for (const std::pair<const std::string, value>& pair : params) m_names.push_back(pair.first);
    }

    virtual size_t length() const {
        return m_params.size();
    }

    virtual int bind(sqlite3_stmt* statement) const {
        size_t args = sqlite3_bind_parameter_count(statement);
        if (args != m_params.size()) return -1;

        int ok;
        const char* paramname = nullptr;
        std::map<std::string, value>::const_iterator result = m_params.end();
        for (int index = 1; index <= args; index++) {
            paramname = sqlite3_bind_parameter_name(statement, index);
            if (!paramname) return 0x80000000 | index;
            result = m_params.find(paramname);
            if (result == m_params.end()) return 0xC0000000 | index;
            ok = std::visit([statement, index](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, buffer>) {
                    void* newbuf = salloc(arg.size());
                    memcpy(newbuf, arg.buf(), arg.size());
                    return sqlite3_bind_blob(statement, index, newbuf, arg.size(), &sfree);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    void* newbuf = salloc(arg.size());
                    memcpy(newbuf, arg.c_str(), arg.size());
                    return sqlite3_bind_text(statement, index, (const char*)newbuf, arg.size(), &sfree);
                } else if constexpr (std::is_same_v<T, double>) {
                    return sqlite3_bind_double(statement, index, arg);
                } else if constexpr (std::is_same_v<T, int>) {
                    return sqlite3_bind_int(statement, index, arg);
                } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    return sqlite3_bind_null(statement, index);
                }
            }, result->second);
            if (ok != SQLITE_OK) return ok;
        }
        return SQLITE_OK;
    }

    virtual std::optional<std::string> error(int bindResult) const {
        if (bindResult > 0) return sqlite3_errstr(bindResult);
        else if (bindResult == -1) {
            return "Mismatch between query variables and number of parameters in list";
        } else if ((bindResult & 0x80000000) == 0x80000000) {
            // missing param name
            return fmt("Unnamed parameter at index %d", bindResult ^ 0x80000000);
        } else if ((bindResult & 0xC0000000) == 0xC0000000) {
            return fmt("Missing named parameter at index %d", bindResult ^ 0xC0000000);
        } else {
            return std::nullopt;
        }
    }
private:
    std::vector<std::string> m_names;
    std::map<std::string, value> m_params;
};

typedef ParamAdapter<std::initializer_list<std::pair<std::string, value>>> paramdict;

}

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
        iterator(const iterator<RowType>& other) = delete;
        iterator(iterator<RowType>&& other) : m_owner(other.m_owner), m_statement(other.m_statement), m_columns(other.m_columns), m_current(std::move(other.m_current)), m_builder(other.m_builder) {
            other.m_owner = nullptr;
            other.m_statement = nullptr;
            other.m_columns = 0;
            other.m_current = std::nullopt;
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
            m_current.emplace(std::move(m_builder(row)));

            return *this;
        }
        RowType& operator*() {
            return *m_current;
        }
        RowType* operator->() {
            return &m_current.value();
        }
        bool done() const {
            return m_statement == nullptr;
        }
    private:
        void finalize() {
            m_owner->m_locked = false;
            sqlite3_finalize(m_statement);
            m_current = std::nullopt;
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
    iterator<RowType> query(std::string query, typename iterator<RowType>::RowBuilder resultBuilder, const sql::IParamAdapter& params) {
        checkState();

        sqlite3_stmt* statement;
        int result = sqlite3_prepare_v2(m_handle, query.c_str(), query.size(), &statement, NULL);
        if (result != SQLITE_OK) throw SQLite::Exception(fmt("Failed to compile query: %s", query.c_str()));
        result = params.bind(statement);
        if (result != SQLITE_OK) throw SQLite::Exception(fmt("Unable to bind parameters to statement: %s", params.error(result)));

        iterator<RowType> it(this, statement, resultBuilder);
        ++it;  // initialize iterator to first result
        return it;
    }

    template <typename RowType>
    iterator<RowType> query(std::string query, typename iterator<RowType>::RowBuilder resultBuilder) {
        return this->query<RowType>(query, resultBuilder, sql::NullParamAdapter{});
    }

    template <typename RowType, std::enable_if_t<
        std::is_invocable_r_v<RowType, decltype(RowType::from), const std::vector<sqlite3_value*>&>,
        bool
    > = true>
    iterator<RowType> query(std::string query, const sql::IParamAdapter& params) {
        return this->query(query, RowType::from, params);
    }

    template <typename RowType, std::enable_if_t<
        std::is_invocable_r_v<RowType, decltype(RowType::from), const std::vector<sqlite3_value*>&>,
        bool
    > = true>
    iterator<RowType> query(std::string query) {
        return this->query<RowType>(query, RowType::from);
    }

    void query(std::string query);
    void query(std::string, const sql::IParamAdapter& params);

    void begin();
    void commit();
private:
    sqlite3* m_handle;
    bool m_in_transaction;
    bool m_valid;
    bool m_locked;

    void checkState();
};

}
