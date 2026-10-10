#include "references.hpp"

#include <string>

using std::string, std::unique_ptr;

namespace zettel {

Reference::Reference(const Id& id) : m_id(id) {}
const Reference::Id& Reference::id() const {
    return m_id;
}

GenericReference::GenericReference(const Id& id, string type, string locator) : Reference(id), m_type(type), m_locator(locator) {}

const string& GenericReference::type() const {
    return m_type;
}
const string& GenericReference::locator() const {
    return m_locator;
}

void GenericReference::setType(string type) {
    m_type = type;
}
void GenericReference::setLocator(string locator) {
    m_locator = locator;
}

string GenericReference::format(const FormatOptions& options) const {
    return wrapString(m_type + ':' + m_locator, options);
}

unique_ptr<Reference> GenericReference::clone() const {
    return unique_ptr<Reference>(new GenericReference(m_id, m_type, m_locator));
}

ZettelReference::ZettelReference(const Id& id, const Id& refId) : Reference(id), m_refId(refId) {}

const Reference::Id& ZettelReference::refId() const {
    return m_refId;
}

void ZettelReference::setRefId(const Id& refId) {
    m_refId = refId;
}

string ZettelReference::format(const FormatOptions& options) const {
    return wrapString(std::to_string(m_refId), options);
}

unique_ptr<Reference> ZettelReference::clone() const {
    return unique_ptr<Reference>(static_cast<Reference*>(new ZettelReference(m_id, m_refId)));
}

}
