#pragma once

#include <memory>
#include <set>
#include <vector>

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

typedef struct reference {
    Zettel::Id source;
    Zettel::Id dest;
    bool valid;

    static struct reference from(const std::vector<sqlite3_value*>& row);
} reference;

}

class Index {
public:
    DEFINE_EXCEPTION;

    Index(const char* db_file);
    Index(const std::shared_ptr<sql::SQLite>& db);
    virtual ~Index() = default;

    void begin();
    void commit();
    void rollback();

    bool exists(const Zettel::Id& id);
    std::map<Zettel::Id, models::zettel> lookup(std::set<Zettel::Id> ids); 
    std::set<std::string> getTagsFor(const Zettel::Id& id);
    std::set<Zettel::Id> getReferencesFor(const Zettel::Id& id, bool validOnly = true);
    std::set<Zettel::Id> getReferencesTo(const Zettel::Id& id, bool validOnly = true);

    std::vector<models::zettel> list();
    std::vector<models::zettel> searchTag(std::string tag);
    std::vector<models::zettel> searchTitle(std::string title);
    std::vector<models::zettel> searchFullText(std::string term, const std::filesystem::path& root);
    void insert(const Zettel* note);
    void update(const Zettel* note);
    void remove(const Zettel* note);

    Zettel::Id nextId();
private:
    void syncReferences(const Zettel* z);

    std::shared_ptr<sql::SQLite> m_db;
};

}
