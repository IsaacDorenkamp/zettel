#include "ident.hpp"

#include <limits>
#include <string>
#include <sstream>

#include "format.hpp"

using std::invalid_argument, std::out_of_range, std::string, std::stringstream, std::unique_ptr;

namespace zettel {

Id::Id(Id::Type type) : m_type(type), m_repr() {}
Id::Type Id::type() const {
    return m_type;
}
string Id::represent() const {
    return m_repr;
}
unique_ptr<Id> Id::parse(string id, Type type) {
    switch (type) {
    case Type::Numeric:
        unsigned long parsed;
        try {
            parsed = std::stoul(id);
            if (parsed > std::numeric_limits<uint32_t>::max()) {
                throw out_of_range(fmt("%lu exceeds uint32 limit", parsed));
            }
        } catch (const out_of_range& exc) {
            throw Id::Exception(exc.what());
        } catch (const invalid_argument& exc) {
            throw Id::Exception(exc.what());
        }
        return unique_ptr<Id>(new NumericId((uint32_t)parsed));
    }
    return nullptr;
}

NumericId::NumericId(uint32_t id) : Id(Type::Numeric), m_id(id) {
    m_repr = fmt("%u", m_id);
}

uint32_t NumericId::id() const { return m_id; }
unique_ptr<Id> NumericId::clone() const {
    return unique_ptr<Id>(new NumericId(m_id));
}
size_t NumericId::hash() const {
    return (size_t)m_id;
}
bool NumericId::operator<(const Id& other) const {
    if (const NumericId* matchingOther = dynamic_cast<const NumericId*>(&other)) {
        return m_id < matchingOther->m_id;
    } else {
        return m_type < other.type();
    }
}
bool NumericId::operator==(const Id& other) const {
    if (const NumericId* matchingOther = dynamic_cast<const NumericId*>(&other)) {
        return m_id == matchingOther->m_id;
    } else {
        return false;
    }
}

}
