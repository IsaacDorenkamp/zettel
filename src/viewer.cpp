#include "viewer.hpp"

#include <curses.h>
#include <algorithm>
#include <stdexcept>

#include "commands.hpp"
#include "content.hpp"
#include "format.hpp"
#include "logging.hpp"
#include "sys.hpp"
#include "uri.hpp"
#include "zettel.hpp"

using std::map, std::pair, std::set, std::string, std::unique_ptr;
using zettel::cmd::Context;

namespace zettel {

Viewer::Viewer(const Zettel* zettel, const Context& ctx) : m_ctx(&ctx), m_top(), m_main(), m_bottom(), m_zettel(zettel), m_screen(), m_mainScreen(), m_links(), m_chain(), m_cache(), m_link(-1) {}
void Viewer::configure() {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    curs_set(0);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    m_screen = tui::screen{rows, cols};

    m_top = newwin(2, m_screen.cols - 2, 0, 1);
    m_main = newwin(m_screen.rows - 4, m_screen.cols - 2, 2, 1);
    m_bottom = newwin(2, m_screen.cols - 2, m_screen.rows - 2, 1);
    m_mainScreen = tui::screen{getmaxy(m_main), getmaxx(m_main)};
}

void Viewer::onResize() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    clear();
    refresh();
    m_screen = tui::screen{rows, cols};

    delwin(m_top);
    delwin(m_main);
    delwin(m_bottom);

    m_top = newwin(2, m_screen.cols - 2, 0, 1);
    m_main = newwin(m_screen.rows - 4, m_screen.cols - 2, 2, 1);
    m_bottom = newwin(2, m_screen.cols - 2, m_screen.rows - 2, 1);
    m_mainScreen = tui::screen{getmaxy(m_main), getmaxx(m_main)};

    setZettel(&m_cache.at(m_chain.back()));
    drawKeyBinds();
    drawTrail();
}

void Viewer::setZettel(const Zettel* zettel) {
    m_links.clear();
    m_link = -1;
    wclear(m_main);

    wmove(m_main, 0, 0);
    wattron(m_main, A_BOLD);
    waddstr(m_main, zettel->title().c_str());
    waddch(m_main, '\n');
    wattroff(m_main, A_BOLD);
    int i;
    for (i = 0; i < std::min((size_t)m_mainScreen.cols, zettel->title().size()); i++) waddch(m_main, '=');
    if (i != m_mainScreen.cols) waddch(m_main, '\n');

    ContentBlock* block;
    ReferenceBlock* ref;
    string displayText;
    int err, y, x;
    bool refs = true;
    for (const unique_ptr<ContentBlock>& block_ptr : zettel->content()) {
        block = block_ptr.get();
        if ((ref = dynamic_cast<ReferenceBlock*>(block))) {
            displayText = ref->text().value_or(ref->uri());
            y = getcury(m_main);
            x = getcurx(m_main);
            tui::pos here{y, x};
            int attr = A_UNDERLINE;
            tui::range trange{ here, here.move(m_mainScreen, displayText.size()) };
            try {
                URI uri = URI::parse(ref->uri());
                m_links.push_back({uri, trange});
                attr |= COLOR_PAIR(1);
            } catch (const URI::Exception& exc) {
                m_links.push_back({URI(), trange});
                attr |= COLOR_PAIR(3);
            }
            wattron(m_main, attr);
            err = wprintw(m_main, "%s", displayText.c_str());
            wattroff(m_main, attr);
        } else {
            err = wprintw(m_main, "%s", block->format(PLAINTEXT_FORMAT).c_str());
        }
        // if the text doesn't fit, stop
        // TODO: switch to pad, add scrolling
        if (err == ERR) {
            refs = false;
            break;
        }
    }
    if (refs) {
        set<Zettel::Id> ids = m_ctx->index->getReferencesTo(zettel->id());
        if (ids.size()) {
            waddstr(m_main, "\nReferences: ");
            map<Zettel::Id, models::zettel> linked = m_ctx->index->lookup(ids);
            bool first = true;
            for (const pair<Zettel::Id, models::zettel>& entry : linked) {
                if (first) first = false;
                else waddstr(m_main, " ... ");
                y = getcury(m_main);
                x = getcurx(m_main);
                tui::pos here{y, x};
                tui::range trange{ here, here.move(m_mainScreen, entry.second.title.size()) };
                m_links.push_back({URI("zettel", std::nullopt, std::to_string(entry.first), std::nullopt, std::nullopt), trange});
                wattron(m_main, A_UNDERLINE | COLOR_PAIR(1));
                err = waddstr(m_main, entry.second.title.c_str());
                wattroff(m_main, A_UNDERLINE | COLOR_PAIR(1));
                if (err) break;
            }
        }
    }
    wrefresh(m_main);
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

void Viewer::run() {
    configure();
    wrefresh(stdscr);
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
            break;
        case 'K':
            deselectCurrentLink();
            m_link = -1;
            drawKeyBinds();
            break;
        case 'b':
            chainPop();
            break;
        case '\n':
            if (m_link >= 0) {
                URI uri = m_links[m_link].first;
                open(uri);
            }
            break;
        case KEY_RESIZE:
            onResize();
            break;
        }
    }
}

