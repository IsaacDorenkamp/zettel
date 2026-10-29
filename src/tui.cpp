#include "tui.hpp"

#include <curses.h>
#include <algorithm>
#include <map>

#define map_color(ansi_color, target_var, default_value) switch (ansi_color) {\
case ansi::Color::BLACK:\
    target_var = COLOR_BLACK;\
    break;\
case ansi::Color::RED:\
    target_var = COLOR_RED;\
    break;\
case ansi::Color::GREEN:\
    target_var = COLOR_GREEN;\
    break;\
case ansi::Color::YELLOW:\
    target_var = COLOR_YELLOW;\
    break;\
case ansi::Color::BLUE:\
    target_var = COLOR_BLUE;\
    break;\
case ansi::Color::MAGENTA:\
    target_var = COLOR_MAGENTA;\
    break;\
case ansi::Color::CYAN:\
    target_var = COLOR_CYAN;\
    break;\
case ansi::Color::WHITE:\
    target_var = COLOR_WHITE;\
    break;\
default:\
    target_var = default_value;\
    break;\
}

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

int register_color_pair(int fg, int bg) {
    static int pair = 1;
    init_pair(pair, fg, bg);
    return pair++;
}

void render_ansi(WINDOW* win, const ansi::block& block) {
    // TODO: enable strikethrough with ANSI workaround
    static std::map<std::pair<int, int>, int> color_attrs;
    int attr = (block.bold() ? A_BOLD : 0) | (block.italic() ? A_ITALIC : 0) | (block.dim() ? A_DIM : 0) | (block.underline() ? A_UNDERLINE : 0)
        | (block.blink() ? A_BLINK : 0) | (block.inverted() ? A_REVERSE : 0) | (block.invisible() ? A_INVIS : 0);

    int fg = -1, bg = -1;
    // need better defaults
    map_color(block.foreground(), fg, COLOR_WHITE);
    map_color(block.background(), bg, COLOR_BLACK);
    std::map<std::pair<int, int>, int>::const_iterator it = color_attrs.find({fg, bg});
    if (it != color_attrs.end()) {
        attr |= COLOR_PAIR(it->second);
    } else {
        int pairid = register_color_pair(fg, bg);
        std::pair<int, int> key{fg, bg};
        color_attrs[key] = pairid;
        attr |= COLOR_PAIR(pairid);
    }
    wattron(win, attr);
    waddstr(win, block.text().c_str());
    wattroff(win, attr);
}

#undef map_color

}
