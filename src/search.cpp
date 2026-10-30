#include "search.hpp"

#include <sstream>

using std::optional, std::pair, std::string, std::stringstream, std::variant;
using zettel::Zettel;

namespace zettel {

Matcher::Matcher(string source, string query) : m_source(source), m_query(query), m_index(0), m_state(State::Searching),
    m_startedAt(0), m_possibleMatch(), m_error(), m_match() {}
Matcher::State Matcher::next() {
    char c = m_source[m_index];
    State newState = accept(c);
    switch (newState) {
    case Searching:
        break;
    case PossibleMatch:
        m_possibleMatch << c;
        if (m_state == State::Searching) m_startedAt = m_index;
        break;
    case FoundMatch:
        m_possibleMatch << c;
        m_match = {m_startedAt, m_possibleMatch.str()};
        m_possibleMatch.str("");
        break;
    case EndMatch:
        m_match = {m_startedAt, m_possibleMatch.str()};
        m_possibleMatch.str("");
        m_index--; // we need to re-check the current character to see if it could be the start of another possible match
    case BrokeMatch:
        m_possibleMatch.str("");
        m_index = m_startedAt;
    default:
        break;
    }
    m_state = newState == PossibleMatch ? PossibleMatch : Searching;
    m_index++;
    return newState;
}
bool Matcher::done() const {
    return m_index >= m_source.size();
}
Matcher::State Matcher::error(const string& message) {
    m_error = message;
    return State::Error;
}
optional<Matcher::Match> Matcher::takeMatch() {
    return std::move(m_match);
}
TermMatcher::TermMatcher(string source, string query, bool caseSensitive) : Matcher(source, query), m_queryIndex(-1), m_caseSensitive(caseSensitive) {}
Matcher::State TermMatcher::accept(char c) {
    if (m_query.empty()) return error("Search query may not be empty.");
    if (m_queryIndex >= 0) {
        if (matches(c, m_query[m_queryIndex])) {
            m_queryIndex++;
            if (m_queryIndex == m_query.size()) return State::FoundMatch;
            else return State::PossibleMatch;
        } else {
            m_queryIndex = -1;
            return State::BrokeMatch;
        }
    } else {
        if (matches(c, m_query[0])) {
            m_queryIndex = 1;
            if (m_queryIndex == m_query.size()) return State::FoundMatch;
            return State::PossibleMatch;
        } else return State::Searching;
    }
}
bool TermMatcher::matches(char a, char b) const {
    if (m_caseSensitive) return a == b;

    if (a >= 65 && a <= 90) {
        return a == b || a == (b - 32);
    } else if (a >= 97 && a <= 122) {
        return a == b || a == (b + 32);
    } else {
        return a == b;
    }
}

SearchResult search(string source, string query, SearchType type) {
    using std::unique_ptr;
    unique_ptr<Matcher> matcher;
    switch (type) {
    case SearchType::TermCS:
        matcher = unique_ptr<Matcher>(new TermMatcher(source, query, true));
    case SearchType::TermCI:
        matcher = unique_ptr<Matcher>(new TermMatcher(source, query, false));
    }
    while (!matcher->done()) {
        Matcher::State s = matcher->next();
        if (s == Matcher::FoundMatch || s == Matcher::EndMatch) return *matcher->takeMatch();
    }
    return {string::npos, ""};
}

variant<FullTextSearchResult, string> searchFile(const FullTextSearchQuery& query) {
    using std::ifstream;
    using zettel::Zettel;
    try {
        Zettel z = Zettel::load(query.file);
        // first, get plaintext body of zettel
        stringstream contentss;
        for (const std::unique_ptr<zettel::ContentBlock>& block : z.content()) contentss << block->format(zettel::PLAINTEXT_FORMAT);
        string content(contentss.str());
        pair<size_t, string> match = search(content, query.term, SearchType::TermCI);
        if (match.first == string::npos) {
            return FullTextSearchResult{std::nullopt, ""};
        } else {
            return FullTextSearchResult{
                zettel::models::zettel{z.id(), z.title()},
                match.second
            };
        }
    } catch (const zettel::ZettelException& exc) {
        return exc.what();
    }
}

}
