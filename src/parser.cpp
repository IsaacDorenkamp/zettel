#include "parser.hpp"

#include <sstream>

using std::istream, std::string, std::stringstream, std::unique_ptr;

namespace zettel {

Parser::Parser(istream& stream) : m_stream(&stream), m_done(false) {}
bool Parser::done() const { return m_done; }

DefaultParser::DefaultParser(istream& stream) : Parser(stream), m_blockId(0), m_buffer(), m_bufferSize(0), m_bufferIndex(0) {}

unique_ptr<ContentBlock> DefaultParser::next() {
    unique_ptr<ContentBlock> content;

    if (m_bufferIndex == m_bufferSize) m_bufferSize = readBlock();
    if (m_bufferSize == 0) return nullptr;

    char first = m_buffer[m_bufferIndex];
    bool seeking = true;
    bool escape = false;
    char current;
    stringstream accum;
    if (first == '[') {
        // go until we find matching end brace
        uint8_t layer = 0;
        while (seeking) {
            for (current = m_buffer[m_bufferIndex]; m_bufferIndex < m_bufferSize; m_bufferIndex++, current = m_buffer[m_bufferIndex]) {
                if (current == '[' && !escape) layer++;
                else if (current == ']' && !escape) {
                    layer--;
                    if (layer == 0) {
                        seeking = false;
                        break;
                    }
                } else if (escape) {
                    // escape is not "consumed"
                    accum << '\\';
                }
                escape = current == '\\';
                if (!escape) {
                    accum << current;
                }
            }
            if (escape) accum << '\\';
            if (m_bufferIndex == m_bufferSize) m_bufferSize = readBlock();
            seeking = seeking && m_bufferSize > 0;
        }

        if (layer > 0) {
            // we have unclosed stuff, so we'll treat what we have as a text block
            return unique_ptr<ContentBlock>(new TextBlock(*nextId(), accum.str()));
        }

        string displayText = accum.str();
        accum = stringstream();
        seeking = m_bufferSize > 0;
        layer = 0;
        escape = false;
        while (seeking) {
            for (current = m_buffer[m_bufferIndex]; m_bufferIndex < m_bufferSize; m_bufferIndex++, current = m_buffer[m_bufferIndex]) {
                if (current == '(' && !escape) layer++;
                else if (current == ')' && !escape) {
                    layer--;
                    if (layer == 0) {
                        seeking = false;
                        break;
                    }
                } else if (escape) {
                    // escape is not "consumed"
                    accum << '\\';
                }
                escape = current == '\\';
                if (!escape) {
                    accum << current;
                }
            }
            if (escape) accum << '\\';
            if (m_bufferIndex == m_bufferSize) m_bufferSize = readBlock();
            seeking = seeking && m_bufferSize > 0;
        }

        if (layer > 0) {
            return unique_ptr<ContentBlock>(new TextBlock(*nextId(), displayText + accum.str()));
        }

        string reference = accum.str();

        // FIXME: implement ReferenceBlock and construct.
        return nullptr;
    } else {
        // go until we find unescaped opening bracket
        while (seeking) {
            for (current = m_buffer[m_bufferIndex]; m_bufferIndex < m_bufferSize; m_bufferIndex++, current = m_buffer[m_bufferIndex]) {
                if (current == '[' && !escape) {
                    seeking = false;
                    break;
                } else if (escape) {
                    // escape is not "consumed"
                    accum << '\\';
                }
                escape = current == '\\';
                if (!escape) {
                    accum << current;
                }
            }
            if (escape) accum << '\\';
            if (m_bufferIndex == m_bufferSize) m_bufferSize = readBlock();
            seeking = seeking && m_bufferSize > 0;
        }
        content = unique_ptr<ContentBlock>(new TextBlock(*nextId(), accum.str()));
    }

    return content;
}

unique_ptr<Id> DefaultParser::nextId() {
    return unique_ptr<Id>(new NumericId(m_blockId++));
}

uint16_t DefaultParser::readBlock() {
    m_stream->read(m_buffer, 1024);
    m_bufferIndex = 0;
    return m_stream->gcount();
}

}
