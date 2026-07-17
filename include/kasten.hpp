#pragma once

#include <exception>
#include <filesystem>
#include <map>
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

    Zettel* createZettel(const std::string& title);
    Zettel* getZettelById(const Id& id) const;
private:
    bool m_loaded;

    std::filesystem::path m_root;
    std::map<uint32_t, Zettel> m_zettels;
};

}
