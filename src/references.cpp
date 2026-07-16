#include "references.hpp"

#include <string>

using std::string, std::unique_ptr, std::vector;

namespace zettel {

Reference::Reference(const Id& id) : m_id(id.clone()) {}
const Id& Reference::id() const {
    return *m_id;
}

ZettelReference::ZettelReference(const Id& id, const Id& refId) : Reference(id), m_refId(refId.clone()) {}
vector<string> ZettelReference::format(const FormatOptions& options) const {
    vector<string> result;
    uint16_t line_size = options.line_size - options.first_line_offset;
    uint16_t index = 0;
    std::string source = m_refId->toString();
    do {
        std::string portion = source.substr(index, line_size);
        result.push_back(portion);
        index += portion.size();
        line_size = options.line_size;
    } while (index < source.size());
    return result;
}

unique_ptr<Reference> ZettelReference::clone() const {
    return unique_ptr<Reference>(static_cast<Reference*>(new ZettelReference(*m_id, *m_refId)));
}

}
