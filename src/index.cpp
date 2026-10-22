#include "index.hpp"

#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <variant>

#include "format.hpp"
#include "logging.hpp"
#include "threading.hpp"
#include "zettel.hpp"

using std::map, std::optional, std::set, std::string, std::stringstream, std::unique_ptr, std::vector;

struct FullTextSearchQuery {
    string term;
    std::filesystem::path file;
};

struct FullTextSearchResult {
    optional<zettel::models::zettel> match;
    string matchText;
};

static std::variant<FullTextSearchResult, string> checkFile(const FullTextSearchQuery& query) {
    using std::ifstream;
    using zettel::Zettel;
    try {
        Zettel z = Zettel::load(query.file);
        // first, get plaintext body of zettel
        stringstream contentss;
        for (const std::unique_ptr<zettel::ContentBlock>& block : z.content()) contentss << block->format(zettel::PLAINTEXT_FORMAT);
        string content(contentss.str());
        // TODO: This search method is very simplistic and inflexible. In the future, a more advanced and flexible one should be used.
        size_t matchAt = content.find(query.term);
        if (matchAt == string::npos) {
            return FullTextSearchResult{std::nullopt, ""};
        } else {
            return FullTextSearchResult{
                zettel::models::zettel{z.id(), z.title()},
                query.term
            };
        }
    } catch (const zettel::ZettelException& exc) {
        return exc.what();
    }
}

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

