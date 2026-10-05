#include "index.hpp"

#include <string>
#include <sstream>

using std::optional, std::string, std::stringstream, std::vector;

namespace zettel {

namespace models {

struct zettel zettel::from(const vector<sqlite3_value*>& row) {
    uint32_t zettelId(sqlite3_value_int64(row[0]));

    int length = sqlite3_value_bytes(row[1]);
    string title((const char*)sqlite3_value_text(row[1]), length);

    return {
        zettelId,
        std::move(title),
    };
}

struct zettel_tag zettel_tag::from(const vector<sqlite3_value*>& row) {
    return {
        (const char*)sqlite3_value_text(row[0]),
        (uint32_t)sqlite3_value_int64(row[1]),
        (uint32_t)sqlite3_value_int(row[2])
    };
}

}

Index::Index(const char* db) : m_db(db) {}

vector<Zettel::Id> Index::search(string tag) {
    vector<Zettel::Id> results;
    sql::SQLite::iterator<models::zettel_tag> rset = m_db.query<models::zettel_tag>(
        "SELECT zettel_tag.tag, zettel_tag.zettel_id, zettel_tag.id FROM zettel_tag WHERE tag = ?",
        sql::paramlist{tag}
    );
    for (; !rset.done(); ++rset) {
        results.push_back(rset->zettel_id);
    }
    return results;
}

void Index::insert(const Zettel* zettel) {
    m_db.begin();
    m_db.query(
        "INSERT INTO zettel (id, title, filename) VALUES (?, ?, ?)",
        sql::paramlist{ (int)zettel->id(), zettel->title(), zettel->file().string() }
    );
    for (const string& tag : zettel->tags()) m_db.query("INSERT INTO zettel_tag (tag, zettel_id) VALUES (?, ?)", sql::paramlist{ tag, (int)zettel->id() });
    m_db.commit();
}

void Index::update(const Zettel* zettel) {
    m_db.begin();
    m_db.query("UPDATE zettel SET title=?", sql::paramlist{ zettel->title() });
    stringstream query("DELETE FROM zettel_tag WHERE tag NOT IN (");
    const vector<string>& tags = zettel->tags();
    for (int i = 0; i < tags.size(); i++) {
        m_db.query("INSERT INTO zettel_tag (tag, zettel_id) VALUES (?, ?)", sql::paramlist{tags[i], (int)zettel->id()});
        if (i > 0) query << ", ";
        query << '?';
    }
    query << ')';
    m_db.query(query.str(), sql::paramvec<string>{zettel->tags()});
    m_db.commit();
}

uint32_t Index::nextId() {
    sql::SQLite::iterator<models::zettel> lastResult = m_db.query<models::zettel>("SELECT id, title FROM zettel ORDER BY id DESC LIMIT 1");
    optional<models::zettel> last = sql::maybeone(lastResult);
    if (last) {
        return last->id + 1;
    } else return 0;
}

}
