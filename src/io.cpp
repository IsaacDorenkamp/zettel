#include "io.hpp"

#include <sstream>

using std::filesystem::path, std::string, std::ifstream, std::stringstream;

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

string readfile(const path& file) {
    ifstream stream;
    stream.open(file);
    return readfile(stream);
}

}

}
