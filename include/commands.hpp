#pragma once

#include "exception.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "zettel.hpp"

namespace zettel {

namespace cmd {

DEFINE_CUSTOM_EXCEPTION(CommandException);

struct Context {
    std::filesystem::path root;
};

void initialize(const Context& ctx);

struct NewOptions {
    std::string title;
    std::vector<std::string> tags;
    bool edit;
};

std::unique_ptr<Zettel> make_new(const Context& ctx, const NewOptions& options);

}

}
