#include "search.hpp"

#include <boost/regex.hpp>
#include <sstream>

using std::optional, std::string, std::stringstream, std::variant;
using zettel::Zettel;

namespace zettel {

Matcher::Matcher(string source, string query) : m_source(source), m_query(query) {}

SequentialMatcher::SequentialMatcher(string source, string query) : Matcher(source, query), m_index(0), m_state(State::Searching),
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

RegexMatcher::RegexMatcher(string source, string query, bool caseSensitive) : Matcher(source, query), m_start(source.begin()), m_end(source.end()),
    m_expression(query, boost::regex::basic | (caseSensitive ? 0 : boost::regex::icase)), m_flags(boost::match_default), m_match() {}
optional<Matcher::Match> RegexMatcher::next() {
    optional<Matcher::Match> result = std::nullopt;
    if (boost::regex_search(m_start, m_end, m_match, m_expression, m_flags)) {
        result = {
            m_match[0].first - m_source.begin(),
            m_match[0].str()
        };
        m_start = m_match[0].second;
        m_flags |= boost::match_prev_avail | boost::match_not_bob;
    }
    return result;
}

optional<Matcher::Match> search(string source, string query, SearchType type) {
    using std::unique_ptr;
    unique_ptr<Matcher> matcher;
    switch (type.type) {
    case TermType::Substring:
        matcher = unique_ptr<Matcher>(new TermMatcher(source, query, type.caseSensitive));
        break;
    case TermType::Regex:
        matcher = unique_ptr<Matcher>(new RegexMatcher(source, query, type.caseSensitive));
        break;
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
        optional<Matcher::Match> match = search(content, query.term, query.type);
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

const SearchType SearchType::SubstringCI{TermType::Substring, false};
const SearchType SearchType::SubstringCS{TermType::Substring, true};
const SearchType SearchType::RegexCI{TermType::Regex, false};
const SearchType SearchType::RegexCS{TermType::Regex, true};

void FullTextSearchQuery::parseAndSetTerm(const string& str) {
    std::string_view view(str);
    if (view.empty()) {
        term = "";
        return;
    }
    if (view.front() == '/') {
        // string_view::substr is far faster than string::substr
        std::string_view remainder = view.substr(1);
        if (remainder.size() == 0 || remainder.back() != '/') return;
        type.type = TermType::Regex;
        term = remainder.substr(1, remainder.size() - 2);
    } else {
        type.type = TermType::Substring;
        term = str;
    }
}

}
