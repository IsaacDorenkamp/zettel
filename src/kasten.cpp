#include "kasten.hpp"

#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "content.hpp"

using std::filesystem::path, std::filesystem::is_directory, std::filesystem::filesystem_error,
      std::filesystem::create_directory, std::filesystem::directory_iterator, std::filesystem::directory_entry;
using std::string, std::unique_ptr;
using std::map, std::optional, std::pair;

namespace zettel {

Zettelkasten::Zettelkasten(path root) : m_loaded(false), m_root(root), m_zettels(), m_inbox() {}

void Zettelkasten::initialize() {
    if (m_loaded) {
        throw ZettelkastenException("Zettelkasten already loaded!");
    }
    path workspace = m_root / ".zettel";
    path inbox = m_root / "inbox";
    if (is_directory(m_root)) {
        if (!is_directory(workspace)) {
            try {
                create_directory(workspace);
            } catch (const filesystem_error& exc) {
                throw ZettelkastenException(fmt("Unable to create workspace directory (.zettel): %s", exc.what()));
            }
        } else {
            throw ZettelkastenException("Workspace directory (.zettel) already exists!");
        }

        if (!exists(inbox)) {
            try {
                create_directory(inbox);
            } catch (const filesystem_error& exc) {
                throw ZettelkastenException(fmt("Unable to create inbox directory (inbox): %s", exc.what()));
            }
        } else if (!is_directory(inbox)) {
            throw ZettelkastenException("Cannot initialize: 'inbox' already exists, but not as a directory");
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
                m_zettels.insert({ zettel.id().hash(), zettel });
            } catch (const ZettelException& exception) {
                std::cerr << exception.what() << std::endl;
                // TODO: Warn
            }
        }
    }
    m_loaded = true;
}

void Zettelkasten::check() const {
    path workspace = m_root / ".zettel";
    if (!is_directory(workspace)) {
        throw ZettelkastenException("workspace directory (.zettel) not found.");
    }

    path inbox = m_root / "inbox";
    if (!is_directory(inbox)) {
        throw ZettelkastenException("inbox directory not found");
    }
}

const path& Zettelkasten::root() const {
    return m_root;
}

Zettel* Zettelkasten::draftZettel(const string& title) {
    unique_ptr<Id> draftId = generateDraftId();
    path draftPath(m_root / "inbox" / fmt("draft_%s.txt", draftId->represent().c_str()));
    pair<map<Id::Hash, Zettel>::iterator, bool> result = m_inbox.insert({ draftId->hash(), Zettel(*draftId, title, draftPath) });
    if (!result.second) {
        throw ZettelkastenException(fmt("Unable to create Zettel draft (ID '%s' already exists)", draftId->represent().c_str()));
    }
    Zettel* zettel = &result.first->second;
    try {
        zettel->save();
    } catch (const ZettelException& exc) {
        m_inbox.erase(result.first);
        throw ZettelkastenException(fmt("Could not save draft: %s", exc.what()));
    }
    return zettel;
}

Zettel* Zettelkasten::createZettel(const string& title, const Id& id) {
    path zettelPath(m_root / fmt("%s.txt", id.represent().c_str()));
    pair<map<Id::Hash, Zettel>::iterator, bool> result = m_zettels.insert({ id.hash(), Zettel(id, title, zettelPath) });
    if (!result.second) {
        throw ZettelkastenException(fmt("Unable to create new Zettel (ID '%s' already exists)", id.represent().c_str()));
    }
    Zettel* zettel = &result.first->second;
    try {
        zettel->save();
    } catch (const ZettelException& exc) {
        m_zettels.erase(result.first);  // if we fail to save the Zettel, remove it from the internal store
        throw ZettelkastenException(fmt("Could not save Zettel: %s", exc.what()));
    }
    return zettel;
}

Zettel* Zettelkasten::getZettelById(const Id& id) {
    map<Id::Hash, Zettel>::iterator entry = m_zettels.find(id.hash());
    if (entry != m_zettels.end()) {
        return &entry->second;
    } else {
        return nullptr;
    }
}

const Zettel* Zettelkasten::getZettelById(const Id& id) const {
    map<Id::Hash, Zettel>::const_iterator entry = m_zettels.find(id.hash());
    if (entry != m_zettels.end()) {
        return &entry->second;
    } else {
        return nullptr;
    }
}

path Zettelkasten::edit() const {
    path noteFile = m_root / ".zettel" / "NOTE";

    std::ofstream note;
    try {
        note.open(noteFile.c_str(), std::ofstream::trunc);
        note.close();
    } catch (const std::ofstream::failure& exc) {
        throw ZettelkastenException("Unable to truncate note buffer.");
    }

    // TODO: Manually spawn in the future. The trouble with my first attempt was that the terminal
    // did not completely reset after vim exited, causing an undesirable artifact.
    int status = system(fmt("/usr/bin/vi -n \"+set noeol\" %s", noteFile.c_str()).c_str());
    return noteFile;
}

void Zettelkasten::editZettel(const Id& id) {
    string content = edit();
    Zettel* zettel = getZettelById(id);
    
    // TODO: Don't use NumericalId(0) as default
    zettel->clearContent();
    zettel->addContentBlock(unique_ptr<ContentBlock>(new TextBlock(NumericId(0), content)));
    path location = m_root / fmt("%s.txt", id.represent().c_str());
    try {
        zettel->save();
    } catch (const ZettelException& exc) {
        throw ZettelkastenException("Unable to save Zettel.");
    }
}

unique_ptr<Id> Zettelkasten::parseId(string id) const {
    // TODO: Don't hardcode this to numeric!
    return Id::parse(id, Id::Type::Numeric);
}

optional<NumericId> Zettelkasten::getDraftId(string filename) {
    if (filename.substr(0, 6).compare("draft_") == 0) {
        string remainder = filename.substr(6);
        if (remainder.length() == 0) return optional<NumericId>();
        uint32_t id = 0;
        for (size_t index = 0; index < remainder.length(); index++) {
            // NOTE: This logic may be a bit shaky, depending on locale.
            if (!std::isdigit(remainder[index])) {
                if (index == 0) return std::nullopt;
                else break;
            } else {
                id *= 10;
                id += remainder[index] - 48;
            }
        }
        return optional<NumericId>(id);
    } else {
        return std::nullopt;
    }
}

unique_ptr<Id> Zettelkasten::generateDraftId() const {
    uint32_t maxId = 0;
    for (const directory_entry& entry : directory_iterator(m_root / "inbox")) {
        optional<NumericId> maybeId = getDraftId(entry.path().filename());
        if (maybeId) {
            uint32_t id = maybeId->id();
            if (id > maxId) maxId = id;
        }
    }
    return unique_ptr<Id>(new NumericId(++maxId));
}

}
