#include "io.hpp"

#include <sstream>

using std::filesystem::path, std::optional, std::string, std::ifstream, std::stringstream;

namespace zettel {

namespace io {

string readfile(ifstream& infile) {
    stringstream content;
    char buf[1024];
    int numRead;
    while (!infile.eof()) {
        infile.read(buf, 1024);
        numRead = infile.gcount();
        content.write(buf, numRead);
    }
    return content.str();
}

optional<string> readfile(const path& file) {
    ifstream stream;
    stream.open(file);
    if (stream.good()) return readfile(stream);
    else return std::nullopt;
}

}

}
