#include "ansi.hpp"

#include <optional>
#include <sstream>

#include <iostream>

using std::optional, std::ostream, std::string, std::stringstream;

namespace ansi {

ANSIBlock::ANSIBlock(string text) : m_text(text), m_flags(0), m_fg(Color::NONE), m_bg(Color::NONE) {}

ANSIBlock& ANSIBlock::bold(bool isBold) {
    m_flags ^= isBold ? ANSIBlock::BOLD : 0;
    return *this;
}

ANSIBlock& ANSIBlock::italic(bool isItalic) {
    m_flags ^= isItalic ? ANSIBlock::ITALIC : 0;
    return *this;
}

ANSIBlock& ANSIBlock::dim(bool isDim) {
    m_flags ^= isDim ? ANSIBlock::DIM : 0;
    return *this;
}

ANSIBlock& ANSIBlock::underline(bool isUnderlined) {
    m_flags ^= isUnderlined ? ANSIBlock::UNDERLINE : 0;
    return *this;
}

ANSIBlock& ANSIBlock::blink(bool isBlinking) {
    m_flags ^= isBlinking ? ANSIBlock::BLINK : 0;
    return *this;
}

ANSIBlock& ANSIBlock::inverted(bool isInverted) {
    m_flags ^= isInverted ? ANSIBlock::INVERTED : 0;
    return *this;
}

ANSIBlock& ANSIBlock::invisible(bool isInvisible) {
    m_flags ^= isInvisible ? ANSIBlock::INVISIBLE : 0;
    return *this;
}

ANSIBlock& ANSIBlock::strikethrough(bool isStruck) {
    m_flags ^= isStruck ? ANSIBlock::STRIKETHROUGH : 0;
    return *this;
}

ANSIBlock& ANSIBlock::foreground(Color fg) {
    m_fg = fg;
    return *this;
}

ANSIBlock& ANSIBlock::background(Color bg) {
    m_bg = bg;
    return *this;
}

bool ANSIBlock::bold() const {
    return (m_flags & ANSIBlock::BOLD) != 0;
}

bool ANSIBlock::italic() const {
    return (m_flags & ANSIBlock::ITALIC) != 0;
}

bool ANSIBlock::dim() const {
    return (m_flags & ANSIBlock::DIM) != 0;
}

bool ANSIBlock::underline() const {
    return (m_flags & ANSIBlock::UNDERLINE) != 0;
}

bool ANSIBlock::blink() const {
    return (m_flags & ANSIBlock::BLINK) != 0;
}

bool ANSIBlock::inverted() const {
    return (m_flags & ANSIBlock::INVERTED) != 0;
}

bool ANSIBlock::invisible() const {
    return (m_flags & ANSIBlock::INVISIBLE) != 0;
}

bool ANSIBlock::strikethrough() const {
    return (m_flags & ANSIBlock::STRIKETHROUGH) != 0;
}

string ANSIBlock::toString() const {
    if (m_flags == 0 && m_fg == Color::NONE && m_bg == Color::NONE) return m_text;

    std::stringstream result;
    result << '\x1B' << '[';

    bool first = true;
    if (bold()) {
        first = false;
        result << '1';
    }
    if (italic()) {
        if (first) first = false;
        else result << ';';
        result << '3';
    }
    if (dim()) {
        if (first) first = false;
        else result << ';';
        result << '2';
    }
    if (underline()) {
        if (first) first = false;
        else result << ';';
        result << '4';
    }
    if (blink()) {
        if (first) first = false;
        else result << ';';
        result << '5';
    }
    if (inverted()) {
        if (first) first = false;
        else result << ';';
        result << '6';
    }
    if (invisible()) {
        if (first) first = false;
        else result << ';';
        result << '7';
    }
    if (strikethrough()) {
        if (first) first = false;
        else result << ';';
        result << '8';
    }

    // foreground color
    optional<string> str;
    switch (m_fg) {
    case Color::NONE:
        break;
    case Color::BLACK:
        str = "30";
        break;
    case Color::RED:
        str = "31";
        break;
    case Color::GREEN:
        str = "32";
        break;
    case Color::YELLOW:
        str = "33";
        break;
    case Color::BLUE:
        str = "34";
        break;
    case Color::MAGENTA:
        str = "35";
        break;
    case Color::CYAN:
        str = "36";
        break;
    case Color::WHITE:
        str = "37";
        break;
    case Color::DEFAULT:
        str = "39";
        break;
    }

    if (str) {
        if (first) first = false;
        else result << ';';
        result << *str;
    }

    // background color
    switch (m_bg) {
    case Color::NONE:
        str = optional<string>();
        break;
    case Color::BLACK:
        str = "40";
        break;
    case Color::RED:
        str = "41";
        break;
    case Color::GREEN:
        str = "42";
        break;
    case Color::YELLOW:
        str = "43";
        break;
    case Color::BLUE:
        str = "44";
        break;
    case Color::MAGENTA:
        str = "45";
        break;
    case Color::CYAN:
        str = "46";
        break;
    case Color::WHITE:
        str = "47";
        break;
    case Color::DEFAULT:
        str = "49";
        break;
    }

    if (str) {
        if (first) first = false;
        else result << ';';
        result << *str;
    }

    result << 'm';
    result << m_text;
    result << '\x1B' << "[0m";

    return result.str();
}

unsigned char ANSIBlock::BOLD = 1;
unsigned char ANSIBlock::ITALIC = 2;
unsigned char ANSIBlock::DIM = 4;
unsigned char ANSIBlock::UNDERLINE = 8;
unsigned char ANSIBlock::BLINK = 16;
unsigned char ANSIBlock::INVERTED = 32;
unsigned char ANSIBlock::INVISIBLE = 64;
unsigned char ANSIBlock::STRIKETHROUGH = 128;

ostream& operator<<(ostream& stream, const ANSIBlock& block) {
    return stream << block.toString();
}

}
