#pragma once

#include <cstdint>

namespace zettel {

enum class DisplayMode {
    ASCII, ANSI
};

struct FormatOptions {
    DisplayMode mode;
    uint16_t line_size;
    uint16_t first_line_offset;
};


}
