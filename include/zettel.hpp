#pragma once

#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "content.hpp"
#include "ident.hpp"
#include "references.hpp"

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
    Zettel(
        const Id& id,
        const std::string& title
    );
    Zettel(const Zettel& zettel);
    virtual ~Zettel() = default;

    const Id& id() const;
    const std::string& title() const;
    const std::vector<std::string>& tags() const;
    const std::vector<std::unique_ptr<ContentBlock>>& content() const;
    const std::vector<std::unique_ptr<Reference>>& references() const;

    void setTitle(std::string title);
    void addTag(std::string tag);
    void addContentBlock(std::unique_ptr<ContentBlock>&& block);
    void addReference(std::unique_ptr<Reference>&& reference);

    void removeTag(std::string tag);
    bool removeContentBlock(const Id& id);
    bool removeReference(const Id& id);

    void save(std::filesystem::path to);

    static Zettel load(std::filesystem::path file);
private:
    std::unique_ptr<Id> m_id;
    std::string m_title;
    std::vector<std::string> m_tags;
    std::vector<std::unique_ptr<ContentBlock>> m_content;
    std::vector<std::unique_ptr<Reference>> m_references;
};

}
