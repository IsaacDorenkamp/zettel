#include "logging.hpp"

#include <map>

#include "ansi.hpp"

using std::map, std::pair, std::shared_ptr, std::string, std::vector;

log::StreamLogHandler::StreamLogHandler(std::ostream* stream, bool ansi) : m_stream(stream), m_ansi(ansi) {}
void log::StreamLogHandler::handle(const log::LogRecord& record) {
    if (m_ansi) {
        ansi::ANSIBlock kind;
        kind.bold();
        switch (record.level) {
        case DEBUG:
            kind.setText("DEBUG");
            kind.foreground(ansi::Color::MAGENTA);
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
        (*m_stream) << kind << " " << record.message << std::endl;
    } else {
        (*m_stream) << record.message << std::endl;
    }
}

log::CaptureLogHandler::CaptureLogHandler() : m_records() {}

void log::CaptureLogHandler::handle(const LogRecord& record) {
    m_records.push_back(record);
}

const vector<log::LogRecord>& log::CaptureLogHandler::records() const {
    return m_records;
}

void log::CaptureLogHandler::clear() {
    m_records.clear();
}

static map<string, shared_ptr<log::LogHandler>> handlers;

void log::log(const LogRecord& record) {
    for (pair<const string, shared_ptr<log::LogHandler>>& entry : handlers) entry.second->handle(record);
}

void log::putHandler(string name, const shared_ptr<log::LogHandler>& handler) {
    if (!handlers.try_emplace(name, handler).second) throw std::runtime_error(zettel::fmt("A handler named '%s' already exists!", name.c_str()));
}

shared_ptr<log::LogHandler> log::popHandler(string name) {
    map<string, shared_ptr<log::LogHandler>>::iterator entry = handlers.find(name);
    if (entry != handlers.end()) {
        shared_ptr<log::LogHandler> result(entry->second);
        handlers.erase(entry);
        return result;
    }
    return nullptr;
}
