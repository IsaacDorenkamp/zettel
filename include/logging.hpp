#pragma once

#include <iostream>
#include <string>

#include "ansi.hpp"
#include "format.hpp"

namespace log {

enum Level {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

template <typename... Args>
void write(Level level, std::string message, Args&&... values) {
    ansi::ANSIBlock kind;
    kind.bold();
    switch (level) {
    case DEBUG:
        kind.setText("DEBUG");
        break;
    case INFO:
        kind.setText("INFO");
        kind.foreground(ansi::Color::BLUE);
        break;
    case WARNING:
        kind.setText("WARNING");
        kind.foreground(ansi::Color::YELLOW);
        break;
    case ERROR:
        kind.setText("ERROR");
        kind.foreground(ansi::Color::RED);
        break;
    }
    std::cout << kind << " " << zettel::fmt(message, std::forward<delctype(values)>(values)...) << std::endl;
}

#define LOG_WRAPPER(fn_name, log_level) template <typename... Args>\
void fn_name(std::string message, Args&&... values) {\
    write(log_level, message, std::forward<decltype(values)>(values)...);\
}\

LOG_WRAPPER(debug, DEBUG);
LOG_WRAPPER(info, INFO);
LOG_WRAPPER(warn, WARNING);
LOG_WRAPPER(error, ERROR);

#undef LOG_WRAPPER

}
