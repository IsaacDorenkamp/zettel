#pragma once

#include <string>

namespace ansi {

enum class Color {
    NONE = 0, BLACK, RED, GREEN, YELLOW, BLUE,
    MAGENTA, CYAN, WHITE, DEFAULT
};

class ANSIBlock {
public:
    ANSIBlock(std::string text);

    ANSIBlock& bold(bool isBold);
    ANSIBlock& italic(bool isItalic);
    ANSIBlock& dim(bool isDim);
    ANSIBlock& underline(bool isUnderlined);
    ANSIBlock& blink(bool isBlinking);
    ANSIBlock& inverted(bool isInverted);
    ANSIBlock& invisible(bool isInvisible);
    ANSIBlock& strikethrough(bool isStruck);
    ANSIBlock& foreground(Color fg);
    ANSIBlock& background(Color bg);

    bool bold() const;
    bool italic() const;
    bool dim() const;
    bool underline() const;
    bool blink() const;
    bool inverted() const;
    bool invisible() const;
    bool strikethrough() const;
    Color foreground() const;
    Color background() const;

    std::string toString() const;

    static unsigned char BOLD;
    static unsigned char ITALIC;
    static unsigned char DIM;
    static unsigned char UNDERLINE;
    static unsigned char BLINK;
    static unsigned char INVERTED;
    static unsigned char INVISIBLE;
    static unsigned char STRIKETHROUGH;

    friend std::ostream& operator<<(std::ostream& stream, const ANSIBlock& block);
private:
    std::string m_text;
    unsigned char m_flags;
    Color m_fg;
    Color m_bg;
};

typedef ANSIBlock block;

}
