#include <catch2/catch_test_macros.hpp>

#include "search.hpp"

using std::optional, std::string;
using zettel::Matcher, zettel::search, zettel::SearchType;

TEST_CASE("returns empty result when there is no match", "[search]") {
    optional<Matcher::Match> result = search("this is the source", "doesn't exist", SearchType::SubstringCI);
    REQUIRE(!result.has_value());
}

TEST_CASE("finds result with matching case", "[search]") {
    optional<Matcher::Match> result = search("this is the source", "source", SearchType::SubstringCI);
    REQUIRE(result.has_value());
    REQUIRE(result->first == 12);
    REQUIRE(result->second.compare("source") == 0);
}

TEST_CASE("finds result with non-matching case", "[search]") {
    optional<Matcher::Match> result = search("this is the SoUrCe", "source", SearchType::SubstringCI);
    REQUIRE(result.has_value());
    REQUIRE(result->first == 12);
    REQUIRE(result->second.compare("SoUrCe") == 0);
}

TEST_CASE("correctly ignores partial match", "[search]") {
    optional<Matcher::Match> result = search("this is the sour - wait, this is the source", "source", SearchType::SubstringCI);
    REQUIRE(result.has_value());
    REQUIRE(result->first == 37);
    REQUIRE(result->second.compare("source") == 0);
}

TEST_CASE("correctly identifies match after several partial matches", "[search]") {
    optional<Matcher::Match> result = search("ssssource", "source", SearchType::SubstringCI);
    REQUIRE(result.has_value());
    REQUIRE(result->first == 3);
    REQUIRE(result->second.compare("source") == 0);

    result = search("sssource", "source", SearchType::SubstringCI);
    REQUIRE(result.has_value());
    REQUIRE(result->first == 2);
    REQUIRE(result->second.compare("source") == 0);
}
