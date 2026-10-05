#include <catch2/catch_test_macros.hpp>

#include "ident.hpp"

using namespace zettel;
using namespace std;

TEST_CASE("Numeric ID comparisons are correct", "[id]") {
    REQUIRE(NumericId(1) < NumericId(2));
    REQUIRE(NumericId(1) == NumericId(1));
}
