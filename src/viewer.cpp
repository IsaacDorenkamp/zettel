#include "viewer.hpp"

#include <curses.h>

#include "content.hpp"
#include "format.hpp"

using std::string, std::unique_ptr;

namespace zettel {

Viewer::Viewer(const Zettel* zettel) : m_zettel(zettel), m_screen(), m_links(), m_chain(), m_cache(), m_link(-1) {}
void Viewer::configure() {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    curs_set(0);
}
void Viewer::setZettel(const Zettel* zettel) {
    m_zettel = zettel;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    m_screen = tui::screen{rows, cols};

    clear();

    move(2, 0);
    attron(A_BOLD);
    addstr(zettel->title().c_str());
    addch('\n');
    attroff(A_BOLD);
    int i;
    for (i = 0; i < std::min((size_t)cols, zettel->title().size()); i++) addch('=');
    if (i != cols) addch('\n');

    ContentBlock* block;
    ReferenceBlock* ref;
    string displayText;
    int err, y, x;
    for (const unique_ptr<ContentBlock>& block_ptr : zettel->content()) {
        block = block_ptr.get();
        if ((ref = dynamic_cast<ReferenceBlock*>(block))) {
            displayText = ref->text().value_or(ref->uri());
            getyx(stdscr, y, x);
            tui::pos here{y, x};
            m_links.push_back({ref->uri(), tui::range{ here, here.move(m_screen, displayText.size()) }});
            attron(A_UNDERLINE | COLOR_PAIR(1));
            err = printw("%s", displayText.c_str());
            attroff(A_UNDERLINE | COLOR_PAIR(1));
        } else {
            err = printw("%s", block->format(PLAINTEXT_FORMAT).c_str());
        }
        // if the text doesn't fit, stop
        if (err == ERR) break;
    }
    refresh();
}

void Viewer::chainPush(const Zettel* zettel) {
    m_chain.push_back(zettel->id());
    setZettel(zettel);
    drawKeyBinds();
    m_cache.insert({ zettel->id(), *zettel });
}

void Viewer::chainPop() {
    if (m_chain.size() > 0) {
        m_chain.pop_back();
        if (m_chain.size() > 0) setZettel(&m_cache.at(m_chain.back()));
        else {
            clear();
            drawKeyBinds();
        }
    }
}

void Viewer::run() {
    configure();
    setZettel(m_zettel);
    drawKeyBinds();

    int ch;
    bool running = true;
    while (running) {
        ch = getch();
        switch (ch) {
        case 'k':
            nextLink();
            drawKeyBinds();
            break;
        case 'K':
            deselectCurrentLink();
            drawKeyBinds();
            refresh();
            break;
        }
    }
}

void Viewer::open(const ReferenceBlock* ref) {
    // TODO: implement
}

void Viewer::deselectCurrentLink() {
    int pos, index;
    tui::pos scrpos{0, 0};
    if (m_link != -1) {
        string uri = m_links[m_link].first;
        tui::range current = m_links[m_link].second;
        move(current.start.y, current.start.x);
        attron(COLOR_PAIR(1) | A_BOLD);
        for (pos = current.start.scrpos(m_screen), index = 0; pos < current.end.scrpos(m_screen); pos++, index++) {
            scrpos = tui::pos(m_screen, pos);
            addch(mvinch(scrpos.y, scrpos.x) & A_CHARTEXT | A_UNDERLINE);
        }
        attroff(COLOR_PAIR(1) | A_BOLD);
        m_link = -1;
    }
}

void Viewer::nextLink() {
    if (m_links.size() == 0) {
        m_link = -1;
        return;
    }
    deselectCurrentLink();
    int pos, index;
    tui::pos scrpos{0, 0};
    m_link = (m_link + 1) % m_links.size();
    attron(COLOR_PAIR(2));
    attroff(A_UNDERLINE);
    tui::range current = m_links[m_link].second;
    move(current.start.y, current.start.x);
    for (pos = current.start.scrpos(m_screen), index = 0; pos < current.end.scrpos(m_screen); pos++, index++) {
        scrpos = tui::pos(m_screen, pos);
        addch(mvinch(scrpos.y, scrpos.x) & A_CHARTEXT);
    }
    attroff(COLOR_PAIR(2));
}

void Viewer::drawKeyBinds() {
    move(m_screen.rows - 1, 0);
    clrtoeol();
    addstr(" ");
    if (m_links.size() == 0) attron(A_DIM);
    addstr("k -> next link");
    attroff(A_DIM);
    addstr("   ");
    if (m_link < 0) attron(A_DIM);
    addstr("K [deselect link]   <Return> [follow link]");
    attroff(A_DIM);
    addstr("   ^C [quit]");
}

}
