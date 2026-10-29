#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "exception.hpp"
#include "index.hpp"
#include "zettel.hpp"

#define ZETTEL_VERSION "1.1.0"

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
    bool use_stdin;
    bool report;
};

enum class SearchType {
    Tag, Title, FullText
};

struct SearchOptions {
    SearchType type;
    std::string term;
    bool plain;
};

struct EditOptions {
    Zettel::Id id;
    std::optional<std::string> title;
    bool detailsOnly;
    bool use_stdin;
    std::vector<std::string> addTags;
    std::vector<std::string> removeTags;
    bool strictAdd;
    bool strictRemove;
};

struct DeleteOptions {
    Zettel::Id id;
    bool force;
};

struct ViewOptions {
    Zettel::Id id;
    bool ansi;
};

struct ListOptions {
    bool plain;
};

std::unique_ptr<Zettel> make_new(const Context& ctx, const NewOptions& options);
size_t search(const Context& ctx, const SearchOptions& options);
std::unique_ptr<Zettel> edit(const Context& ctx, const EditOptions& options);
void remove(const Context& ctx, const DeleteOptions& options);
void view(const Context& ctx, const ViewOptions& options);
void list(const Context& ctx, const ListOptions& options);
uint32_t check(const Context& ctx);

}

}
