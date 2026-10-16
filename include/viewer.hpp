#pragma once

#include <ncurses.h>

#include "tui.hpp"
#include "zettel.hpp"

namespace zettel {

class Viewer {
public:
    Viewer(const Zettel* zettel);
    virtual ~Viewer() = default;

    void configure();
    void setZettel(const Zettel* zettel);
    void run();
private:
    void nextLink();

    const Zettel* m_zettel;
    tui::screen m_screen;
    std::vector<std::pair<std::string, tui::range>> m_links;
    int m_link;
};

}
