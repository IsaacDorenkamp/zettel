#include "zettel.hpp"

#include "ident.hpp"
using std::string, std::unique_ptr, std::vector;

namespace zettel {

Zettel::Zettel(const Id& id, const std::string& title) : m_id(id.clone()), m_title(title), m_tags(), m_content(), m_references() {}
Zettel::Zettel(const Zettel& zettel) : Zettel(zettel.id(), zettel.title()) {}

const Id& Zettel::id() const {
    return *m_id;
}
const string& Zettel::title() const {
    return m_title;
}

const vector<string>& Zettel::tags() const {
    return m_tags;
}

const vector<unique_ptr<ContentBlock>>& Zettel::content() const {
    return m_content;
}

const vector<unique_ptr<Reference>>& Zettel::references() const {
    return m_references;
}

void Zettel::setTitle(string title) {
    m_title = title;
}

void Zettel::addTag(string tag) {
    // TODO: check if tag already exists
    m_tags.push_back(tag);
}

void Zettel::addContentBlock(unique_ptr<ContentBlock>&& block) {
    m_content.push_back(std::move(block));
}

void Zettel::addReference(unique_ptr<Reference>&& reference) {
    m_references.push_back(std::move(reference));
}

void Zettel::removeTag(string tag) {
    for (vector<string>::const_iterator it = m_tags.begin(); it != m_tags.end(); it++) {
        if (it->compare(tag) == 0) {
            m_tags.erase(it);
            break;
        }
    }
}

bool Zettel::removeContentBlock(const Id& id) {
    for (vector<unique_ptr<ContentBlock>>::const_iterator it = m_content.begin(); it != m_content.end(); it++) {
        if ((*it)->id() == id) {
            m_content.erase(it);
            return true;
        }
    }
    return false;
}

bool Zettel::removeReference(const Id& id) {
    for (vector<unique_ptr<Reference>>::const_iterator it = m_references.begin(); it != m_references.end(); it++) {
        if ((*it)->id() == id) {
            m_references.erase(it);
            return true;
        }
    }
    return false;
}

Zettel Zettel::load(std::filesystem::path path) {
    // TODO: Implement
    Zettel result(NumericalId(1), "");
    return result;
}

}
