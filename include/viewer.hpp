#pragma once

#include <ncurses.h>
#include <map>

#include "tui.hpp"
#include "uri.hpp"
#include "zettel.hpp"

namespace zettel {

namespace cmd {

struct Context;

}

class Viewer {
public:
    Viewer(const Zettel* zettel, const cmd::Context& ctx);
    virtual ~Viewer() = default;

    void configure();
    void onResize();
    void setZettel(const Zettel* zettel);
    void chainPush(const Zettel* zettel);
    void chainPop();
    void run();
private:
    void error(std::string msg);
    void deselectCurrentLink();
    void nextLink();
    void drawTrail();
    void drawKeyBinds();
    void open(const URI& uri);

    Zettel* getZettel(const Zettel::Id& id);

    WINDOW* m_top;
    WINDOW* m_main;
    WINDOW* m_bottom;

    const cmd::Context* m_ctx;
    const Zettel* m_zettel;
    tui::screen m_screen;
    tui::screen m_mainScreen;
    std::vector<std::pair<URI, tui::range>> m_links;
    std::vector<Zettel::Id> m_chain;
    std::map<Zettel::Id, Zettel> m_cache;
    int m_link;

    int m_linkDefault;
    int m_linkHover;
    int m_bad;
};

}
