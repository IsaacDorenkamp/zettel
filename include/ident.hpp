#pragma once

#include <memory>
#include <string>

namespace zettel {

class Id {
public:
    virtual ~Id() = default;

    virtual std::ostream& write(std::ostream& stream) const = 0;
    virtual std::string toString() const = 0;
    virtual bool operator==(const Id& other) const = 0;
    virtual std::unique_ptr<Id> clone() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Id& id);
};

class NumericalId : public Id {
public:
    NumericalId(uint32_t id);

    uint32_t id() const;

    virtual std::ostream& write(std::ostream& stream) const;
    virtual std::string toString() const;
    virtual bool operator==(const Id& other) const;
    virtual std::unique_ptr<Id> clone() const;
private:
    uint32_t m_id;
    std::string m_repr;

    void construct_repr();
};

}
