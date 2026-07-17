#include "kasten.hpp"

using std::filesystem::path, std::filesystem::is_directory, std::filesystem::filesystem_error,
      std::filesystem::create_directory, std::filesystem::directory_iterator, std::filesystem::directory_entry;
using std::string, std::unique_ptr;

namespace zettel {

Zettelkasten::Zettelkasten(path root) : m_loaded(false), m_root(root), m_zettels() {}

void Zettelkasten::initialize() {
    if (m_loaded) {
        throw new ZettelkastenException("Zettelkasten already loaded!");
    }
    path workspace = m_root / ".zettel";
    if (is_directory(m_root)) {
        if (!is_directory(workspace)) {
            try {
                create_directory(workspace);
            } catch (const filesystem_error& error) {
                throw new ZettelkastenException("Unable to create workspace directory (.zettel).");
            }
        }
        m_loaded = true;
    } else {
        throw new ZettelkastenException("Root directory does not exist.");
    }
}

void Zettelkasten::load() {
    if (m_loaded) {
        throw new ZettelkastenException("Zettelkasten already loaded!");
    }
    path workspace = m_root / ".zettel";
    for (const directory_entry& entry : directory_iterator(workspace)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            try {
                Zettel zettel = Zettel::load(entry.path());
                m_zettels.insert({ zettel.id().hash(), zettel });
            } catch (const ZettelException& exception) {
                // TODO: Warn
            }
        }
    }
}

Zettel* Zettelkasten::createZettel(const string& title) {
    // TODO: Implement
    return nullptr;
}

Zettel* Zettelkasten::getZettelById(const Id& id) const {
    // TODO: Implement
    return nullptr;
}

}
