#pragma once

#include <memory>
#include <string>
#include <vector>

#include "format.hpp"
#include "ident.hpp"
#include "references.hpp"

namespace zettel {

class ContentBlock {
public:
    ContentBlock(const Id& id);
    ContentBlock(const ContentBlock& block);
    virtual ~ContentBlock() = default;
    virtual std::unique_ptr<ContentBlock> clone() const = 0;
    virtual std::vector<std::string> format(const FormatOptions& options) const = 0;

    const Id& id() const;
protected:
    std::unique_ptr<Id> m_id;
};

class TextBlock : public ContentBlock {
public:
    TextBlock(const Id& id, std::string text);
    virtual ~TextBlock() = default;

    void setText(const std::string& text);
    const std::string& text() const;

    virtual std::unique_ptr<ContentBlock> clone() const;
    virtual std::vector<std::string> format(const FormatOptions& options) const;
protected:
    std::string m_text;
};

class ReferenceBlock : public ContentBlock {
public:
    ReferenceBlock(const Id& id, const Reference& ref);
    virtual ~ReferenceBlock() = default;

    void setReference(std::unique_ptr<Reference>&& ref);
    const Reference& reference() const;

    virtual std::unique_ptr<ContentBlock> clone() const;
    virtual std::vector<std::string> format(const FormatOptions& options) const;
protected:
    std::unique_ptr<Reference> m_ref;
};

}
