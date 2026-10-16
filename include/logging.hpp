#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "format.hpp"

namespace logs {

enum Level {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

struct LogRecord {
    Level level;
    std::string message;
};

class LogHandler {
public:
    virtual ~LogHandler() = default;
    virtual void handle(const LogRecord& record) = 0;
};

class StreamLogHandler : public LogHandler {
public:
    StreamLogHandler(std::ostream* stream, bool ansi = false);
    virtual void handle(const LogRecord& record);
private:
    std::ostream* m_stream;
    bool m_ansi;
};

class CaptureLogHandler : public LogHandler {
public:
    CaptureLogHandler();
    virtual void handle(const LogRecord& record);

    const std::vector<LogRecord>& records() const;
    void clear();
private:
    std::vector<LogRecord> m_records;
};

void log(const LogRecord& record);

template <typename... Args>
void logf(Level level, std::string format, Args&&... args) {
    std::string message = zettel::fmt(format, std::forward<decltype(args)>(args)...);
    log(LogRecord{ level, message });
}

#define LOG_WRAPPER(fn_name, log_level) template <typename... Args>\
void fn_name(std::string message, Args&&... args) {\
    logf(log_level, message, std::forward<decltype(args)>(args)...);\
}\

LOG_WRAPPER(debug, DEBUG);
LOG_WRAPPER(info, INFO);
LOG_WRAPPER(warn, WARNING);
LOG_WRAPPER(error, ERROR);

#undef LOG_WRAPPER

void putHandler(std::string name, const std::shared_ptr<LogHandler>& handler);
std::shared_ptr<LogHandler> popHandler(std::string name);

}
