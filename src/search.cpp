#include "search.hpp"

#include <sstream>

using std::optional, std::string, std::stringstream, std::variant;
using zettel::Zettel;

namespace zettel {

SequentialMatcher::SequentialMatcher(string source, string query) : m_source(source), m_query(query), m_index(0), m_state(State::Searching),
    m_startedAt(0), m_possibleMatch(), m_error(), m_match() {}
SequentialMatcher::State SequentialMatcher::update() {
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
bool SequentialMatcher::done() const {
    return m_index >= m_source.size();
}
optional<Matcher::Match> SequentialMatcher::next() {
    State matchState;
    while (m_index < m_source.size()) {
        matchState = update();
        if (matchState == FoundMatch || matchState == EndMatch) return takeMatch();
    }
    optional<Matcher::Match> result = std::nullopt;
    matchState = finalize();
    if (matchState == FoundMatch || matchState == EndMatch)
        result = takeMatch();
    return result;
}
SequentialMatcher::State SequentialMatcher::error(const string& message) {
    m_error = message;
    return State::Error;
}
optional<Matcher::Match> SequentialMatcher::takeMatch() {
    return std::move(m_match);
}
TermMatcher::TermMatcher(string source, string query, bool caseSensitive) : SequentialMatcher(source, query), m_queryIndex(-1), m_caseSensitive(caseSensitive) {}
SequentialMatcher::State TermMatcher::accept(char c) {
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
SequentialMatcher::State TermMatcher::finalize() {
    return State::Searching;
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

optional<Matcher::Match> search(string source, string query, SearchType type) {
    using std::unique_ptr;
    unique_ptr<Matcher> matcher;
    switch (type) {
    case SearchType::TermCS:
        matcher = unique_ptr<Matcher>(new TermMatcher(source, query, true));
    case SearchType::TermCI:
        matcher = unique_ptr<Matcher>(new TermMatcher(source, query, false));
    }
    optional<Matcher::Match> match = matcher->next();
    if (match) return *match;
    return std::nullopt;
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
        optional<Matcher::Match> match = search(content, query.term, SearchType::TermCI);
        if (match) {
            return FullTextSearchResult{
                zettel::models::zettel{z.id(), z.title()},
                match->second
            };
        } else {
            return FullTextSearchResult{std::nullopt, ""};
        }
    } catch (const zettel::ZettelException& exc) {
        return exc.what();
    }
}

}
