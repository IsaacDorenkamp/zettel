#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <sstream>

#include "commands.hpp"
#include "index.hpp"
#include "logging.hpp"
#include "parser.hpp"
#include "zettel.hpp"

using std::istringstream, std::make_shared, std::make_unique, std::set, std::shared_ptr, std::string, std::vector;
using namespace zettel;

class IndexFixture {
public:
    IndexFixture() : m_db(make_shared<sql::SQLite>(":memory:")), m_ctx({
            .root = "",
            .dotdir = "",
            .db = m_db,
            .index = make_unique<Index>(m_db)
        }), m_idx(m_ctx.index.get()), m_defaultHandler(), m_captureHandler(make_shared<log::CaptureLogHandler>()) {
        cmd::initialize(m_ctx);
        m_defaultHandler = log::popHandler("default");
        log::putHandler("capture", m_captureHandler);
    }

    virtual ~IndexFixture() {
        log::putHandler("default", std::move(m_defaultHandler));
        log::popHandler("capture");
    }
protected:
    std::shared_ptr<log::LogHandler> m_defaultHandler;
    std::shared_ptr<log::CaptureLogHandler> m_captureHandler;

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

    set<Zettel::Id> refs = m_idx->getReferences(z2.id(), true);
    REQUIRE(refs.size() == 1);
    REQUIRE(refs.find(0) != refs.end());

    // there should be no logged messages
    const vector<log::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 0);
}

TEST_CASE_METHOD(IndexFixture, "insert zettel with bad reference", "[index]") {
    Zettel z = make_zettel("ingress", "this is the ingress point [bad reference](zettel:1)");
    m_idx->insert(&z);
    REQUIRE(m_idx->exists(0));

    set<Zettel::Id> goodRefs = m_idx->getReferences(z.id(), true);
    REQUIRE(goodRefs.size() == 0);

    set<Zettel::Id> allRefs = m_idx->getReferences(z.id(), false);
    REQUIRE(allRefs.size() == 1);
    REQUIRE(allRefs.find(1) != allRefs.end());

    // ensure warning was logged
    const vector<log::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == log::WARNING);
    REQUIRE(records[0].message.compare("Reference to non-existent note '1'") == 0);
}

TEST_CASE_METHOD(IndexFixture, "update zettel with bad reference to good reference", "[index]") {
    std::cout << "=====" << std::endl;
    Zettel z1 = make_zettel("ingress", "this is the ingress point");
    Zettel z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:100)", 1);
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    REQUIRE(m_idx->exists(0));
    REQUIRE(m_idx->exists(1));

    const vector<log::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == log::WARNING);
    REQUIRE(records[0].message.compare("Reference to non-existent note '100'") == 0);

    // ensure that there is an invalid reference
    set<Zettel::Id> refs = m_idx->getReferences(z2.id(), true);
    REQUIRE(refs.size() == 0);

    set<Zettel::Id> allRefs = m_idx->getReferences(z2.id(), false);
    REQUIRE(allRefs.size() == 1);
    REQUIRE(allRefs.find(100) != allRefs.end());

    m_captureHandler->clear();

    z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:0)", 1);
    m_idx->update(&z2);

    // check our references
    sql::SQLite::iterator<models::reference> results = m_db->query<models::reference>("SELECT source, dest, valid FROM reference");
    vector<models::reference> mrefs = sql::gather(results);
    INFO(fmt("mrefs: %zd", mrefs.size()));
    for (const models::reference& ref : mrefs) {
        std::cout << fmt("source: %u\tdest: %u\tvalid: %d", ref.source, ref.dest, ref.valid) << std::endl;
    }

    // ensure that there is a valid reference, and no invalid references
    refs = m_idx->getReferences(z2.id(), true);
    REQUIRE(refs.size() == 1);
    REQUIRE(refs.find(0) != refs.end());

    refs = m_idx->getReferences(z2.id(), false);
    REQUIRE(refs.size() == 0);
    std::cout << "=====" << std::endl;
}

TEST_CASE_METHOD(IndexFixture, "update zettel with good reference to bad reference", "[index]") {
    Zettel z1 = make_zettel("ingress", "this is the ingress point");
    Zettel z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:0)", 1);
    m_idx->insert(&z1);
    m_idx->insert(&z2);
    REQUIRE(m_idx->exists(0));
    REQUIRE(m_idx->exists(1));

    // ensure that there is a valid reference, and no invalid references
    set<Zettel::Id> refs = m_idx->getReferences(z2.id(), true);
    REQUIRE(refs.size() == 1);
    REQUIRE(refs.find(0) != refs.end());

    refs = m_idx->getReferences(z2.id(), false);
    REQUIRE(refs.size() == 0);

    z2 = make_zettel("egress", "this is the egress point from the [ingress](zettel:100)", 1);
    m_idx->update(&z2);

    const vector<log::LogRecord>& records = m_captureHandler->records();
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].level == log::WARNING);
    REQUIRE(records[0].message.compare("Reference to non-existent note '100'") == 0);

    // ensure that there is an invalid reference
    refs = m_idx->getReferences(z2.id(), true);
    REQUIRE(refs.size() == 0);

    set<Zettel::Id> allRefs = m_idx->getReferences(z2.id(), false);
    REQUIRE(allRefs.size() == 1);
    REQUIRE(allRefs.find(100) != allRefs.end());
}
