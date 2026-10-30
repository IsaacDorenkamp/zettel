#include <catch2/catch_test_macros.hpp>

#include "search.hpp"

using std::string;
using zettel::search, zettel::SearchResult, zettel::SearchType;

TEST_CASE("returns empty result when there is no match", "[search]") {
    SearchResult result = search("this is the source", "doesn't exist", SearchType::TermCI);
    REQUIRE(result.first == string::npos);
    REQUIRE(result.second.empty());
}

TEST_CASE("finds result with matching case", "[search]") {
    SearchResult result = search("this is the source", "source", SearchType::TermCI);
    REQUIRE(result.first == 12);
    REQUIRE(result.second.compare("source") == 0);
}

TEST_CASE("finds result with non-matching case", "[search]") {
    SearchResult result = search("this is the SoUrCe", "source", SearchType::TermCI);
    REQUIRE(result.first == 12);
    REQUIRE(result.second.compare("SoUrCe") == 0);
}

TEST_CASE("correctly ignores partial match", "[search]") {
    SearchResult result = search("this is the sour - wait, this is the source", "source", SearchType::TermCI);
    REQUIRE(result.first == 37);
    REQUIRE(result.second.compare("source") == 0);
}

TEST_CASE("correctly identifies match after several partial matches", "[search]") {
    SearchResult result = search("ssssource", "source", SearchType::TermCI);
    REQUIRE(result.first == 3);
    REQUIRE(result.second.compare("source") == 0);

    result = search("sssource", "source", SearchType::TermCI);
    REQUIRE(result.first == 2);
    REQUIRE(result.second.compare("source") == 0);
}
