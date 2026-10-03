#pragma once

#include <filesystem>
#include <map>
#include <string>

namespace zettel {

namespace meta {

void write(std::filesystem::path dest, std::map<std::string, std::string> metadata);
std::map<std::string, std::string> read(std::filesystem::path source);

}

}
