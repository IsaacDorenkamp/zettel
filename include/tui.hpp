#pragma once

#include <curses.h>
#include <exception>
#include <type_traits>

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


template <class Fn, std::enable_if_t<std::is_invocable<Fn>::value, bool> = true>
void run_curses(const Fn& app) {
    initscr();
    noecho();
    cbreak();
    start_color();
    keypad(stdscr, true);

    try {
        app();
    } catch (const std::exception& exc) {
        /* ignore any errors */
    }

    endwin();
}

}
