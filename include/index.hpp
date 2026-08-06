#pragma once

#include "buffer.hpp"
#include "ident.hpp"
#include "sql.hpp"
#include "zettel.hpp"

namespace zettel {

namespace models {

struct tag {
    std::string tag;
    uint32_t id;

    static struct tag from(const std::vector<sqlite3_value*>& row);
};

struct zettel {
    Buffer note_id;
    std::string title;
    std::string filename;
    uint32_t id;

    static struct zettel from(const std::vector<sqlite3_value*>& row);
};

struct zettel_tag {
    uint32_t tag_id;
    uint32_t zettel_id;
    uint32_t id;

    static struct zettel_tag from(const std::vector<sqlite3_value*>& row);
};

}

class Index {
public:
    Index(const char* db_file);
    virtual ~Index() = default;

    std::vector<std::unique_ptr<Id>> search(std::string tag);
    void insert(const Zettel* note);
    void update(const Zettel* note);
private:
    SQLite m_db;
};

}
