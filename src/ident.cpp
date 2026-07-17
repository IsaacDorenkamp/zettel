#include "ident.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

using std::ostream,
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

}
