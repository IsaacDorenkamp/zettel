#pragma once

#include <functional>

namespace tui {

struct screen {
    int rows;
    int cols;
};

struct pos {
    int y;
    int x;

    explicit pos(const screen& scr, int cursorpos);
    pos(int y, int x) : y(y), x(x) {}

    int scrpos(const screen& scr) const;
    pos move(const screen& scr, int amount) const;

    bool operator==(const pos& other) const;
};

struct range {
    pos start;
    pos end;
};

void run_curses(std::function<void()> app);

}
