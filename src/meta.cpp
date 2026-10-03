#include "meta.hpp"

#include <fstream>
#include <sstream>

using std::map, std::pair, std::string, std::stringstream;

namespace zettel {

namespace meta {

void write(std::filesystem::path dest, std::map<std::string, std::string> data) {
    using std::endl, std::ofstream;
    ofstream metafile(dest);
    for (const pair<const string, string>& entry : data) {
        metafile << entry.first << ' ' << entry.second << endl;
    }
}

static pair<string, string> _parseline(const string& line) {
    stringstream stream;
    size_t index;
    char c;
    for (index = 0; index < line.size(); index++) {
        c = line[index];
        if (c == ' ') {
            index++;
            break;
        }
        stream << c;
    }
    string key = stream.str();
    stream.str("");
    for (; index < line.size(); index++) stream << line[index];
    return { key, stream.str() };
}

map<string, string> read(std::filesystem::path source) {
    using std::ifstream, std::stringstream;
    stringstream line;
    char buf[1024];
    ifstream metafile(source);
    map<string, string> values;
    while (!metafile.eof()) {
        metafile.getline(buf, 1024);
        line << buf;
        if (metafile.fail()) continue;
        values.emplace(_parseline(line.str()));
        line.str("");
    }
    if (line.tellp() > 0) values.emplace(_parseline(line.str()));
    return values;
}

}

}
