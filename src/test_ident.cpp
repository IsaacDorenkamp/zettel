#include <catch2/catch_test_macros.hpp>

#include <iostream>

#include "ident.hpp"

using namespace zettel;
using namespace std;

TEST_CASE("Numeric ID comparisons are correct", "[id]") {
    REQUIRE(NumericId(1) < NumericId(2));
    REQUIRE(NumericId(1) == NumericId(1));
}

TEST_CASE("Classic ID comparisons are correct", "[id]") {
    REQUIRE(ClassicId({1}) < ClassicId({1, 1}));
    REQUIRE(ClassicId({1, 2, 3}) == ClassicId({1, 2, 3}));
    REQUIRE(!(ClassicId({1, 2, 3}) < ClassicId({1, 2})));
}

TEST_CASE("Incongruent comparisons defer to type", "[id]") {
    REQUIRE(NumericId(1) < ClassicId({1}));
    REQUIRE(!(ClassicId({1}) < NumericId(1)));
}

TEST_CASE("Representations generate correctly", "[id]") {
    cout << NumericId(1).represent() << endl;
    REQUIRE(NumericId(1).represent().compare("1") == 0);
    REQUIRE(ClassicId({1}).represent().compare("1") == 0);
    cout << ClassicId({1, 0, 1}).represent() << endl;
    REQUIRE(ClassicId({1, 0, 1}).represent().compare("1a1") == 0);
}
