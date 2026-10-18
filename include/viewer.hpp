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
    Viewer(const Zettel* zettel);
    virtual ~Viewer() = default;

    void configure();
    void setZettel(const Zettel* zettel);
    void chainPush(const Zettel* zettel);
    void chainPop();
    void run(const cmd::Context& ctx);
private:
    void error(std::string msg);
    void deselectCurrentLink();
    void nextLink();
    void drawTrail();
    void drawKeyBinds();
    void open(const cmd::Context& ctx, const URI& uri);

    const Zettel* m_zettel;
    tui::screen m_screen;
    std::vector<std::pair<URI, tui::range>> m_links;
    std::vector<Zettel::Id> m_chain;
    std::map<Zettel::Id, Zettel> m_cache;
    int m_link;
};

}
