#pragma once

#include "exception.hpp"
#include <optional>
#include <string>

namespace zettel {

class URI {
public:
    DEFINE_EXCEPTION;

    URI(const std::string& scheme, const std::optional<std::string>& authority, const std::string& path, const std::optional<std::string>& query,
            const std::optional<std::string>& fragment);
    URI();

    const std::string& scheme() const;
    const std::optional<std::string>& authority() const;
    const std::string& path() const;
    const std::optional<std::string>& query() const;
    const std::optional<std::string>& fragment() const;

    void setScheme(const std::string& scheme);
    void setAuthority(const std::optional<std::string>& authority);
    void setPath(const std::string& path);
    void setQuery(const std::optional<std::string>& query);
    void setFragment(const std::optional<std::string>& fragment);

    const std::string& represent() const;

    static URI parse(std::string uri);
private:
    void updateRepr();

    std::string m_scheme;
    std::optional<std::string> m_authority;
    std::string m_path;
    std::optional<std::string> m_query;
    std::optional<std::string> m_fragment;

    std::string m_repr;
};

}
