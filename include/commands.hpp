#pragma once

#include "exception.hpp"
#include <filesystem>

namespace zettel {

namespace cmd {

DEFINE_CUSTOM_EXCEPTION(CommandException);

struct Context {
    std::filesystem::path root;
};

void initialize(const Context& ctx);

}

}
