#include "index.hpp"

#include <string>
#include <sstream>

#include "logging.hpp"

using std::string, std::stringstream, std::unique_ptr, std::vector;

namespace zettel {

namespace models {

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
    };
}

struct zettel_tag zettel_tag::from(const vector<sqlite3_value*>& row) {
    return {
        (const char*)sqlite3_value_text(row[0]),
        (const char*)sqlite3_value_text(row[1]),
        (uint32_t)sqlite3_value_int(row[2])
    };
}

}

Index::Index(const char* db) : m_db(db) {}

vector<unique_ptr<Id>> Index::search(string tag) {
    vector<unique_ptr<Id>> results;
    sql::SQLite::iterator<models::zettel_tag> rset = m_db.query<models::zettel_tag>(
        "SELECT zettel_tag.tag, zettel_tag.zettel_id, zettel_tag.id FROM zettel_tag WHERE tag = ?",
        sql::paramlist{tag}
    );
    for (; !rset.done(); ++rset) {
        // TODO: Don't default to Numeric, pull the type dynamically.
        try {
            unique_ptr<Id> id = Id::parse(rset->zettel_id, Id::Type::Numeric);
            results.push_back(id);
        } catch (const Id::Exception& exc) {
            log::warn("Bad Zettel ID encountered: %s", exc.what());
        }
    }
    return results;
}

void Index::insert(const Zettel* zettel) {
    m_db.begin();
    string zid = zettel->id().represent();
    m_db.query(
        "INSERT INTO zettel (id, title, filename) VALUES (?, ?, ?)",
        sql::paramlist{ zid, zettel->title(), zettel->file().string() }
    );
    for (const string& tag : zettel->tags()) m_db.query("INSERT INTO zettel_tag (tag, zettel_id) VALUES (?, ?)", sql::paramlist{ tag, zid });
    m_db.commit();
}

void Index::update(const Zettel* zettel) {
    m_db.begin();
    m_db.query("UPDATE zettel SET title=?", sql::paramlist{ zettel->title() });
    string zid = zettel->id().represent();
    stringstream query("DELETE FROM zettel_tag WHERE tag NOT IN (");
    const vector<string>& tags = zettel->tags();
    for (int i = 0; i < tags.size(); i++) {
        m_db.query("INSERT INTO zettel_tag (tag, zettel_id) VALUES (?, ?)", sql::paramlist{tags[i], zid});
        if (i > 0) query << ", ";
        query << '?';
    }
    query << ')';
    m_db.query(query.str(), sql::paramvec<string>{zettel->tags()});
    m_db.commit();
}

}
