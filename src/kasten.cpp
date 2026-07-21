#include "kasten.hpp"

#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "content.hpp"
#include "io.hpp"

using std::filesystem::path, std::filesystem::is_directory, std::filesystem::filesystem_error,
      std::filesystem::create_directory, std::filesystem::directory_iterator, std::filesystem::directory_entry;
using std::string, std::unique_ptr;
using std::map, std::pair;

namespace zettel {

Zettelkasten::Zettelkasten(path root) : m_loaded(false), m_root(root), m_zettels(), m_idGenerator(static_cast<IdGenerator*>(new NumericalIdGenerator())) {}

void Zettelkasten::initialize() {
    if (m_loaded) {
        throw ZettelkastenException("Zettelkasten already loaded!");
    }
    path workspace = m_root / ".zettel";
    if (is_directory(m_root)) {
        if (!is_directory(workspace)) {
            try {
                create_directory(workspace);
            } catch (const filesystem_error& error) {
                throw ZettelkastenException("Unable to create workspace directory (.zettel).");
            }
        } else {
            throw ZettelkastenException("Workspace directory (.zettel) already exists!");
        }
        m_loaded = true;
    } else {
        throw ZettelkastenException("Root directory does not exist.");
    }
}

void Zettelkasten::load() {
    if (m_loaded) {
        throw ZettelkastenException("Zettelkasten already loaded!");
    }
    path workspace = m_root / ".zettel";
    for (const directory_entry& entry : directory_iterator(m_root)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            try {
                Zettel zettel = Zettel::load(entry.path());
                m_idGenerator->consume(zettel.id());
                m_zettels.insert({ zettel.id().hash(), zettel });
            } catch (const ZettelException& exception) {
                std::cerr << exception.what() << std::endl;
                // TODO: Warn
            }
        }
        m_loaded = true;
    }
}

void Zettelkasten::check() const {
    path workspace = m_root / ".zettel";
    if (!is_directory(workspace)) {
        throw ZettelkastenException("Workspace directory (.zettel) not found.");
    }
}

const path& Zettelkasten::root() const {
    return m_root;
}

Zettel* Zettelkasten::createZettel(const string& title) {
    unique_ptr<Id> nextId = m_idGenerator->next();
    pair<map<uint32_t, Zettel>::iterator, bool> result = m_zettels.insert({ nextId->hash(), Zettel(*nextId, title) });
    if (!result.second) {
        throw ZettelkastenException("Unable to create new Zettel (did the ID generator produce a duplicate?)");
    }
    Zettel* zettel = &result.first->second;
    try {
        zettel->save(m_root / fmt("%s.txt", nextId->toString().c_str()));
    } catch (const ZettelException& exc) {
        m_zettels.erase(result.first);  // if we fail to save the Zettel, remove it from the internal store
        throw ZettelkastenException(fmt("Could not save Zettel: %s", exc.what()));
    }
    return zettel;
}

Zettel* Zettelkasten::getZettelById(const Id& id) {
    map<uint32_t, Zettel>::iterator entry = m_zettels.find(id.hash());
    if (entry != m_zettels.end()) {
        return &entry->second;
    } else {
        return nullptr;
    }
}

const Zettel* Zettelkasten::getZettelById(const Id& id) const {
    map<uint32_t, Zettel>::const_iterator entry = m_zettels.find(id.hash());
    if (entry != m_zettels.end()) {
        return &entry->second;
    } else {
        return nullptr;
    }
}

void Zettelkasten::editZettel(const Id& id) {
    using std::cout, std::endl;
    Zettel* zettel = getZettelById(id);
    path content = m_root / ".zettel" / "NOTE";

    std::ofstream note;
    try {
        note.open(content.c_str(), std::ofstream::trunc);
        note.close();
    } catch (const std::ofstream::failure& exc) {
        throw ZettelkastenException("Unable to truncate note buffer.");
    }

    // TODO: Manually spawn in the future. The trouble with my first attempt was that the terminal
    // did not completely reset after vim exited, causing an undesirable artifact.
    int status = system(fmt("/usr/bin/vi -n \"+set noeol\" %s", content.c_str()).c_str());
    path location = m_root / fmt("%s.txt", zettel->id().toString().c_str());
    std::ifstream contentFile;
    std::string noteContent;
    try {
        contentFile.open(content);
        noteContent = stripWhitespace(io::readfile(contentFile));
    } catch (const std::ifstream::failure& exc) {
        throw ZettelkastenException("Unable to read content from note file.");
    }

    // TODO: Don't use NumericalId(0) as default
    zettel->clearContent();
    zettel->addContentBlock(unique_ptr<ContentBlock>(new TextBlock(NumericalId(0), noteContent)));
    try {
        zettel->save(location);
    } catch (const ZettelException& exc) {
        throw ZettelkastenException("Unable to save Zettel.");
    }
}

unique_ptr<Id> Zettelkasten::parseId(string id) const {
    return IdParser<NumericalId>::parse(id);
}

}
