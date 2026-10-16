#include "tui.hpp"

#include <curses.h>

using std::function;

namespace tui {

pos::pos(const screen& scr, int cursorpos) : y(cursorpos / scr.cols), x(cursorpos % scr.cols) {}

int pos::scrpos(const screen& scr) const {
    return (y * scr.cols) + x;
}

pos pos::move(const screen& scr, int amount) const {
    int curpos = scrpos(scr);
    int newpos = std::max(0, std::min((scr.cols * scr.rows) - 1, curpos + amount));
    int newRow = newpos / scr.cols;
    int newCol = newpos % scr.cols;
    return pos{
        newRow,
        newCol
    };
}

bool pos::operator==(const pos& other) const {
    return x == other.x && y == other.y;
}

void run_curses(function<void()> app) {
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