void Viewer::error(string message) {
    wmove(m_bottom, 0, 0);
    wattron(m_bottom, COLOR_PAIR(3) | A_BOLD);
    waddstr(m_bottom, "ERROR ");
    wattroff(m_bottom, COLOR_PAIR(3) | A_BOLD);
    waddstr(m_bottom, message.substr(0, m_screen.cols - 6).c_str());
    wrefresh(m_bottom);
}

void Viewer::open(const URI& uri) {
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
        if (std::find(m_chain.begin(), m_chain.end(), id) != m_chain.end()) {
            while (m_chain.back() != id) chainPop();
            return;
        }

        Zettel* z = getZettel(id);
        if (z) chainPush(z);
    } else {
        sys::ExitResult result = sys::open(uri);
        if (result.status > 0 || result.signal >= 0) {
            logs::error("Unable to open URI %s", uri.represent().c_str());
        }
    }
}

Zettel* Viewer::getZettel(const Zettel::Id& id) {
    map<Zettel::Id, Zettel>::iterator it = m_cache.find(id);
    if (it == m_cache.end()) {
        try {
            it = m_cache.insert({ id, Zettel::load(m_ctx->root / Zettel::getFilenameForId(id)) }).first;
        } catch (const ZettelException& exc) {
            error(fmt("Unable to load Zettel %u: %s", id, exc.what()));
            return nullptr;
        }
    }
    return &it->second;
}

void Viewer::deselectCurrentLink() {
    int pos, index;
    tui::pos scrpos{0, 0};
    if (m_link != -1) {
        tui::range current = m_links[m_link].second;
        wmove(m_main, current.start.y, current.start.x);
        wattron(m_main, COLOR_PAIR(1) | A_BOLD);
        for (pos = current.start.scrpos(m_mainScreen), index = 0; pos < current.end.scrpos(m_mainScreen); pos++, index++) {
            scrpos = tui::pos(m_mainScreen, pos);
            waddch(m_main, mvwinch(m_main, scrpos.y, scrpos.x) & A_CHARTEXT | A_UNDERLINE);
        }
        wattroff(m_main, COLOR_PAIR(1) | A_BOLD);
    }
    wrefresh(m_main);
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
    wattron(m_main, COLOR_PAIR(2));
    wattroff(m_main, A_UNDERLINE);
    tui::range current = m_links[m_link].second;
    wmove(m_main, current.start.y, current.start.x);
    for (pos = current.start.scrpos(m_mainScreen), index = 0; pos < current.end.scrpos(m_mainScreen); pos++, index++) {
        scrpos = tui::pos(m_mainScreen, pos);
        waddch(m_main, mvwinch(m_main, scrpos.y, scrpos.x) & A_CHARTEXT);
    }
    wattroff(m_main, COLOR_PAIR(2));
    wrefresh(m_main);
}

void Viewer::drawTrail() {
    wmove(m_top, 0, 0);
    wclrtoeol(m_top);
    // TODO: What if this gets too long?
    Zettel::Id id;
    const Zettel* z;
    for (size_t index = 0; index < m_chain.size(); index++) {
        id = m_chain[index];
        z = &m_cache.at(id);
        if (index > 0) waddstr(m_top, " > ");
        wattron(m_top, A_ITALIC);
        waddstr(m_top, z->title().c_str());
        wattroff(m_top, A_ITALIC);
    }
    wrefresh(m_top);
}

void Viewer::drawKeyBinds() {
    wmove(m_bottom, 1, 0);
    wclrtoeol(m_bottom);
    if (m_links.size() == 0) wattron(m_bottom, A_DIM);
    waddstr(m_bottom, "k [next link]");
    wattroff(m_bottom, A_DIM);
    waddstr(m_bottom, "   ");
    if (m_link < 0) wattron(m_bottom, A_DIM);
    waddstr(m_bottom, "K [deselect link]   <Return> [follow link]");
    wattroff(m_bottom, A_DIM);
    if (m_chain.size() <= 1) wattron(m_bottom, A_DIM);
    waddstr(m_bottom, "   b [go to previous zettel]");
    wattroff(m_bottom, A_DIM);
    waddstr(m_bottom, "   ^C [quit]");
    wrefresh(m_bottom);
}

}
