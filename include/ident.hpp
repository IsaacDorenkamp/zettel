#pragma once

#include "exception.hpp"
#include <cstdint>
#include <memory>
#include <string>

// Developer's note: There was originally an additional Id type,
// ClassicId, which employed a scheme of identity used for physical
// Zettelkastens. However, I found this system unfitting for a
// digital tool, and thus removed it. The Id class remains abstract,
// leaving open the possibility for different identification schemes
// in the future.

namespace zettel {

class Id {
public:
    DEFINE_EXCEPTION;

    using Hash = size_t;
    enum class Type {
        Numeric = 0
    };
    Id(Type type);
    virtual ~Id() = default;

    Type type() const;

    virtual std::unique_ptr<Id> clone() const = 0;
    virtual Hash hash() const = 0;

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
    virtual Hash hash() const;

    virtual bool operator<(const Id& other) const;
    virtual bool operator==(const Id& other) const;
private:
    uint32_t m_id;
};

}
