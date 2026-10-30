#pragma once

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include "index.hpp"

namespace zettel {

class Matcher {
public:
    using Match = std::pair<size_t, std::string>;

    /*
     * Searching means that the matcher is not currently considering input as possibly contributing
     * to a match. PossibleMatch means that the matcher is considering input as possibly belonging
     * to a match, but the match is not complete. FoundMatch means that a complete match has been
     * found. BrokeMatch means that the most recent input character invalidates the possibility of
     * a match. EndMatch means that the current character cannot contribute further to the match,
     * but that the currently accrued possible match should be treated and accepted as a match.
     * Error indicates that the query is malformed and that the search should be stopped. If Error
     * is returned by accept(), m_error MUST be populated.
     */
    enum State {
        Searching, PossibleMatch, FoundMatch, BrokeMatch, EndMatch, Error
    };

    Matcher(std::string source, std::string query);
    virtual ~Matcher() = default;
    virtual State accept(char c) = 0;
    bool done() const;
    State next();

    const std::optional<std::string>& error() const;
    std::optional<Match> takeMatch();
protected:
    State error(const std::string& err);
    std::string m_source;
    std::string m_query;
    size_t m_index;
private:
    State m_state;
    size_t m_startedAt;
    std::stringstream m_possibleMatch;
    std::optional<std::string> m_error;
    std::optional<Match> m_match;
};

class TermMatcher : public Matcher {
public:
    TermMatcher(std::string source, std::string query, bool caseSensitive = false);
    virtual ~TermMatcher() = default;
    virtual State accept(char c);
private:
    bool matches(char a, char b) const;
    ssize_t m_queryIndex;
    bool m_caseSensitive;
};

using SearchResult = std::pair<size_t, std::string>;
enum class SearchType {
    TermCS, TermCI
};
std::pair<size_t, std::string> search(std::string source, std::string query, SearchType type);

struct FullTextSearchQuery {
    std::string term;
    std::filesystem::path file;
    SearchType type;
};

struct FullTextSearchResult {
    std::optional<zettel::models::zettel> match;
    std::string matchText;
};

std::variant<FullTextSearchResult, std::string> searchFile(const FullTextSearchQuery& query);

}
