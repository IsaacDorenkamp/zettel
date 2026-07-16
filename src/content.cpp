#include "content.hpp"

using std::string, std::unique_ptr, std::vector;

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
    uint16_t line_size = options.line_size - options.first_line_offset;
    uint16_t index = 0;
    do {
        std::string portion = m_text.substr(index, line_size);
        result.push_back(portion);
        index += portion.size();
        line_size = options.line_size;
    } while (index < m_text.size());
    return result;
}

}
