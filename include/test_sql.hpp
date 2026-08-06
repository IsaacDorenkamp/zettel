#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "sql.hpp"

struct TestRow {
    int a;
    std::string b;

    static TestRow from(const std::vector<sqlite3_value*>& values) {
        return TestRow {
            sqlite3_value_int(values[0]),
            (const char*)sqlite3_value_text(values[1]),
        };
    }
};

TEST_CASE("validity checks", "[sql]") {
    zettel::SQLite db("file:test.db?mode=invalid");
    REQUIRE(!db.valid());
    bool valid = true;
    try {
        db.query("SELECT 1;");
    } catch (const zettel::SQLite::Exception& exc) {
        valid = false;
    }
    REQUIRE(!valid);
}

TEST_CASE("handle query error correctly", "[sql]") {
    zettel::SQLite db(":memory:");
    bool exceptionOccurred = false;
    try {
        db.query("SELECT a, b FROM test;");
    } catch (const zettel::SQLite::Exception& exc) {
        exceptionOccurred = true;
    }
    REQUIRE(exceptionOccurred);
}

TEST_CASE("create and query table", "[sql]") {
    zettel::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\"), (2, \"world\")");
    zettel::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;", [](const std::vector<sqlite3_value*>& values) {
        return TestRow {
            sqlite3_value_int(values[0]),
            (const char*)sqlite3_value_text(values[1])
        };
    });
    uint8_t rows = 0;
    for (; !it.done(); ++it, ++rows);
    REQUIRE(rows == 2);
}

TEST_CASE("querying table produces correct data", "[sql]") {
    zettel::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    zettel::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;", [](const std::vector<sqlite3_value*>& values) {
        return TestRow {
            sqlite3_value_int(values[0]),
            (const char*)sqlite3_value_text(values[1])
        };
    });
    TestRow& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("hello") == 0);
}

TEST_CASE("query with a self-constructing model works", "[sql]") {
    zettel::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    zettel::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;");
    TestRow& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("hello") == 0);
}
