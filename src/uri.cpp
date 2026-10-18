#include "uri.hpp"

#include <sstream>

using std::optional, std::string, std::stringstream;

namespace zettel {

URI::URI(const string& scheme, const optional<string>& authority, const string& path, const optional<string>& query, const optional<string>& fragment)
    : m_scheme(scheme), m_authority(authority), m_path(path), m_query(query), m_fragment(fragment), m_repr() {
    updateRepr();
}
URI::URI() : URI("", std::nullopt, "", std::nullopt, std::nullopt) {}

const string& URI::scheme() const { return m_scheme; }
const optional<string>& URI::authority() const { return m_authority; }
const string& URI::path() const { return m_path; }
const optional<string>& URI::query() const { return m_query; }
const optional<string>& URI::fragment() const { return m_fragment; }

void URI::setScheme(const string& scheme) {
    m_scheme = scheme;
    updateRepr();
}
void URI::setAuthority(const optional<string>& authority) {
    m_authority = authority;
    updateRepr();
}
void URI::setPath(const string& path) {
    m_path = path;
    updateRepr();
}
void URI::setQuery(const optional<string>& query) {
    m_query = query;
    updateRepr();
}
void URI::setFragment(const optional<string>& fragment) {
    m_fragment = fragment;
    updateRepr();
}

const string& URI::represent() const {
    return m_repr;
}

URI URI::parse(string uri) {
    string::const_iterator it = uri.begin();
    string::const_iterator end = uri.end();
    stringstream ss;
    char ch;
    do {
        if (it == end) throw Exception("URI is empty!");
        if (*it == ':') {
            it++;
            break;
        }
        ss << *it++;
    } while (true);

    string scheme = ss.str();
    ss.str("");

    optional<string> authority = std::nullopt;
    if (it == end) return URI(scheme, std::nullopt, "", std::nullopt, std::nullopt);
    if (*it == '/') {
        it++;
        if (it == end) return URI(scheme, std::nullopt, "/", std::nullopt, std::nullopt);
        if (*it == '/') {
            it++;
            // get authority
            do {
                if (it == end) return URI(scheme, ss.str(), "", std::nullopt, std::nullopt);
                if (*it == '/') {
                    // third slash means start path
                    it++;
                    authority = ss.str();
                    ss.str("");
                    ss << "/";
                    break;
                }
                ss << *it++;
            } while (true);
        } else ss << '/';
    }

    string path;
    bool skipQuery = true;
    do {
        if (it == end) return URI(scheme, authority, ss.str(), std::nullopt, std::nullopt);
        if (*it == '?' || *it == '#') {
            skipQuery = *it++ == '#';
            path = ss.str();
            ss.str("");
            break;
        }
        ss << *it++;
    } while (true);

    string query;
    while (!skipQuery) {
        if (it == end) return URI(scheme, authority, path, ss.str(), std::nullopt);
        if (*it == '#') {
            it++;
            query = ss.str();
            ss.str("");
            break;
        }
        ss << *it++;
    }

    while (it != end) ss << *it++;
    return URI(scheme, authority, path, query, ss.str());
}

void URI::updateRepr() {
    stringstream ss;
    ss << m_scheme << ':';
    if (m_authority) ss << "//" << *m_authority;
    ss << m_path;
    if (m_query) ss << '?' << *m_query;
    if (m_fragment) ss << '#' << *m_fragment;
    m_repr = ss.str();
}

}
