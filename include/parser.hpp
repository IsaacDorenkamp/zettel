#pragma once

#include <functional>
#include <iostream>
#include <memory>

#include "content.hpp"

#include "exception.hpp"
#define PARSER_BUFFER_SIZE 1024

namespace zettel {

class Parser {
public:
    DEFINE_EXCEPTION;

    Parser(std::istream& stream);
    virtual ~Parser() = default;

    virtual std::unique_ptr<ContentBlock> next() = 0;
    bool done() const;
protected:
    std::istream* m_stream;
    bool m_done;

    uint16_t m_bufferSize;
    uint16_t m_bufferIndex;
    char m_buffer[PARSER_BUFFER_SIZE];
    void iterateUntil(std::function<bool(char)>);
    void loadNextBlock();
};

class DefaultParser : public Parser {
public:
    DefaultParser(std::istream& stream, std::function<std::unique_ptr<Id>(std::string)> idParser);
    virtual ~DefaultParser() = default;

    virtual std::unique_ptr<ContentBlock> next();
private:
    std::function<std::unique_ptr<Id>(std::string)> m_idParser;
    uint32_t m_blockId;

    std::unique_ptr<Id> nextId();
    uint16_t readBlock();
};

}
