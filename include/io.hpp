#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace zettel {

namespace io {

std::string readfile(std::ifstream& infile);
std::string readfile(const std::filesystem::path& file);

}

}
