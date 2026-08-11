#pragma once

#include <exception>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "ident.hpp"
#include "zettel.hpp"

namespace zettel {

class ZettelkastenException : public std::exception {
public:
    ZettelkastenException(std::string message) : m_message(message) {}
    virtual const char* what() const throw() {
        return m_message.c_str();
    }
private:
    std::string m_message;
};

class Zettelkasten {
public:
    Zettelkasten(std::filesystem::path);
    virtual ~Zettelkasten() = default;

    void initialize();
    void load();
    void check() const;

    const std::filesystem::path& root() const;

    Zettel* draftZettel(const std::string& title);
    Zettel* createZettel(const std::string& title, const Id& id);
    Zettel* getZettelById(const Id& id);
    const Zettel* getZettelById(const Id& id) const;

    std::filesystem::path edit() const;
    void editZettel(const Id& id);

    std::unique_ptr<Id> parseId(std::string id) const;

    static std::optional<NumericId> getDraftId(std::string filename);
private:
    bool m_loaded;

    std::filesystem::path m_root;
    std::map<size_t, Zettel> m_zettels;
    std::map<size_t, Zettel> m_inbox;

    std::unique_ptr<Id> generateDraftId() const;
};

}
