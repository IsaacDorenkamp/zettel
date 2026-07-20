#include "ident.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

using std::make_unique,
      std::ostream,
      std::setfill,
      std::setw,
      std::string,
      std::stringstream,
      std::unique_ptr;

namespace zettel {

ostream& operator<<(ostream& os, const Id& id) {
    return id.write(os);
}

NumericalId::NumericalId(uint32_t id) : m_id(id), m_repr() {
    construct_repr();
}

uint32_t NumericalId::id() const {
    return m_id;
}

uint32_t NumericalId::hash() const {
    return m_id;
}

ostream& NumericalId::write(ostream& stream) const {
    return stream << m_id;
}

string NumericalId::toString() const {
    return m_repr;
}

bool NumericalId::operator==(const Id& other) const {
    const Id* ptr = &other;
    if (const NumericalId* matched = dynamic_cast<const NumericalId*>(ptr)) {
        return matched->m_id == m_id;
    } else {
        return false;
    }
}

unique_ptr<Id> NumericalId::clone() const {
    return unique_ptr<Id>(static_cast<Id*>(new NumericalId(m_id)));
}

void NumericalId::construct_repr() {
    stringstream ss;
    ss << setfill('0') << setw(10) << m_id;
    m_repr = ss.str();
}

NumericalIdGenerator::NumericalIdGenerator() : m_next(0) {}

unique_ptr<NumericalId> NumericalIdGenerator::typedNext() {
    return make_unique<NumericalId>(m_next++);
}

void NumericalIdGenerator::typedConsume(const NumericalId& consumed) {
    uint32_t wouldBeNext = consumed.id() + 1;
    if (wouldBeNext > m_next) m_next = wouldBeNext;
}

unique_ptr<NumericalId> IdParser<NumericalId>::parse(string source) {
    if (source.size() > 10) {
        throw IdException("NumericalId should be no more than 10 characters in length.");
    }
    uint32_t id = 0;
    uint32_t factor = 1;
    for (string::const_reverse_iterator place = source.crbegin(); place != source.crend(); place++) {
        switch (*place) {
        case '0':
            break;
        case '1':
            id += factor;
            break;
        case '2':
            id += 2 * factor;
            break;
        case '3':
            id += 3 * factor;
            break;
        case '4':
            id += 4 * factor;
            break;
        case '5':
            id += 5 * factor;
            break;
        case '6':
            id += 6 * factor;
            break;
        case '7':
            id += 7 * factor;
            break;
        case '8':
            id += 8 * factor;
            break;
        case '9':
            id += 9 * factor;
            break;
        default:
            throw IdException("All characters in NumericalID must be decimal digits.");
        }
        factor *= 10;
    }
    return make_unique<NumericalId>(id);
}

}
