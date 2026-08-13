#include "ident.hpp"

#include <limits>
#include <string>
#include <sstream>

#include "format.hpp"

using std::invalid_argument, std::out_of_range, std::string, std::stringstream, std::unique_ptr, std::vector;

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
    case Type::Classic:
        bool isNumericPart = true;
        uint16_t current = 0;
        vector<uint16_t> ids;
        for (char c : id) {
            if (isNumericPart) {
                if (c >= 48 && c <= 57) {
                    current *= 10;
                    current += c - 48;  // easy way to convert char code to single digit value
                } else if (c >= 97 && c <= 122) {
                    if (current == 0) throw Id::Exception(fmt("Expected a digit, got %c instead", c));
                    ids.push_back(current);
                    current = c - 97;  // easy way to convert char code into 0-26
                    isNumericPart = false;
                } else {
                    throw Id::Exception(fmt("Character not permitted in classic ID format: '%c'", c));
                }
            } else {
                if (c >= 48 && c <= 57) {
                    if (current == 0) throw Id::Exception(fmt("Expected a lowercase letter, got %c instead", c));
                    ids.push_back(current);
                    current = c - 48;
                    isNumericPart = true;
                } else if (c >= 97 && c <= 122) {
                    current *= 26;
                    current += c - 97;
                } else {
                    throw Id::Exception(fmt("Character not permitted in classic ID format: '%c'", c));
                }
            }
        }
        if (current == 0) throw Id::Exception("Classic ID cannot be empty or 0!");
        ids.push_back(current);
        return unique_ptr<Id>(new ClassicId(ids));
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

ClassicId::ClassicId(vector<uint16_t> id) : Id(Type::Classic), m_id(id) {
    stringstream result;
    bool isNumericPart = true;
    for (uint16_t item : m_id) {
        if (isNumericPart) {
            result << item;
        } else {
            if (item == 0) result << 'a';
            while (item > 0) {
                char c = (item % 26) + 97;
                result << c;
                item /= 26;
            }
        }
        isNumericPart = !isNumericPart;
    }
    m_repr = result.str();
}

const std::vector<uint16_t>& ClassicId::id() const { return m_id; }

unique_ptr<Id> ClassicId::clone() const {
    return unique_ptr<Id>(new ClassicId(m_id));
}
size_t ClassicId::hash() const {
    return std::hash<string>{}(m_repr);
}
bool ClassicId::operator<(const Id& other) const {
    if (const ClassicId* matchingOther = dynamic_cast<const ClassicId*>(&other)) {
        for (size_t index = 0; index < m_id.size(); index++) {
            if (index >= matchingOther->m_id.size()) {
                return false;
            } else {
                if (m_id[index] != matchingOther->m_id[index]) {
                    return m_id[index] < matchingOther->m_id[index];
                }
            }
        }
        return true;
    } else {
        return m_type < other.type();
    }
}

bool ClassicId::operator==(const Id& other) const {
    if (const ClassicId* matchingOther = dynamic_cast<const ClassicId*>(&other)) {
        if (m_id.size() != matchingOther->m_id.size()) return false;
        for (size_t index = 0; index < m_id.size(); index++) {
            if (m_id[index] != matchingOther->m_id[index]) return false;
        }
        return true;
    } else {
        return false;
    }
}

}
