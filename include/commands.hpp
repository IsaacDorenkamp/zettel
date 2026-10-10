#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "exception.hpp"
#include "index.hpp"
#include "zettel.hpp"

namespace zettel {

namespace cmd {

DEFINE_CUSTOM_EXCEPTION(CommandException);

struct Context {
    std::filesystem::path root;
    std::filesystem::path dotdir;
    std::shared_ptr<sql::SQLite> db;
    std::unique_ptr<Index> index;

    bool valid() const;
};

void initialize(const Context& ctx);

struct NewOptions {
    std::string title;
    std::vector<std::string> tags;
    bool edit;
};

struct SearchOptions {
    std::string tag;
};

struct EditOptions {
    uint32_t id;
};

std::unique_ptr<Zettel> make_new(const Context& ctx, const NewOptions& options);
std::vector<models::zettel> search(const Context& ctx, const SearchOptions& options);
std::unique_ptr<Zettel> edit(const Context& ctx, const EditOptions& options);

}

}
