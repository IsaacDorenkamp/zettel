#pragma once

#include <cstdint>
#include <exception>
#include <string>
#include <vector>

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
    Id(Type type);
    virtual ~Id() = default;

    Type type() const;

    virtual std::unique_ptr<Id> clone() const = 0;
    virtual size_t hash() const = 0;

    virtual bool operator<(const Id& other) const = 0;
    virtual bool operator==(const Id& other) const = 0;

    std::string represent() const;

    static std::unique_ptr<Id> parse(std::string id, Type type);
protected:
    Type m_type;
    std::string m_repr;
};

class NumericId : public Id {
public:
    NumericId(uint32_t id);

    uint32_t id() const;

    virtual std::unique_ptr<Id> clone() const;
    virtual size_t hash() const;

    virtual bool operator<(const Id& other) const;
    virtual bool operator==(const Id& other) const;
private:
    uint32_t m_id;
    std::string m_repr;
};

class ClassicId : public Id {
public:
    ClassicId(std::vector<uint16_t> id);

    const std::vector<uint16_t>& id() const;

    virtual std::unique_ptr<Id> clone() const;
    virtual size_t hash() const;

    virtual bool operator<(const Id& other) const;
    virtual bool operator==(const Id& other) const;
private:
    std::vector<uint16_t> m_id;
    std::string m_repr;
};


}
