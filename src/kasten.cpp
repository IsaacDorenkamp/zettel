#include "kasten.hpp"

#include <iostream>

#include "content.hpp"
#include "sys.hpp"

using std::filesystem::path, std::filesystem::is_directory, std::filesystem::filesystem_error,
      std::filesystem::create_directory, std::filesystem::directory_iterator, std::filesystem::directory_entry;
using std::make_unique, std::string, std::unique_ptr;
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
    string command = fmt("%s", content.c_str());
    char* const argv[] = { const_cast<char*>("-c"), const_cast<char*>(command.c_str()), nullptr };
    int status = zettel::sys::spawn("/usr/bin/vi", argv, nullptr);
    path location = m_root / fmt("%s.txt", zettel->id().toString().c_str());
    // TODO: Allow for more complex structure
    zettel->addContentBlock(unique_ptr<ContentBlock>(new TextBlock(NumericalId(0), "")));
    try {
        zettel->save(location);
    } catch (const ZettelException& exc) {
        // TODO: Handle error
    }
}

}
