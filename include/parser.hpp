#pragma once

#include <iostream>
#include <memory>

#include "content.hpp"

namespace zettel {

class Parser {
public:
    Parser(std::istream& stream);
    virtual ~Parser() = default;

    virtual std::unique_ptr<ContentBlock> next() = 0;
    bool done() const;
protected:
    std::istream* m_stream;
    bool m_done;
};

class DefaultParser : public Parser {
public:
    DefaultParser(std::istream& stream);
    virtual ~DefaultParser() = default;

    virtual std::unique_ptr<ContentBlock> next();

private:
    uint32_t m_blockId;
    uint16_t m_bufferSize;
    uint16_t m_bufferIndex;
    char m_buffer[1024];

    std::unique_ptr<Id> nextId();
    uint16_t readBlock();
};

}
