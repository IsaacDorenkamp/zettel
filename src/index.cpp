#include "index.hpp"

#include <string>

#include "logging.hpp"

using std::string, std::unique_ptr, std::vector;

namespace zettel {

namespace models {

struct tag tag::from(const vector<sqlite3_value*>& row) {
    return {
        (const char*)sqlite3_value_text(row[0]),
        (uint32_t)sqlite3_value_int(row[1]),
    };
}

struct zettel zettel::from(const vector<sqlite3_value*>& row) {
    int length = sqlite3_value_bytes(row[0]);
    // TODO: Handle UTF-8 text??
    string zettelId((const char*)sqlite3_value_text(row[0]), length);

    length = sqlite3_value_bytes(row[1]);
    string title((const char*)sqlite3_value_text(row[1]), length);

    length = sqlite3_value_bytes(row[2]);
    string filename((const char*)sqlite3_value_text(row[2]), length);

    return {
        std::move(zettelId),
        std::move(title),
        std::move(filename),
        (uint32_t)sqlite3_value_int(row[3])
    };
}

struct zettel_tag zettel_tag::from(const vector<sqlite3_value*>& row) {
    return {
        (uint32_t)sqlite3_value_int(row[0]),
        (uint32_t)sqlite3_value_int(row[1]),
        (uint32_t)sqlite3_value_int(row[2])
    };
}

}

Index::Index(const char* db) : m_db(db) {}

vector<unique_ptr<Id>> Index::search(string tag) {
    // NOTE: rather than sanitizing or using prepared statements, we will simply force tags to be lowercase letters. This may change in the future.
    for (char c : tag) {
        if (c < 97 || c > 122) {
            throw Index::Exception("tags must only consist of lowercase letters");
        }
    }
    vector<unique_ptr<Id>> results;
    string query(zettel::fmt("SELECT zettel_tag.tag_id, zettel_tag.zettel_id, zettel_tag.id FROM zettel_tag INNER JOIN tag on tag.tag = \"%s\"", tag));
    SQLite::iterator<models::zettel> rset(m_db.query<models::zettel>(query));
    for (; !rset.done(); ++rset) {
        // TODO: Don't default to Numeric, pull the type dynamically.
        try {
            unique_ptr<Id> id = Id::parse(rset->note_id, Id::Type::Numeric);
            results.push_back(id);
        } catch (const Id::Exception& exc) {
            log::warn("Bad Zettel ID encountered: %s", exc.what());
        }
    }
    return results;
}

void Index::insert(const Zettel* zettel) {
    m_db.begin();
    // TODO: queries with prepared statements and binding...
    m_db.commit();
}

}
