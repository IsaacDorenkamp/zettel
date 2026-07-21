#include "content.hpp"

#include <sstream>

using std::string, std::stringstream, std::unique_ptr, std::vector;

namespace zettel {

ContentBlock::ContentBlock(const Id& id) : m_id(id.clone()) {}
ContentBlock::ContentBlock(const ContentBlock& block) : ContentBlock(*block.m_id) {}
const Id& ContentBlock::id() const {
    return *m_id;
}

TextBlock::TextBlock(const Id& id, string text) : ContentBlock(id), m_text(text) {}
unique_ptr<ContentBlock> TextBlock::clone() const {
    return unique_ptr<ContentBlock>(static_cast<ContentBlock*>(new TextBlock(*m_id, m_text)));
}
vector<string> TextBlock::format(FormatOptions options) const {
    vector<string> result;
    stringstream line;
    uint16_t lineSize = options.line_size - options.first_line_offset;
    uint16_t curLineSize = 0;
    char current;
    for (size_t index = 0; index < m_text.size(); index++) {
        current = m_text[index];
        if (current == '\n') {
            result.push_back(line.str());
            line.str("");
            curLineSize = 0;
        } else {
            line << current;
            curLineSize++;
            if (curLineSize == lineSize) {
                result.push_back(line.str());
                line.str("");
                curLineSize = 0;
            }
        }
    }
    result.push_back(line.str());
    return result;
}

}
