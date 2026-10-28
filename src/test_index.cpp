#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <sstream>

#include "commands.hpp"
#include "index.hpp"
#include "logging.hpp"
#include "parser.hpp"
#include "zettel.hpp"

using std::istringstream, std::make_shared, std::make_unique, std::map, std::set, std::shared_ptr, std::string, std::vector;
using namespace zettel;

class IndexFixture {
public:
    IndexFixture() : m_db(make_shared<sql::SQLite>(":memory:")), m_ctx({
            .root = "",
            .dotdir = "",
            .db = m_db,
            .index = make_unique<Index>(m_db)
        }), m_idx(m_ctx.index.get()), m_captureHandler(make_shared<logs::CaptureLogHandler>()) {
        cmd::initialize(m_ctx);
        logs::putHandler("capture", m_captureHandler);
    }

    virtual ~IndexFixture() {
        logs::popHandler("capture");
    }
protected:
    std::shared_ptr<logs::CaptureLogHandler> m_captureHandler;

    std::shared_ptr<sql::SQLite> m_db;
    cmd::Context m_ctx;
    Index* m_idx;
};

Zettel make_zettel(string title, string body, Zettel::Id id = 0) {
    Zettel z(id, title, "");

    istringstream ss(body);
    ss.seekg(0);
    DefaultParser parser(ss);
    while (!parser.done()) z.addContentBlock(parser.next());

    return z;
}

TEST_CASE_METHOD(IndexFixture, "insert zettel without reference", "[index]") {
    Zettel z = make_zettel("test", "this is the body");
    m_idx->insert(&z);
    REQUIRE(m_idx->exists(0));
}

TEST_CASE_METHOD(IndexFixture, "insert zettel with good reference", "[index]") {
    Zettel z1 = make_zettel("ingress", "this is the ingress point", 0);
    Zettel z2 = make_zettel("hallway", "this hallway comes from the [ingress](zettel:0)", 1);
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    REQUIRE(m_idx->exists(0));
    REQUIRE(m_idx->exists(1));

    set<Zettel::Id> refs = m_idx->getReferencesFor(z2.id(), true);
    REQUIRE(refs.size() == 1);
    REQUIRE(refs.find(0) != refs.end());

    // there should be no logged messages
    const vector<logs::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 0);
}

TEST_CASE_METHOD(IndexFixture, "insert zettel with bad reference", "[index]") {
    Zettel z = make_zettel("ingress", "this is the ingress point [bad reference](zettel:1)");
    m_idx->insert(&z);
    REQUIRE(m_idx->exists(0));

    set<Zettel::Id> goodRefs = m_idx->getReferencesFor(z.id(), true);
    REQUIRE(goodRefs.size() == 0);

    set<Zettel::Id> allRefs = m_idx->getReferencesFor(z.id(), false);
    REQUIRE(allRefs.size() == 1);
    REQUIRE(allRefs.find(1) != allRefs.end());

    // ensure warning was logged
    const vector<logs::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == logs::WARNING);
    REQUIRE(records[0].message.compare("Reference to non-existent note '1'") == 0);
}

TEST_CASE_METHOD(IndexFixture, "update zettel with bad reference to good reference", "[index]") {
    Zettel z1 = make_zettel("ingress", "this is the ingress point");
    Zettel z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:100)", 1);
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    REQUIRE(m_idx->exists(0));
    REQUIRE(m_idx->exists(1));

    const vector<logs::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == logs::WARNING);
    REQUIRE(records[0].message.compare("Reference to non-existent note '100'") == 0);

    // ensure that there is an invalid reference
    set<Zettel::Id> refs = m_idx->getReferencesFor(z2.id(), true);
    REQUIRE(refs.size() == 0);

    set<Zettel::Id> allRefs = m_idx->getReferencesFor(z2.id(), false);
    REQUIRE(allRefs.size() == 1);
    REQUIRE(allRefs.find(100) != allRefs.end());

    m_captureHandler->clear();

    z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:0)", 1);
    m_idx->update(&z2);

    // check our references
    sql::SQLite::iterator<models::reference> results = m_db->query<models::reference>("SELECT source, dest, valid FROM reference");
    vector<models::reference> mrefs = sql::gather(results);

    // ensure that there is a valid reference, and no invalid references
    refs = m_idx->getReferencesFor(z2.id(), true);
    REQUIRE(refs.size() == 1);
    REQUIRE(refs.find(0) != refs.end());

    refs = m_idx->getReferencesFor(z2.id(), false);
    REQUIRE(refs.size() == 1);
}

