#include "io.hpp"

#include <sstream>

using std::getline, std::string, std::ifstream, std::stringstream;

namespace zettel {

namespace io {

string readfile(ifstream& infile) {
    stringstream content;
    string line;
    while (!infile.eof()) {
        getline(infile, line);
        if (infile.fail()) break;
        content << line;
        if (!infile.eof()) content << '\n';
    }
    return content.str();
}

}

}
