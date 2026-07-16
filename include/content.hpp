#pragma once

#include <memory>
#include <string>
#include <vector>

#include "format.hpp"
#include "ident.hpp"

namespace zettel {

class ContentBlock {
public:
    ContentBlock(const Id& id);
    virtual ~ContentBlock() = default;
    virtual std::unique_ptr<ContentBlock> clone() const = 0;
    virtual std::vector<std::string> format(FormatOptions options) const = 0;

    const Id& id() const;
protected:
    std::unique_ptr<Id> m_id;
};

class TextBlock : public ContentBlock {
public:
    TextBlock(const Id& id, std::string text);
    virtual ~TextBlock() = default;
    virtual std::unique_ptr<ContentBlock> clone() const;
    virtual std::vector<std::string> format(FormatOptions options) const;
protected:
    std::string m_text;
};

}
