#include "content.hpp"

#include <sstream>

using std::optional, std::string, std::stringstream, std::unique_ptr;

namespace zettel {

ContentBlock::ContentBlock(const Id& id) : m_id(id) {}
ContentBlock::ContentBlock(const ContentBlock& block) : ContentBlock(block.m_id) {}
const ContentBlock::Id& ContentBlock::id() const {
    return m_id;
}

TextBlock::TextBlock(const Id& id, string text) : ContentBlock(id), m_text(text) {}

void TextBlock::setText(const string& text) { m_text = text; }
const string& TextBlock::text() const {
    return m_text;
}

unique_ptr<ContentBlock> TextBlock::clone() const {
    return unique_ptr<ContentBlock>(static_cast<ContentBlock*>(new TextBlock(m_id, m_text)));
}
string TextBlock::format(const FormatOptions& options) const {
    if (options.line_size == 0) return m_text;
    stringstream result;
    uint16_t lineSize = options.line_size - options.first_line_offset;
    uint16_t curLineSize = 0;
    char current;
    for (size_t index = 0; index < m_text.size(); index++) {
        current = m_text[index];
        // TODO: carriage returns?
        if (current == '\n') {
            result << '\n';
            curLineSize = 0;
        } else {
            if (curLineSize == lineSize) {
                result << '\n';
                curLineSize = 0;
                lineSize = options.line_size;
            }
            curLineSize++;
        }
    }
    return result.str();
}

ReferenceBlock::ReferenceBlock(const Id& id, const std::string& uri, const optional<string>& text) : ContentBlock(id), m_uri(uri), m_text(text) {}

void ReferenceBlock::setURI(const string& uri) { m_uri = uri; }
const std::string& ReferenceBlock::uri() const { return m_uri; }

void ReferenceBlock::setText(const optional<string>& text) { m_text = text; }
const optional<string>& ReferenceBlock::text() const { return m_text; }

unique_ptr<ContentBlock> ReferenceBlock::clone() const {
    return unique_ptr<ContentBlock>(new ReferenceBlock(m_id, m_uri));
}
string ReferenceBlock::format(const FormatOptions& options) const {
    if (m_text) {
        return fmt("[%s](%s)", m_text->c_str(), m_uri.c_str());
    } else {
        return fmt("<%s>", m_uri.c_str());
    }
}

}
