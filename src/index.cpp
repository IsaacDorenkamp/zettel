#include "index.hpp"

#include <memory>
#include <set>
#include <string>
#include <sstream>

#include "logging.hpp"

using std::optional, std::set, std::string, std::stringstream, std::unique_ptr, std::vector;

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

Index::Index(const char* db) : m_db(std::make_shared<sql::SQLite>(db)) {}
Index::Index(const std::shared_ptr<sql::SQLite>& db) : m_db(db) {}

void Index::begin() {
    m_db->begin();
}

void Index::commit() {
    m_db->commit();
}

vector<models::zettel> Index::search(string tag) {
    vector<models::zettel> results;
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>(
        "SELECT zettel.id, zettel.title FROM zettel INNER JOIN tag ON tag.zettel_id = zettel.id WHERE tag.tag = ?",
        sql::paramlist{tag}
    );
    for (; !rset.done(); ++rset) {
        results.push_back(*rset);
    }
    return results;
}

bool Index::exists(uint32_t id) {
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>("SELECT zettel.id, zettel.title FROM zettel WHERE zettel.id = ?", sql::paramlist{(int)id});
    return (bool)sql::maybeone(rset);
}

void Index::insert(const Zettel* zettel) {
    m_db->query(
        "INSERT INTO zettel (id, title) VALUES (?, ?)",
        sql::paramlist{ (int)zettel->id(), zettel->title() }
    );
    for (const string& tag : zettel->tags()) m_db->query("INSERT INTO tag (tag, zettel_id) VALUES (?, ?)", sql::paramlist{ tag, (int)zettel->id() });
}

void Index::update(const Zettel* zettel) {
    m_db->query("UPDATE zettel SET title=?", sql::paramlist{ zettel->title() });
    stringstream query("DELETE FROM tag WHERE tag NOT IN (");
    query.seekp(0, std::ios_base::end);
    const vector<string>& tags = zettel->tags();
    // TODO: refactor to avoid querying within the loop
    for (int i = 0; i < tags.size(); i++) {
        m_db->query("INSERT INTO tag (tag, zettel_id) VALUES (?, ?) ON CONFLICT (zettel_id, tag) DO NOTHING", sql::paramlist{tags[i], (int)zettel->id()});
        if (i > 0) query << ", ";
        query << '?';
    }
    query << ')';
    m_db->query(query.str(), sql::paramvec<string>{zettel->tags()});

    // we use a set since a Zettel might refer to the same Zettel multiple times
    set<Zettel::Id> zettelRefs;
    ReferenceBlock* ref;
    string uri;
    unsigned long zid;
    for (const unique_ptr<ContentBlock>& block : zettel->content()) {
        if ((ref = dynamic_cast<ReferenceBlock*>(block.get()))) {
            uri = ref->uri();
            if (uri.size() >= 8 && uri.substr(0, 7).compare("zettel:") == 0) {
                try {
                    zid = stoul(uri.substr(7));
                    zettelRefs.insert(zid);
                } catch (const std::exception& exc) {
                    log::warn("Found reference to Zettel with invalid ID '%s'", uri.c_str());
                }
            }
        }
    }

    query.str("DELETE FROM references WHERE source = ? AND dest NOT IN (");
    for (size_t index = 0; index < zettelRefs.size(); index++) {
        if (index > 0) query << ", ";
        query << '?';
    }
    query << ')';
    vector<string> params{std::to_string(zettel->id())};
    for (const Zettel::Id& id : zettelRefs) params.push_back(std::to_string(id));
    m_db->query(query.str(), sql::paramvec<string>{params});
    // TODO: Add new references
}

uint32_t Index::nextId() {
    sql::SQLite::iterator<models::zettel> lastResult = m_db->query<models::zettel>("SELECT id, title FROM zettel ORDER BY id DESC LIMIT 1");
    optional<models::zettel> last = sql::maybeone(lastResult);
    if (last) {
        return last->id + 1;
    } else return 0;
}

}
