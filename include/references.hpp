#pragma once

#include <string>
#include <vector>

#include "format.hpp"
#include "ident.hpp"

namespace zettel {

class Reference {
public:
    Reference(const Id& id);
    virtual ~Reference() = default;

    const Id& id() const;

    virtual std::vector<std::string> format(const FormatOptions& options) const = 0;
    virtual std::unique_ptr<Reference> clone() const = 0;
protected:
    std::unique_ptr<Id> m_id;
};

class GenericReference : public Reference {
public:
    GenericReference(const Id& id, std::string type, std::string locator);

    const std::string& type() const;
    const std::string& locator() const;

    void setType(std::string type);
    void setLocator(std::string locator);

    virtual std::vector<std::string> format(const FormatOptions& options) const;
    virtual std::unique_ptr<Reference> clone() const;
private:
    std::string m_type;
    std::string m_locator;
};

class ZettelReference : public Reference {
public:
    ZettelReference(const Id& id, const Id& refId);

    const Id& refId() const;
    void setRefId(const Id& refId);

    virtual std::vector<std::string> format(const FormatOptions& options) const;
    virtual std::unique_ptr<Reference> clone() const;
protected:
    std::unique_ptr<Id> m_refId;
};

}