struct reference reference::from(const vector<sqlite3_value*>& row) {
    return {
        (uint32_t)sqlite3_value_int(row[0]),
        (uint32_t)sqlite3_value_int(row[1]),
        sqlite3_value_int(row[2]) > 0
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

void Index::rollback() {
    m_db->rollback();
}

vector<models::zettel> Index::searchTag(string tag) {
    vector<models::zettel> results;
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>(
        "SELECT zettel.id, zettel.title FROM zettel INNER JOIN tag ON tag.zettel_id = zettel.id WHERE tag.tag = ? ORDER BY id DESC",
        sql::paramlist{tag}
    );
    return sql::gather(rset);
}

vector<models::zettel> Index::searchTitle(string title) {
    vector<models::zettel> results;
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>(
        "SELECT zettel.id, zettel.title FROM zettel WHERE zettel.title LIKE ?",
        sql::paramlist{fmt("%%%s%%", title.c_str())}
    );
    return sql::gather(rset);
}

vector<models::zettel> Index::searchFullText(string term, const std::filesystem::path& root) {
    // TODO: Find some way to make this more efficient??
    vector<models::zettel> results;
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>("SELECT zettel.id, zettel.title FROM zettel");
    sys::ThreadSafeQueue<FullTextSearchQuery> queries;
    sys::ThreadSafeQueue<std::variant<FullTextSearchResult, string>> searchResults;

    for (const models::zettel& entry : sql::gather(rset)) {
        queries.put(FullTextSearchQuery{
            term,
            root / Zettel::getFilenameForId(entry.id)
        });
    }
    queries.shutdown();

    sys::ThreadPool<FullTextSearchQuery, std::variant<FullTextSearchResult, string>> pool(queries, searchResults, &checkFile);
    pool.execute(4);

    searchResults.sink<vector<models::zettel>, models::zettel>(results, [](std::variant<FullTextSearchResult, string> result) -> optional<models::zettel> {
        if (std::holds_alternative<FullTextSearchResult>(result)) {
            FullTextSearchResult item = std::get<FullTextSearchResult>(result);
            if (item.match) return *item.match;
            else return std::nullopt;
        } else {
            return std::nullopt;
        }
    });
    return results;
}

bool Index::exists(const Zettel::Id& id) {
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>("SELECT zettel.id, zettel.title FROM zettel WHERE zettel.id = ?", sql::paramlist{(int)id});
    return (bool)sql::maybeone(rset);
}

map<Zettel::Id, models::zettel> Index::lookup(set<Zettel::Id> ids) {
    stringstream query;
    query << "SELECT zettel.id, zettel.title FROM zettel WHERE zettel.id IN (";
    bool first = true;
    for (const Zettel::Id& id : ids) {
        if (first) first = false;
        else query << ", ";
        query << '?';
    }
    query << ")";
    vector<int> qids;
    for (const Zettel::Id& id : ids) qids.push_back((int)id);
    sql::SQLite::iterator<models::zettel> rset = m_db->query<models::zettel>(query.str(), sql::paramvec<int>{qids});
    map<Zettel::Id, models::zettel> results;
    for(; !rset.done(); ++rset) results.insert({ rset->id, *rset });
    return results;
}

set<Zettel::Id> Index::getReferencesFor(const Zettel::Id& id, bool validOnly) {
    string query = fmt("SELECT source, dest, valid FROM reference WHERE source = ?%s", validOnly ? " AND valid > 0" : "");
    sql::SQLite::iterator<models::reference> rset = m_db->query<models::reference>(query, sql::paramlist{(int)id});
    set<Zettel::Id> ids;
    while (!rset.done()) ids.insert((rset++)->dest);
    return ids;
}

set<Zettel::Id> Index::getReferencesTo(const Zettel::Id& id, bool validOnly) {
    string query = fmt("SELECT source, dest, valid FROM reference WHERE dest = ?%s", validOnly ? " AND valid > 0" : "");
    sql::SQLite::iterator<models::reference> rset = m_db->query<models::reference>(query, sql::paramlist{(int)id});
    set<Zettel::Id> ids;
    while (!rset.done()) ids.insert((rset++)->source);
    return ids;
}

void Index::insert(const Zettel* zettel) {
    m_db->begin();
    m_db->query(
        "INSERT INTO zettel (id, title) VALUES (?, ?)",
        sql::paramlist{ (int)zettel->id(), zettel->title() }
    );
    for (const string& tag : zettel->tags()) m_db->query("INSERT INTO tag (tag, zettel_id) VALUES (?, ?)", sql::paramlist{ tag, (int)zettel->id() });
    syncReferences(zettel);
    m_db->commit();
}

void Index::update(const Zettel* zettel) {
    m_db->begin();
    m_db->query("UPDATE zettel SET title=? WHERE id=?", sql::paramlist{ zettel->title(), (int)zettel->id() });
    stringstream query("DELETE FROM tag WHERE zettel_id = ? AND tag NOT IN (");
    query.seekp(0, std::ios_base::end);
    vector<string> tags = zettel->tags();
    // TODO: refactor to avoid querying within the loop
    for (int i = 0; i < tags.size(); i++) {
        m_db->query("INSERT INTO tag (tag, zettel_id) VALUES (?, ?) ON CONFLICT (zettel_id, tag) DO NOTHING", sql::paramlist{tags[i], (int)zettel->id()});
        if (i > 0) query << ", ";
        query << '?';
    }
    query << ')';
    tags.insert(tags.begin(), std::to_string(zettel->id()));
    m_db->query(query.str(), sql::paramvec<string>{tags});
    syncReferences(zettel);
    m_db->commit();
}

void Index::remove(const Zettel* zettel) {
    m_db->begin();
    // invalidate references to this Zettel
    sql::SQLite::iterator<models::reference> affected = m_db->query<models::reference>("UPDATE reference SET valid = 0 WHERE dest = ? RETURNING source, dest, valid", sql::paramlist{(int)zettel->id()});
    while (!affected.done()) {
        logs::warn("A reference from %u to %u is now invalid", affected->source, affected->dest);
        affected++;
    }
    m_db->query("DELETE FROM zettel WHERE id = ?", sql::paramlist{(int)zettel->id()});
    m_db->commit();
}

void Index::syncReferences(const Zettel* zettel) {
    m_db->query("UPDATE reference SET valid = true WHERE dest = ? AND valid = 0", sql::paramlist{(int)zettel->id()});

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
                    logs::warn("Found reference to Zettel with invalid ID '%s'", uri.c_str());
                }
            }
        }
    }

    stringstream query("DELETE FROM reference WHERE source = ? AND dest NOT IN (");
    query.seekp(0, std::ios_base::end);
    for (size_t index = 0; index < zettelRefs.size(); index++) {
        if (index > 0) query << ", ";
        query << '?';
    }
    query << ')';
    vector<int> params;
    params.push_back(zettel->id());
    for (const Zettel::Id& id : zettelRefs) params.push_back((int)id);
    m_db->query(query.str(), sql::paramvec<int>{params});
    for (const Zettel::Id& id : zettelRefs) {
        sql::SQLite::iterator<models::reference> result = m_db->query<models::reference>("INSERT INTO reference (source, dest, valid) VALUES (:zid, :destid, (SELECT COUNT(id) FROM zettel WHERE id = :destid))"
                " ON CONFLICT DO NOTHING RETURNING source, dest, valid", sql::paramdict{ {"zid", (int)zettel->id()}, {"destid", (int)id} });
        optional<models::reference> ref = sql::maybeone(result);
        if (ref && !ref->valid) logs::warn("Reference to non-existent note '%u'", id);
    }
}

Zettel::Id Index::nextId() {
    sql::SQLite::iterator<models::zettel> lastResult = m_db->query<models::zettel>("SELECT id, title FROM zettel ORDER BY id DESC LIMIT 1");
    optional<models::zettel> last = sql::maybeone(lastResult);
    if (last) {
        return last->id + 1;
    } else return 0;
}

}
