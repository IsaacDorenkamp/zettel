#pragma once

#include <cstdint>
#include <exception>
#include <memory>
#include <string>

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
    virtual ~Id() = default;

    virtual std::ostream& write(std::ostream& stream) const = 0;
    virtual std::string toString() const = 0;
    virtual bool operator==(const Id& other) const = 0;
    virtual std::unique_ptr<Id> clone() const = 0;
    virtual uint32_t hash() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Id& id);
};

class NumericalId : public Id {
public:
    NumericalId(uint32_t id);

    uint32_t id() const;
    uint32_t hash() const;

    virtual std::ostream& write(std::ostream& stream) const;
    virtual std::string toString() const;
    virtual bool operator==(const Id& other) const;
    virtual std::unique_ptr<Id> clone() const;
private:
    uint32_t m_id;
    std::string m_repr;

    void construct_repr();
};

class IdGenerator {
public:
    virtual ~IdGenerator() = default;
    virtual std::unique_ptr<Id> next() = 0;
    virtual void consume(const Id& used) = 0;
};

template <class IdType>
class TypedIdGenerator : public IdGenerator {
public:
    TypedIdGenerator() = default;
    virtual ~TypedIdGenerator() = default;

    virtual std::unique_ptr<Id> next() {
        return std::unique_ptr<Id>(static_cast<Id*>(typedNext().release()));
    }
    virtual void consume(const Id& used) {
        const Id* ptr = &used;
        if (const IdType* casted = dynamic_cast<const IdType*>(ptr)) {
            typedConsume(*casted);
        }
        // we can ignore any ID of an unmatched type
    }

    virtual std::unique_ptr<IdType> typedNext() = 0;
    virtual void typedConsume(const IdType& used) = 0;
};

class NumericalIdGenerator : public TypedIdGenerator<NumericalId> {
public:
    NumericalIdGenerator();
    virtual ~NumericalIdGenerator() = default;

    virtual std::unique_ptr<NumericalId> typedNext();
    virtual void typedConsume(const NumericalId& used);
private:
    uint32_t m_next;
};


template <typename T>
class IdParser;

template <>
class IdParser<NumericalId> {
public:
    static NumericalId parse(std::string source);
};

}
