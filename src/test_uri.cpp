#include <catch2/catch_test_macros.hpp>

#include "uri.hpp"

using zettel::URI;

TEST_CASE("parses basic web address", "[uri]") {
    URI u = URI::parse("https://google.com");
    REQUIRE(u.scheme().compare("https") == 0);
    REQUIRE((u.authority().has_value() && u.authority()->compare("google.com") == 0));
    REQUIRE(u.path().compare("") == 0);
    REQUIRE(!u.query().has_value());
    REQUIRE(!u.fragment().has_value());

}

TEST_CASE("parses full web address", "[uri]") {
    URI u = URI::parse("http://localsite.com/somepart?a=b#anchor");
    REQUIRE(u.scheme().compare("http") == 0);
    REQUIRE((u.authority().has_value() && u.authority()->compare("localsite.com") == 0));
    REQUIRE(u.path().compare("/somepart") == 0);
    REQUIRE((u.query().has_value() && u.query()->compare("a=b") == 0));
    REQUIRE((u.fragment().has_value() && u.fragment()->compare("anchor") == 0));
}

TEST_CASE("path with single leading slash is not interpreted as authority", "[uri]") {
    URI u = URI::parse("scheme:/path");
    REQUIRE(u.scheme().compare("scheme") == 0);
    REQUIRE(!u.authority().has_value());
    REQUIRE(u.path().compare("/path") == 0);
}

TEST_CASE("edge case - empty authority", "[uri]") {
    URI u = URI::parse("scheme://");
    REQUIRE(u.scheme().compare("scheme") == 0);
    REQUIRE((u.authority().has_value() && u.authority()->empty()));
}
