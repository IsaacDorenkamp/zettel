#include "parser.hpp"

#include <sstream>

using std::function, std::istream, std::string, std::stringstream, std::unique_ptr;

namespace zettel {

Parser::Parser(istream& stream) : m_stream(&stream), m_done(false), m_bufferSize(0), m_bufferIndex(0), m_buffer() {}
bool Parser::done() const { return m_done; }

void Parser::iterateUntil(function<bool(char)> processor) {
    if (m_bufferIndex >= m_bufferSize) loadNextBlock();
    if (m_done) return;

    bool seeking = true;
    char c;
    while (seeking) {
        for (c = m_buffer[m_bufferIndex]; m_bufferIndex < m_bufferSize; c = m_buffer[++m_bufferIndex]) {
            seeking = processor(c);
            if (!seeking) {
                m_bufferIndex++;
                break;
            }
        }
        if (m_bufferIndex == m_bufferSize) loadNextBlock();
        seeking = seeking && !m_done;
    }
}

void Parser::loadNextBlock() {
    m_stream->read(m_buffer, PARSER_BUFFER_SIZE);
    m_bufferSize = m_stream->gcount();
    m_bufferIndex = 0;
    m_done = m_bufferSize == 0;
}

DefaultParser::DefaultParser(istream& stream, function<unique_ptr<Id>(string)> idParser) : Parser(stream), m_blockId(0), m_idParser(idParser) {}

unique_ptr<ContentBlock> DefaultParser::next() {
    unique_ptr<ContentBlock> content;

    if (m_bufferIndex == m_bufferSize) loadNextBlock();
    if (m_done) return nullptr;

    char first = m_buffer[m_bufferIndex];
    bool escape = false;
    char current;
    stringstream accum;
    if (first == '[') {
        // go until we find matching end brace
        uint8_t layer = 0;
        iterateUntil([&accum, &escape, &layer](char c) {
            if (c == '[' && !escape) layer++;
            else if (c == ']' && !escape) {
                layer--;
                accum << c;
                if (layer == 0) {
                    return false;
                }
            } else if (escape) {
                // escape is not "consumed"
                accum << '\\';
            }
            escape = c == '\\';
            if (!escape) accum << c;
            return true;
        });
        if (escape) accum << '\\';

        if (layer > 0) {
            // we have unclosed stuff, so we'll treat what we have as a text block
            return unique_ptr<ContentBlock>(new TextBlock(*nextId(), accum.str()));
        }

        string displayText = accum.str();
        accum = stringstream();
        layer = 0;
        escape = false;
        iterateUntil([&accum, &escape, &layer](char c) {
            if (c == '(' && !escape) layer++;
            else if (c == ')' && !escape) {
                layer--;
                accum << c;
                if (layer == 0) {
                    return false;
                }
            } else if (escape) {
                // escape is not "consumed"
                accum << '\\';
            }
            escape = c == '\\';
            if (!escape) accum << c;
            return true;
        });
        if (escape) accum << '\\';

        if (layer > 0) {
            return unique_ptr<ContentBlock>(new TextBlock(*nextId(), displayText + accum.str()));
        }

        string fullReference = accum.str();
        string reference = fullReference.substr(1, fullReference.size() - 2);

        size_t delimiter = reference.find(':');
        if (delimiter != string::npos) {
            string kind = reference.substr(0, delimiter);
            string locator = reference.substr(delimiter + 1);
            unique_ptr<Reference> ref;
            if (kind.compare("zettel") == 0) {
                unique_ptr<Id> id;
                try {
                    id = m_idParser(locator);
                } catch (const Id::Exception& exc) {
                    throw Parser::Exception(exc.what());
                }
                if (!id) throw Parser::Exception(fmt("Unable to parse Zettel ID '%s'", locator.c_str()));
                ref = unique_ptr<Reference>(new ZettelReference(*nextId(), *id));
            } else {
                ref = unique_ptr<Reference>(new GenericReference(*nextId(), kind, locator));
            }
            content = unique_ptr<ContentBlock>(new ReferenceBlock(*nextId(), *ref));
        } else {
            content = unique_ptr<ContentBlock>(new TextBlock(*nextId(), displayText + fullReference));
        }
    } else {
        // go until we find unescaped opening bracket
        iterateUntil([this, &accum, &escape](char c) {
            if (c == '[' && !escape) {
                m_bufferIndex--;  // back up one character
                return false;
            }
            else if (escape) {
                accum << '\\';
            }
            escape = c == '\\';
            if (!escape) accum << c;
            return true;
        });
        if (escape) accum << '\\';  // if the last character is a backslash
        content = unique_ptr<ContentBlock>(new TextBlock(*nextId(), accum.str()));
    }

    return content;
}

unique_ptr<Id> DefaultParser::nextId() {
    return unique_ptr<Id>(new NumericId(m_blockId++));
}

}
