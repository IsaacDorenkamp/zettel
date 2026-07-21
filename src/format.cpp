#include "format.hpp"

using std::string;

namespace zettel {

string stripWhitespace(string other) {
    size_t start;
    size_t end;

    for (start = 0; start < other.size() && std::isspace(other[start]); start++);
    for (end = other.size() - 1; end > 0 && end >= start && std::isspace(other[end]); end--);

    if (start < end) return other.substr(start, (end - start) + 1);
    else return other;
}

}
