#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <vector>

#include "buffer.hpp"
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

struct TestRow2 {
    int a;
    std::string b;
    double c;
    zettel::buffer d;

    static TestRow2 from(const std::vector<sqlite3_value*>& values) {
        return TestRow2 {
            sqlite3_value_int(values[0]),
            (const char*)sqlite3_value_text(values[1]),
            sqlite3_value_double(values[2]),
            zettel::buffer(sqlite3_value_blob(values[3]), sqlite3_value_bytes(values[3]))
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
    zettel::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;");
    uint8_t rows = 0;
    for (; !it.done(); ++it, ++rows);
    REQUIRE(rows == 2);
}

TEST_CASE("querying table produces correct data", "[sql]") {
    zettel::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    zettel::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;");
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

TEST_CASE("query with unnamed params", "[sql]") {
    zettel::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT, c REAL, d BLOB)");

    void* blob = malloc(4);
    memcpy(blob, "blob", 4);
    zettel::buffer b(blob, 4);
    db.query("INSERT INTO test (a, b, c, d) VALUES (?, ?, ?, ?)", zettel::sql::paramlist{1, "text", 2.0, std::move(b)});

    zettel::SQLite::iterator<TestRow2> it = db.query<TestRow2>("SELECT a, b, c, d FROM test;");
    TestRow2& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("text") == 0);
    REQUIRE(current.c == 2.0);
    REQUIRE(strncmp((const char*)current.d.buf(), "blob", current.d.size()) == 0);
}
