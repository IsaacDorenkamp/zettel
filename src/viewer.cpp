#include "viewer.hpp"

#include <curses.h>
#include <stdexcept>

#include "commands.hpp"
#include "content.hpp"
#include "format.hpp"
#include "uri.hpp"
#include "zettel.hpp"

using std::string, std::unique_ptr;
using zettel::cmd::Context;

namespace zettel {

Viewer::Viewer(const Zettel* zettel) : m_zettel(zettel), m_screen(), m_links(), m_chain(), m_cache(), m_link(-1) {}
void Viewer::configure() {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    curs_set(0);
}
void Viewer::setZettel(const Zettel* zettel) {
    m_zettel = zettel;
    m_links.clear();
    m_link = -1;

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
            int attr = A_UNDERLINE;
            tui::range trange{ here, here.move(m_screen, displayText.size()) };
            try {
                URI uri = URI::parse(ref->uri());
                m_links.push_back({uri, trange});
                attr |= COLOR_PAIR(1);
            } catch (const URI::Exception& exc) {
                m_links.push_back({URI(), trange});
                attr |= COLOR_PAIR(3);
            }
            attron(attr);
            err = printw("%s", displayText.c_str());
            attroff(attr);
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
    m_cache.insert({ zettel->id(), *zettel });
    setZettel(zettel);
    drawKeyBinds();
    drawTrail();
}

void Viewer::chainPop() {
    if (m_chain.size() > 1) {
        m_chain.pop_back();
        setZettel(&m_cache.at(m_chain.back()));
        drawKeyBinds();
        drawTrail();
    }
}

void Viewer::run(const Context& ctx) {
    configure();
    chainPush(m_zettel);
    drawKeyBinds();
    drawTrail();

    int ch;
    bool running = true;
    while (running) {
        ch = getch();
        switch (ch) {
        case 'k':
            nextLink();
            drawKeyBinds();
            refresh();
            break;
        case 'K':
            deselectCurrentLink();
            drawKeyBinds();
            refresh();
            break;
        case 'b':
            chainPop();
            refresh();
        case '\n':
            if (m_link >= 0) {
                URI uri = m_links[m_link].first;
                open(ctx, uri);
            }
            break;
        }
    }
}

void Viewer::error(string message) {
    move(m_screen.rows - 2, 0);
    attron(COLOR_PAIR(3) | A_BOLD);
    addstr("ERROR ");
    attroff(COLOR_PAIR(3) | A_BOLD);
    addstr(message.substr(0, m_screen.cols - 6).c_str());
}

void Viewer::open(const Context& ctx, const URI& uri) {
    if (uri.scheme().compare("zettel") == 0) {
        if (uri.authority().has_value()) {
            error("Zettel URI should not have an authority.");
            return;
        }
        if (uri.query().has_value()) {
            error("Zettel URI should not have a query string.");
            return;
        }
        // NOTE: May add support for fragments in the future.
        if (uri.fragment().has_value()) {
            error("Zettel URI should not have a fragment.");
            return;
        }
        unsigned long id;
        try {
            id = std::stoul(uri.path());
            if (id > 0xFFFFFFFF) throw std::out_of_range(fmt("%lu exceeds 0xFFFFFFFF", id));
        } catch (const std::invalid_argument& exc) {
            error(fmt("Invalid Zettel ID: %s", uri.path().c_str()));
            return;
        } catch (const std::out_of_range& exc) {
            error(fmt("Zettel ID out of range: %s", exc.what()));
            return;
        }
        string filename = Zettel::getFilenameForId((uint32_t)id);
        std::filesystem::path zettelPath = ctx.root / filename;
        try {
            Zettel z = Zettel::load(zettelPath);
            chainPush(&z);
        } catch (const ZettelException& exc) {
            error(fmt("Couldn't load Zettel: %s", exc.what()));
        }
    } else {
        // TODO: defer to the user's machine
    }
}

void Viewer::deselectCurrentLink() {
    int pos, index;
    tui::pos scrpos{0, 0};
    if (m_link != -1) {
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

void Viewer::drawTrail() {
    move(0, 1);
    // TODO: What if this gets too long?
    Zettel::Id id;
    const Zettel* z;
    for (size_t index = 0; index < m_chain.size(); index++) {
        id = m_chain[index];
        z = &m_cache.at(id);
        if (index > 0) addstr(" > ");
        attron(A_ITALIC);
        addstr(z->title().c_str());
        attroff(A_ITALIC);
    }
}

void Viewer::drawKeyBinds() {
    move(m_screen.rows - 1, 1);
    clrtoeol();
    if (m_links.size() == 0) attron(A_DIM);
    addstr("k [next link]");
    attroff(A_DIM);
    addstr("   ");
    if (m_link < 0) attron(A_DIM);
    addstr("K [deselect link]   <Return> [follow link]");
    attroff(A_DIM);
    if (m_chain.size() <= 1) attron(A_DIM);
    addstr("   b [go to previous zettel]");
    attroff(A_DIM);
    addstr("   ^C [quit]");
}

}
