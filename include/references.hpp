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

class ZettelReference : public Reference {
public:
    ZettelReference(const Id& id, const Id& refId);

    virtual std::vector<std::string> format(const FormatOptions& options) const;
    virtual std::unique_ptr<Reference> clone() const;
protected:
    std::unique_ptr<Id> m_refId;
};

}
