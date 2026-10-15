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

struct TestRow3 {
    int a;
    int b;
    int c;

    static TestRow3 from(const std::vector<sqlite3_value*>& values) {
        return TestRow3 {
            sqlite3_value_int(values[0]),
            sqlite3_value_int(values[1]),
            sqlite3_value_int(values[2])
        };
    }
};

using namespace zettel;

TEST_CASE("validity checks", "[sql]") {
    sql::SQLite db("file:test.db?mode=invalid");
    REQUIRE(!db.valid());
    bool valid = true;
    try {
        db.query("SELECT 1;");
    } catch (const sql::SQLite::Exception& exc) {
        valid = false;
    }
    REQUIRE(!valid);
}

TEST_CASE("handle query error correctly", "[sql]") {
    sql::SQLite db(":memory:");
    bool exceptionOccurred = false;
    try {
        db.query("SELECT a, b FROM test;");
    } catch (const sql::SQLite::Exception& exc) {
        exceptionOccurred = true;
    }
    REQUIRE(exceptionOccurred);
}

TEST_CASE("create and query table", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\"), (2, \"world\")");
    sql::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;");
    uint8_t rows = 0;
    for (; !it.done(); ++it, ++rows);
    REQUIRE(rows == 2);
}

TEST_CASE("querying table produces correct data", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    sql::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;");
    TestRow& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("hello") == 0);
}

TEST_CASE("query with a self-constructing model works", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    sql::SQLite::iterator<TestRow> it = db.query<TestRow>("SELECT a, b FROM test;");
    TestRow& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("hello") == 0);
}

TEST_CASE("query with unnamed params", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT, c REAL, d BLOB)");

    void* blob = malloc(4);
    memcpy(blob, "blob", 4);
    zettel::buffer b(blob, 4);
    db.query("INSERT INTO test (a, b, c, d) VALUES (?, ?, ?, ?)", zettel::sql::paramlist{1, "text", 2.0, std::move(b)});

    sql::SQLite::iterator<TestRow2> it = db.query<TestRow2>("SELECT a, b, c, d FROM test");
    TestRow2& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("text") == 0);
    REQUIRE(current.c == 2.0);
    REQUIRE(strncmp((const char*)current.d.buf(), "blob", current.d.size()) == 0);
}

TEST_CASE("query with named params", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT, c REAL, d BLOB)");

    void* blob = malloc(4);
    memcpy(blob, "blob", 4);
    zettel::buffer b(blob, 4);
    db.query("INSERT INTO test (a, b, c, d) VALUES (:a, :b, :c, :d)", zettel::sql::paramdict{
        {"a", 1},
        {"b", "text"},
        {"c", 2.0},
        {"d", b}
    });

    sql::SQLite::iterator<TestRow2> it = db.query<TestRow2>("SELECT a, b, c, d FROM test");
    TestRow2& current = *it;
    REQUIRE(current.a == 1);
    REQUIRE(current.b.compare("text") == 0);
    REQUIRE(current.c == 2.0);
    REQUIRE(strncmp((const char*)current.d.buf(), "blob", current.d.size()) == 0);
}

TEST_CASE("error on bad query", "[sql]") {
    sql::SQLite db(":memory:");
    try {
        db.query("select * from not_a_table", zettel::sql::paramlist{});
        REQUIRE(false);
    } catch (const sql::SQLite::Exception& exc) {
        REQUIRE(strcmp(exc.what(), "Failed to compile query 'select * from not_a_table': no such table: not_a_table") == 0);
    }
}

TEST_CASE("binding errors", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");

    // using unnamed parameters with paramdict should fail
    try {
        db.query("INSERT INTO test (a, b) VALUES (?, ?)", zettel::sql::paramdict{});
        REQUIRE(false);
    } catch (const sql::SQLite::Exception& exc) {
        REQUIRE(strcmp(exc.what(), "Failed to bind parameters to statement: Unnamed parameter at index 1") == 0);
    }

    try {
        db.query("INSERT INTO test (a, b) VALUES (:a, :b)", zettel::sql::paramdict{});
        REQUIRE(false);
    } catch (const sql::SQLite::Exception& exc) {
        REQUIRE(strcmp(exc.what(), "Failed to bind parameters to statement: Missing named parameter at index 1") == 0);
    }

    try {
        db.query("INSERT INTO test (a, b) VALUES (?, ?)", zettel::sql::paramlist{});
        REQUIRE(false);
    } catch (const sql::SQLite::Exception& exc) {
        REQUIRE(strcmp(exc.what(), "Failed to bind parameters to statement: Mismatch between query parameters and number of parameters in list") == 0);
    }
}

TEST_CASE("vector param adapter", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b INT, c INT)");
    std::vector<int> values{10, 20, 30};
    db.query("INSERT INTO test (a, b, c) VALUES (?, ?, ?)", zettel::sql::paramvec<int>{values});

    sql::SQLite::iterator<TestRow3> result = db.query<TestRow3>("SELECT a, b, c FROM test");
    TestRow3 row = sql::one(result);
    REQUIRE(row.a == 10);
    REQUIRE(row.b == 20);
    REQUIRE(row.c == 30);
}

TEST_CASE("delete query", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    db.query("DELETE FROM test");
    sql::SQLite::iterator<int> result = db.query<int>("SELECT count(a) FROM test", [](const std::vector<sqlite3_value*>& row) {
        return sqlite3_value_int(row[0]);
    });
    int count = sql::one(result);
    REQUIRE(count == 0);
}

TEST_CASE("nested transactions", "[sql]") {
    sql::SQLite db(":memory:");
    db.query("CREATE TABLE test (a INT, b TEXT)");
    db.query("INSERT INTO test (a, b) VALUES (1, \"hello\")");
    db.begin();
    db.query("INSERT INTO test (a, b) VALUES (2, \"hello2\")");
    db.begin();
    db.query("INSERT INTO test (a, b) VALUES (3, \"hello3\")");
    db.begin();
    db.query("INSERT INTO test (a, b) VALUES (4, \"hello4\")");
    sql::SQLite::iterator<TestRow> result = db.query<TestRow>("SELECT a, b FROM test ORDER BY a DESC");
    REQUIRE(sql::gather(result).size() == 4);
    db.rollback();
    sql::SQLite::iterator<TestRow> result2 = db.query<TestRow>("SELECT a, b FROM test ORDER BY a DESC");
    REQUIRE(sql::gather(result2).size() == 3);
    db.rollback();
    sql::SQLite::iterator<TestRow> result3 = db.query<TestRow>("SELECT a, b FROM test ORDER BY a DESC");
    REQUIRE(sql::gather(result3).size() == 2);
}
