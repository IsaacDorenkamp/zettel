#pragma once

#include "exception.hpp"
#include <filesystem>
#include <string>
#include <vector>

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

void cmd_new(const Context& ctx, const NewOptions& options);

}

}
