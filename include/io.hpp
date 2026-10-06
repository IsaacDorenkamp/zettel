#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace zettel {

namespace io {

std::string readfile(std::ifstream& infile);
std::optional<std::string> readfile(const std::filesystem::path& file);

}

}
