#pragma once

#include "exception.hpp"
#include "ident.hpp"
#include "sql.hpp"
#include "zettel.hpp"

namespace zettel {

namespace models {

typedef struct zettel {
    std::string id;
    std::string title;
    std::string filename;

    static struct zettel from(const std::vector<sqlite3_value*>& row);
} zettel;

typedef struct zettel_tag {
    std::string tag;
    std::string zettel_id;
    uint32_t id;

    static struct zettel_tag from(const std::vector<sqlite3_value*>& row);
} zettel_tag;

}

class Index {
public:
    DEFINE_EXCEPTION;

    Index(const char* db_file);
    virtual ~Index() = default;

    std::vector<std::unique_ptr<Id>> search(std::string tag);
    void insert(const Zettel* note);
    void update(const Zettel* note);

    void create();
private:
    sql::SQLite m_db;
};

}
