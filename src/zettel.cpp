#include "zettel.hpp"

using std::unique_ptr;

namespace zettel {

Zettel::Zettel(const ZettelId& id, const std::string& title) : m_id(id.clone()), m_title(title) {}

}
