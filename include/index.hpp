#pragma once

#include "exception.hpp"
#include "sql.hpp"
#include "zettel.hpp"

namespace zettel {

namespace models {

typedef struct zettel {
    Zettel::Id id;
    std::string title;

    static struct zettel from(const std::vector<sqlite3_value*>& row);
} zettel;

typedef struct zettel_tag {
    std::string tag;
    Zettel::Id zettel_id;
    uint32_t id;

    static struct zettel_tag from(const std::vector<sqlite3_value*>& row);
} zettel_tag;

}

class Index {
public:
    DEFINE_EXCEPTION;

    Index(const char* db_file);
    virtual ~Index() = default;

    void begin();
    void commit();

    std::vector<models::zettel> search(std::string tag);
    void insert(const Zettel* note);
    void update(const Zettel* note);

    uint32_t nextId();
private:
    sql::SQLite m_db;
};

}
