#include "kasten.hpp"

using std::filesystem::path, std::filesystem::is_directory, std::filesystem::filesystem_error, std::filesystem::create_directory;

namespace zettel {

Zettelkasten::Zettelkasten(path root) : m_root(root), m_zettels() {}

void Zettelkasten::initialize() {
    path workspace = m_root / ".zettel";
    if (is_directory(m_root)) {
        if (!is_directory(workspace)) {
            try {
                create_directory(workspace);
            } catch (const filesystem_error& error) {
                throw new ZettelkastenException("Unable to create workspace directory (.zettel).");
            }
        }
    } else {
        throw new ZettelkastenException("Root directory does not exist.");
    }
}

}
