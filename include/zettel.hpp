#pragma once

#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "content.hpp"

namespace zettel {

class ZettelException : public std::exception {
public:
    ZettelException(std::string message) : m_message(message) {}
    virtual const char* what() const throw() {
        return m_message.c_str();
    }
private:
    std::string m_message;
};

class Zettel {
public:
    using Id = uint32_t;
    Zettel(
        const Id& id,
        const std::string& title,
        const std::filesystem::path& path
    );
    Zettel(const Zettel& zettel);
    virtual ~Zettel() = default;

    const Id& id() const;
    const std::string& title() const;
    const std::vector<std::string>& tags() const;
    const std::vector<std::unique_ptr<ContentBlock>>& content() const;

    void setTitle(std::string title);
    void addTag(std::string tag);
    ContentBlock* addContentBlock(std::unique_ptr<ContentBlock>&& block);

    void removeTag(std::string tag);
    bool removeContentBlock(const Id& id);
    bool removeReference(const Id& id);

    ContentBlock* getContentBlock(const ContentBlock::Id& id);
    const ContentBlock* getContentBlock(const ContentBlock::Id& id) const;

    void clearContent();
    void clearReferences();

    const std::filesystem::path& file() const;

    void save();

    static Zettel load(const std::filesystem::path& file);
private:
    Id m_id;
    std::string m_title;
    std::filesystem::path m_path;
    std::vector<std::string> m_tags;
    std::vector<std::unique_ptr<ContentBlock>> m_content;
};

}