TEST_CASE_METHOD(IndexFixture, "update zettel with good reference to bad reference", "[index]") {
    Zettel z1 = make_zettel("ingress", "this is the ingress point");
    Zettel z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:0)", 1);
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    REQUIRE(m_idx->exists(0));
    REQUIRE(m_idx->exists(1));

    // ensure that there is a valid reference, and no invalid references
    set<Zettel::Id> refs = m_idx->getReferencesFor(z2.id(), true);
    REQUIRE(refs.size() == 1);
    REQUIRE(refs.find(0) != refs.end());

    refs = m_idx->getReferencesFor(z2.id(), false);
    REQUIRE(refs.size() == 1);

    z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:100)", 1);
    m_idx->update(&z2);

    const vector<logs::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == logs::WARNING);
    REQUIRE(records[0].message.compare("Reference to non-existent note '100'") == 0);

    // ensure that there is an invalid reference
    refs = m_idx->getReferencesFor(z2.id(), true);
    REQUIRE(refs.size() == 0);

    set<Zettel::Id> allRefs = m_idx->getReferencesFor(z2.id(), false);
    REQUIRE(allRefs.size() == 1);
    REQUIRE(allRefs.find(100) != allRefs.end());
}

TEST_CASE_METHOD(IndexFixture, "search by tag", "[index]") {
    Zettel z(0, "", "");
    for (int i = 1; i <= 10; i++) {
        z = Zettel(i, fmt("Zettel %d", i), "");
        if (i % 2 == 0) z.addTag("even");
        m_idx->insert(&z);
    }
    vector<models::zettel> results = m_idx->searchTag("even");
    REQUIRE(results.size() == 5);
    for (int i = 0; i < 5; i++) REQUIRE(results[4 - i].id == (i + 1) * 2);
}

TEST_CASE_METHOD(IndexFixture, "search by title", "[index]") {
    Zettel z(0, "title", "");
    m_idx->insert(&z);
    vector<models::zettel> results = m_idx->searchTitle("title");
    REQUIRE(results.size() == 1);
}

TEST_CASE_METHOD(IndexFixture, "search by text", "[index]") {
    Zettel z(0, "", "/tmp/0.txt");
    z.addContentBlock(std::unique_ptr<ContentBlock>(new TextBlock(0, "this is the zettel text")));
    Zettel z2(1, "", "/tmp/1.txt");
    z2.addContentBlock(std::unique_ptr<ContentBlock>(new TextBlock(1, "this is some text")));
    z.save();
    z2.save();
    m_idx->insert(&z);
    m_idx->insert(&z2);
    vector<models::zettel> results = m_idx->searchFullText("zettel", "/tmp");
    INFO(fmt("result count: %zd", results.size()));
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].id == 0);
}

TEST_CASE_METHOD(IndexFixture, "delete a Zettel", "[index]") {
    Zettel z1(0, "Ingress", "");
    Zettel z2(1, "Hallway", "");
    z2.addContentBlock(std::unique_ptr<ContentBlock>(new ReferenceBlock(0, "zettel:0", "the ingress")));
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    REQUIRE(m_idx->exists(0));
    REQUIRE(m_idx->exists(1));

    set<Zettel::Id> refs = m_idx->getReferencesFor(z2.id(), true);
    REQUIRE(refs.size() == 1);

    m_idx->remove(&z1);

    const vector<logs::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == logs::WARNING);
    REQUIRE(records[0].message.compare("A reference from 1 to 0 is now invalid") == 0);
}

TEST_CASE_METHOD(IndexFixture, "test lookup", "[index]") {
    Zettel z1(0, "Ingress", "");
    Zettel z2(1, "Hallway", "");
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    map<Zettel::Id, models::zettel> result = m_idx->lookup({0, 1});
    REQUIRE(result.size() == 2);
    REQUIRE(result.at(0).title.compare(z1.title()) == 0);
    REQUIRE(result.at(1).title.compare(z2.title()) == 0);
}
