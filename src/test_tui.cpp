#include <catch2/catch_test_macros.hpp>

#include "tui.hpp"

using namespace tui;

TEST_CASE("pos::move", "[tui]") {
    tui::screen s{20, 20};
    pos p{0, 0};
    REQUIRE(p.move(s, 5) == pos{0, 5});
    REQUIRE(p.move(s, 25) == pos{1, 5});

    p = pos{10, 10};
    REQUIRE(p.move(s, -10) == pos{10, 0});
    REQUIRE(p.move(s, -5000) == pos{0, 0});
    REQUIRE(p.move(s, -20) == pos{9, 10});
    REQUIRE(p.move(s, 5000) == pos{19, 19});
}
