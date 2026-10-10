#pragma once

#include <memory>
#include <optional>
#include <string>

#include "format.hpp"

namespace zettel {

class ContentBlock {
public:
    using Id = uint32_t;

    ContentBlock(const Id& id);
    ContentBlock(const ContentBlock& block);
    virtual ~ContentBlock() = default;
    virtual std::unique_ptr<ContentBlock> clone() const = 0;
    virtual std::string format(const FormatOptions& options) const = 0;

    const Id& id() const;
protected:
    Id m_id;
};

class TextBlock : public ContentBlock {
public:
    TextBlock(const Id& id, std::string text);
    virtual ~TextBlock() = default;

    void setText(const std::string& text);
    const std::string& text() const;

    virtual std::unique_ptr<ContentBlock> clone() const;
    virtual std::string format(const FormatOptions& options) const;
protected:
    std::string m_text;
};

class ReferenceBlock : public ContentBlock {
public:
    ReferenceBlock(const Id& id, const std::string& uri, const std::optional<std::string>& text = std::nullopt);
    virtual ~ReferenceBlock() = default;

    void setURI(const std::string& uri);
    const std::string& uri() const;

    void setText(const std::optional<std::string>& text);
    const std::optional<std::string>& text() const;

    virtual std::unique_ptr<ContentBlock> clone() const;
    virtual std::string format(const FormatOptions& options) const;
protected:
    std::string m_uri;
    std::optional<std::string> m_text;
};

}
