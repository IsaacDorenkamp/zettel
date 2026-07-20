#include "zettel.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ident.hpp"

using std::ifstream, std::ofstream, std::string, std::stringstream, std::unique_ptr, std::vector;

namespace zettel {

Zettel::Zettel(const Id& id, const std::string& title) : m_id(id.clone()), m_title(title), m_tags(), m_content(), m_references() {}
Zettel::Zettel(const Zettel& zettel) : Zettel(zettel.id(), zettel.title()) {
    m_tags = zettel.m_tags;
    for (const unique_ptr<ContentBlock>& block : zettel.m_content) {
        m_content.push_back(block->clone());
    }
    for (const unique_ptr<Reference>& ref : zettel.m_references) {
        m_references.push_back(ref->clone());
    }
}

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

ContentBlock* Zettel::addContentBlock(unique_ptr<ContentBlock>&& block) {
    m_content.push_back(std::move(block));
    return m_content.back().get();
}

Reference* Zettel::addReference(unique_ptr<Reference>&& reference) {
    m_references.push_back(std::move(reference));
    return m_references.back().get();
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

ContentBlock* Zettel::getContentBlock(const Id& id) {
    for (unique_ptr<ContentBlock>& ptr : m_content) {
        if (ptr->id() == id) {
            return ptr.get();
        }
    }
    return nullptr;
}

const ContentBlock* Zettel::getContentBlock(const Id& id) const {
    for (const unique_ptr<ContentBlock>& ptr : m_content) {
        if (ptr->id() == id) {
            return ptr.get();
        }
    }
    return nullptr;
}

Reference* Zettel::getReference(const Id& id) {
    for (unique_ptr<Reference>& ptr : m_references) {
        if (ptr->id() == id) {
            return ptr.get();
        }
    }
    return nullptr;
}

const Reference* Zettel::getReference(const Id& id) const {
    for (const unique_ptr<Reference>& ptr : m_references) {
        if (ptr->id() == id) {
            return ptr.get();
        }
    }
    return nullptr;
}

void Zettel::clearContent() {
    m_content.clear();
}

void Zettel::clearReferences() {
    m_references.clear();
}

void Zettel::save(std::filesystem::path to) {
    ofstream out;
    try {
        out.open(to, ofstream::out | ofstream::trunc);
        out << "[" << m_id->toString() << "] " << m_title << std::endl;
        bool first = true;
        for (const string& tag : m_tags) {
            if (first) first = false;
            else out << " ";
            out << "#" << tag;
        }
        out << std::endl;

        // TODO: Put this constant somewhere else!
        const uint16_t LINE_WIDTH = 75;
        FormatOptions options{
            .mode = DisplayMode::ASCII,
            .line_size = LINE_WIDTH - 1,
            .first_line_offset = 0
        };
        stringstream body;
        for (const unique_ptr<ContentBlock>& block : m_content) {
            vector<string> lines = block->format(options);
            if (lines.size() == 0) continue;
            size_t back = lines.back().size();
            first = true;
            for (const string& line : lines) {
                if (first) first = false;
                else body << std::endl;
                body << "|" << line;
            }
            if (back == LINE_WIDTH) {
                body << std::endl;
                options.first_line_offset = 0;
            } else {
                options.first_line_offset = back;
            }
        }

        options.first_line_offset = 0;
        for (const unique_ptr<Reference>& reference : m_references) {
            vector<string> lines = reference->format(options);
            first = true;
            for (const string& line : lines) {
                if (!first) first = true;
                else body << std::endl;
                body << line;
            }
            body << std::endl;
        }

        out << body.str();

        out.close();
    } catch (const ofstream::failure& exc) {
        throw ZettelException("Failed to write Zettel.");
    }
}

Zettel Zettel::load(std::filesystem::path path) {
    ifstream in;
    try {
        in.open(path);
        std::string id_token;
        if (in >> id_token) {
            if (id_token.size() < 2) {
                throw ZettelException(fmt("Malformed ID: %s", id_token.c_str()));
            }
            if (id_token.front() != '[' || id_token.back() != ']') {
                throw ZettelException("Malformed ID (should be wrapped in brackets).");
            }
            id_token = id_token.substr(1, id_token.size() - 2);
        } else {
            throw ZettelException("Unable to read Zettel ID.");
        }
        std::unique_ptr<Id> id;
        try {
            id = IdParser<NumericalId>::parse(id_token);
        } catch (const IdException& exc) {
            throw ZettelException(fmt("Unable to parse Zettel ID '%s': %s", id_token.c_str(), exc.what()));
        }
        std::string title;
        in.ignore(1, ' ');
        std::getline(in, title);
        Zettel result(*id, title);
        // parse tags
        string tagline;
        getline(in, tagline);
        // no tagline? return the current note.
        if (in.fail() && in.eof()) {
            return result;
        }
        if (!tagline.empty()) {
            std::istringstream tagsource(tagline);
            for (string tag;; getline(tagsource, tag, ' ')) {
                if (tag.empty()) continue;
                if (tag.front() != '#') throw ZettelException(fmt("Invalid tag '%s': must start with #", tag.c_str()));
                tag = tag.substr(1);
                result.addTag(tag);
            }
        }
        // read content section
        stringstream content;
        string line;
        while (in.peek() == '|') {
            getline(in, line);
            line = line.substr(1);  // strip '|'
            content << line;
        }

        result.addContentBlock(unique_ptr<ContentBlock>(new TextBlock(NumericalId(0), content.str())));

        // TODO: all remaining lines are references
        return result;
    } catch (const ifstream::failure& exc) {
        throw ZettelException(fmt("Unable to load Zettel from %s", path.c_str()));
    }
}

}
