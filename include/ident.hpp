#pragma once

#include <cstdint>
#include <exception>
#include <memory>
#include <string>

#include "buffer.hpp"

namespace zettel {

class IdException : public std::exception {
public:
    IdException(std::string message) : m_message(message) {}
    virtual const char* what() const throw() {
        return m_message.c_str();
    }
private:
    std::string m_message;
};

class Id {
public:
    enum class Type {
        Numeric = 0, Classic = 1
    };
    using Representation = std::variant<uint32_t, std::string>;
    Id(Representation repr);
    virtual ~Id() = default;

    uint32_t hash() const;
    bool operator==(const Id& other) const;
private:
    Representation m_value;
};

}
